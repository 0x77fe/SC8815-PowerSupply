#include <algorithm>
#include <cmath>
#include <string_view>

#include "board_config.h"
#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_intr_alloc.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "input.h"
#include "power_supply.h"
#include "task_context.h"

namespace InputTask
{
    // 记录功率调节时间和编码器中断状态。
    static constinit std::uint32_t power_adjusted_at_ms = 0;
    // 编码器中断与输入任务通过同一临界区交接步数和旋转标志。
    static constinit portMUX_TYPE encoder_lock = portMUX_INITIALIZER_UNLOCKED;
    static constinit std::int32_t pending_regular_steps = 0;
    static constinit std::int32_t pending_power_steps = 0;
    static constinit bool turned_while_pressed = false;
    static constinit std::uint8_t previous_encoder_state = 0;
    static constinit std::int8_t quarter_steps = 0;

    static_assert(CONFIG_GPIO_CTRL_FUNC_IN_IRAM, "输入中断要求 GPIO 读取函数位于 IRAM");

    static constexpr auto encoder_a = static_cast<gpio_num_t>(board_encoder_a);
    static constexpr auto encoder_b = static_cast<gpio_num_t>(board_encoder_b);
    static constexpr auto button_pin = static_cast<gpio_num_t>(board_button);

    // 正交状态按 AB 编码，正向顺序为 11、10、00、01、11。
    static DRAM_ATTR const std::int8_t transition[4][4] =
    {
        { 0,  1, -1,  0},
        {-1,  0,  0,  1},
        { 1,  0,  0, -1},
        { 0, -1,  1,  0}
    };

    /*
    * 解码 A/B 相变化并累计完整的旋钮步进。
    */
    static void IRAM_ATTR encoderInterrupt(void *)
    {
        // 持锁读取引脚并更新编码器状态。
        portENTER_CRITICAL_ISR(&encoder_lock);

        // 依次读取 A、B 两相并组合为状态。
        const int a = gpio_get_level(encoder_a);
        const int b = gpio_get_level(encoder_b);
        const std::uint8_t state = static_cast<std::uint8_t>((a << 1) | b);

        // 解码相变。
        const std::uint8_t changed = previous_encoder_state ^ state;
        const bool button_down = gpio_get_level(button_pin) == 0;
        turned_while_pressed |= changed != 0 && button_down;

        // 两相同时变化时方向不明确，从当前状态重新同步。
        if (changed == 3) quarter_steps = 0;
        else if (changed != 0)
        {
            // 相邻状态往返互相抵消，完整的四次相变才产生一步。
            quarter_steps += transition[previous_encoder_state][state];
            if (quarter_steps == 4 || quarter_steps == -4)
            {
                // 根据按键电平将完整步进计入对应的计数器。
                const int direction = quarter_steps > 0 ? 1 : -1;
                if (button_down) pending_power_steps += direction;
                else pending_regular_steps += direction;
                quarter_steps = 0;
            }
        }
        previous_encoder_state = state;
        portEXIT_CRITICAL_ISR(&encoder_lock);
    }

    void initializeInput()
    {
        const gpio_config_t config =
        {
            .pin_bit_mask = (1ULL << encoder_a) | (1ULL << encoder_b) | (1ULL << button_pin),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        ESP_ERROR_CHECK(gpio_config(&config));

        // 依次读取 A、B 两相，记录初始状态。
        const int a = gpio_get_level(encoder_a);
        const int b = gpio_get_level(encoder_b);
        previous_encoder_state = static_cast<std::uint8_t>((a << 1) | b);

        // ISR 和转移表驻留内部存储器，Flash 操作期间继续捕获编码器边沿。
        ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_LEVEL1));
        ESP_ERROR_CHECK(gpio_set_intr_type(encoder_a, GPIO_INTR_ANYEDGE));
        ESP_ERROR_CHECK(gpio_set_intr_type(encoder_b, GPIO_INTR_ANYEDGE));
        ESP_ERROR_CHECK(gpio_isr_handler_add(encoder_a, encoderInterrupt, nullptr));
        ESP_ERROR_CHECK(gpio_isr_handler_add(encoder_b, encoderInterrupt, nullptr));
    }

    OutputSettings normalizeSettings(std::int64_t voltage_mv, std::int64_t current_ma, std::int64_t power_limit_w)
    {
        voltage_mv = std::clamp<std::int64_t>(voltage_mv, board_min_voltage_mv, board_max_voltage_mv);
        power_limit_w = std::clamp<std::int64_t>(power_limit_w, board_min_power_limit_w, board_max_power_limit_w);
        power_limit_w = (power_limit_w / board_power_limit_step_w) * board_power_limit_step_w;

        // 按功率向下量化电流上限，并约束在 IBUS 限流量程内。
        const std::int64_t power_ceiling_ma = (power_limit_w * 1000000 / voltage_mv /
                                        board_current_ceiling_step_ma) * board_current_ceiling_step_ma;
        constexpr auto ibus_full_scale_ma = static_cast<std::int64_t>(board_ibus_full_scale_a * 1000.0f);
        const std::int64_t ceiling_ma = std::min(power_ceiling_ma, ibus_full_scale_ma);
        current_ma = std::clamp<std::int64_t>(current_ma, board_min_current_ma, ceiling_ma);
        return
        {
            static_cast<std::uint16_t>(voltage_mv),
            static_cast<std::uint16_t>(current_ma),
            static_cast<std::uint16_t>(power_limit_w)
        };
    }

    // 控制电源待机引脚。
    static constexpr auto power_stop_pin = static_cast<gpio_num_t>(board_power_stop);

    /*
    * 在待机且设定已应用时使能输出。
    */
    static bool powerEnableOutput()
    {
        if (output_state != OUTPUT_Standby || !has_applied) return false;

        ESP_ERROR_CHECK(gpio_set_level(power_stop_pin, 0));
        output_state = OUTPUT_Enabled;
        return true;
    }

    /*
    * 关闭输出并清除输出采样的有效状态。
    */
    static void powerDisableOutput()
    {
        ESP_ERROR_CHECK(gpio_set_level(power_stop_pin, 1));
        SampleTask::resetOutputSamples();
        if (output_state == OUTPUT_Enabled) output_state = OUTPUT_Standby;
    }

    /*
    * 在写入失败时关闭输出并记录失败步骤。
    */
    static bool powerFail(std::string_view step)
    {
        powerDisableOutput();
        has_applied = false;
        ESP_LOGW("power_supply", "%.*s", static_cast<int>(step.size()), step.data());
        return false;
    }

    /*
    * 输出设定与寄存器量程换算。
    */
    static bool powerWriteVoltage()
    {
        // 反算校正后的指令，再舍入到最接近的 2 mV 参考档位。
        const float reference_step_mv = 2.0f;
        const float steps = (output_settings.VoltageMv - board_output_voltage_offset_mv) /
                            (board_output_voltage_gain * board_feedback_ratio * reference_step_mv);
        const auto reference_mv = static_cast<std::uint16_t>(
            std::clamp(std::round(steps) * reference_step_mv, 2.0f, 2048.0f));
        return sc8815VbusRefESet(power_chip, reference_mv);
    }

    /*
    * 将设定的电流上限写入芯片。
    */
    static bool powerWriteCurrent()
    {
        const float limit_a = std::clamp(output_settings.CurrentMa * 0.001f, board_ibus_full_scale_a / 256.0f, board_ibus_full_scale_a);
        return sc8815IbusLimSet(power_chip, limit_a);
    }

    /*
    * 按安全顺序应用电压和限流设定。
    */
    static bool powerApply()
    {
        // 升压或降低限流时优先写限流；降压并提高限流时优先写电压。
        const bool current_changed = !has_applied || output_settings.CurrentMa != applied_settings.CurrentMa;
        const bool voltage_changed = !has_applied || output_settings.VoltageMv != applied_settings.VoltageMv;
        const bool voltage_first = has_applied && output_settings.VoltageMv < applied_settings.VoltageMv &&
                                  output_settings.CurrentMa > applied_settings.CurrentMa;

        if (voltage_first && !powerWriteVoltage()) return powerFail("Output voltage");
        if (current_changed && !powerWriteCurrent()) return powerFail("Current limit");
        if (!voltage_first && voltage_changed && !powerWriteVoltage()) return powerFail("Output voltage");

        // 两项写入均成功后提交硬件设定。
        applied_settings = output_settings;
        has_applied = true;
        return true;
    }

    bool powerInitialize()
    {
        // 初始化PSTOP
        gpio_set_level(power_stop_pin, 1);
        gpio_set_direction(power_stop_pin, GPIO_MODE_OUTPUT);

        // 复位参数
        output_state = OUTPUT_Standby;
        has_applied = false;
        SampleTask::resetSamples();

        // 芯片配置和初始设定。
        constexpr Sc8815DitheringAndSlewRateConfig slew =
        {
            .FactoryBit = SC8815_FACTORY_POWERUP,
            .EnDither = SC8815_EN_DITHER_DISABLE,
            .Slew = SC8815_SLEW_SET_2mV_us,
        };

        if (!sc8815Begin(power_chip)) return powerFail("I2C probe");
        if (!sc8815VbatSet(power_chip, board_battery)) return powerFail("Battery config");
        if (!sc8815Ratio(power_chip, board_ratios)) return powerFail("Ratio config");
        if (!sc8815Ctrl0Set(power_chip, board_switching)) return powerFail("Switching config");
        if (!sc8815Ctrl1Set(power_chip, board_feedback)) return powerFail("Feedback config");
        if (!sc8815Ctrl2Set(power_chip, slew)) return powerFail("Slew config");
        if (!sc8815Ctrl3Set(power_chip, board_monitoring)) return powerFail("ADC config");
        if (!sc8815SetIrqMask(power_chip, {})) return powerFail("IRQ mask");
        return powerApply();
    }

    /*
    * 将编码器步进转换为一组有效设定。
    */
    static void applyEncoderSteps(std::int32_t regular_steps, std::int32_t power_steps, std::uint32_t now_ms)
    {
        std::int64_t voltage_mv = output_settings.VoltageMv;
        std::int64_t current_ma = output_settings.CurrentMa;
        if (adjust_mode == ADJUST_Voltage) voltage_mv += static_cast<std::int64_t>(regular_steps) * board_voltage_step_mv;
        else current_ma += static_cast<std::int64_t>(regular_steps) * board_current_step_ma;

        const std::int64_t power_limit_w = output_settings.PowerLimitW +
            static_cast<std::int64_t>(power_steps) * board_power_limit_step_w;
        const OutputSettings next = normalizeSettings(voltage_mv, current_ma, power_limit_w);
        if (next.VoltageMv == output_settings.VoltageMv && next.CurrentMa == output_settings.CurrentMa && next.PowerLimitW == output_settings.PowerLimitW) return;

        // 更新用户设定，记录保存静默期的起点。
        output_settings = next;
        output_settings.ChangedAtMs = now_ms;
        output_settings.SavePending = true;

        // 已完成芯片初始化和设定写入时，更新输出寄存器。
        if (has_applied) powerApply();
    }

    void run(void *)
    {
        constexpr std::uint32_t long_press_ms = 1000;
        bool pressed = false;
        bool long_press_triggered = false;
        std::uint32_t pressed_at_ms = 0;

        // 处理编码器步进并判定按键手势。
        TickType_t last_wake = xTaskGetTickCount();
        for (;; xTaskDelayUntil(&last_wake, pdMS_TO_TICKS(board_input_period_ms)))
        {
            xSemaphoreTake(state_mutex, portMAX_DELAY);
            const auto now_ms = static_cast<std::uint32_t>(esp_timer_get_time() / 1000);

            // 读取中断累计值，将旋转标记保留至松开按键。
            portENTER_CRITICAL(&encoder_lock);
            const bool button_down = gpio_get_level(button_pin) == 0;
            const bool turned = turned_while_pressed;
            const std::int32_t regular_steps = pending_regular_steps;
            const std::int32_t power_steps = pending_power_steps;
            pending_regular_steps = 0;
            pending_power_steps = 0;
            if (!button_down) turned_while_pressed = false;
            portEXIT_CRITICAL(&encoder_lock);

            // 按住旋钮转动一个完整步进时显示功率上限。
            if (turned)
            {
                power_limit_visible = true;
                power_adjusted_at_ms = now_ms;
            }
            if (regular_steps != 0 || power_steps != 0) applyEncoderSteps(regular_steps, power_steps, now_ms);

            // 记录本次按压的起点。
            if (button_down && !pressed)
            {
                pressed_at_ms = now_ms;
                long_press_triggered = false;
            }

            // 持续按住达到阈值时触发一次长按。
            const auto held_ms = now_ms - pressed_at_ms;
            if (button_down && !turned && !long_press_triggered && held_ms >= long_press_ms)
            {
                adjust_mode = adjust_mode == ADJUST_Voltage ? ADJUST_Current : ADJUST_Voltage;
                long_press_triggered = true;
            }

            // 松开时仅处理阈值以内的静止按压。
            if (!button_down && pressed && !turned && held_ms < long_press_ms)
            {
                if (output_state == OUTPUT_Enabled) powerDisableOutput();
                else if (output_state == OUTPUT_Standby) powerEnableOutput();
            }
            pressed = button_down;

            // 调节超时后隐藏功率上限。
            if (power_limit_visible && now_ms - power_adjusted_at_ms >= 1200) power_limit_visible = false;

            // 释放共享状态锁，等待下一轮轮询。
            xSemaphoreGive(state_mutex);
        }
    }
}
