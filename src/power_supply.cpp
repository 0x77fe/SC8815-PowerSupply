#include <algorithm>

#include "board_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "input.h"
#include "power_supply.h"
#include "task_context.h"

namespace SampleTask
{
    class SampleFilter
    {
    public:
        /*
        * 将样本加入滑动窗口并返回低通结果。
        */
        float push(float value)
        {
            // 第一级对窗口内有效样本取平均。
            Values[Next] = value;
            Next = (Next + 1) % filter_window_size;
            if (Count < filter_window_size) ++Count;

            float sum = 0.0f;
            for (std::uint8_t i = 0; i < Count; ++i) sum += Values[i];
            const float average = sum / Count;

            // 第二级以首个有效读数初始化，此后对滑动平均执行一阶低通。
            const float alpha = 0.4f;
            if (Count == 1) LowPass = average;
            else LowPass += alpha * (average - LowPass);
            return LowPass;
        }

        /*
        * 清空有效采样窗口。
        */
        void reset()
        {
            Count = 0;
            Next = 0;
        }

    private:
        static constexpr unsigned filter_window_size = 10;

        float Values[filter_window_size];
        float LowPass;
        std::uint8_t Count;
        std::uint8_t Next;
    };

    // 各采样通道独立维护滤波窗口。
    static constinit SampleFilter vbus_filter{};
    static constinit SampleFilter vbat_filter{};
    static constinit SampleFilter ibus_filter{};
    static constinit SampleFilter ibat_filter{};

    void resetOutputSamples()
    {
        vbus_filter.reset();
        ibus_filter.reset();
        power_sample.VbusValid = false;
        power_sample.IbusValid = false;
    }

    void resetSamples()
    {
        resetOutputSamples();
        vbat_filter.reset();
        ibat_filter.reset();
        power_sample = {};
    }

    void run(void *)
    {
        // 首轮立即采样，每条路径在解锁后等待下一周期。
        TickType_t last_wake = xTaskGetTickCount();
        for (;; xTaskDelayUntil(&last_wake, pdMS_TO_TICKS(board_sample_period_ms)))
        {
            // 持锁读取采样通道并更新读数。
            xSemaphoreTake(state_mutex, portMAX_DELAY);

            // 输出开启时先检查短路标志，关断后保持故障状态。
            if (output_state == OUTPUT_Enabled)
            {
                Sc8815Status status{};
                if (sc8815GetStatus(power_chip, status) && status.VbusShort)
                {
                    output_state = OUTPUT_Short;
                    InputTask::powerDisableOutput();
                }
            }

            // 读取输入电压。
            std::uint16_t vbat_mv;
            power_sample.VbatValid = sc8815ReadVbatVoltage(power_chip, vbat_mv);
            if (power_sample.VbatValid)
            {
                // 校正并滤波，将结果四舍五入并限幅为整数毫伏。
                const float filtered_vbat_mv = vbat_filter.push(vbat_mv * board_vbat_gain);
                power_sample.VbatMv = static_cast<std::uint16_t>(std::clamp(filtered_vbat_mv + 0.5f, 0.0f, 65535.0f));
            }
            else vbat_filter.reset();

            // 读取输入电流。
            float ibat_a;
            power_sample.IbatValid = sc8815ReadIbatCurrent(power_chip, ibat_a);
            if (power_sample.IbatValid) power_sample.IbatA = ibat_filter.push(ibat_a);
            else ibat_filter.reset();

            // 输出开启时才读取 VBUS 和 IBUS。
            if (output_state != OUTPUT_Enabled)
            {
                xSemaphoreGive(state_mutex);
                continue;
            }

            // 读取输出电压。
            std::uint16_t vbus_mv;
            power_sample.VbusValid = sc8815ReadVbusVoltage(power_chip, vbus_mv);
            if (power_sample.VbusValid)
            {
                // 校正并滤波，将结果四舍五入并限幅为整数毫伏。
                const float filtered_vbus_mv = vbus_filter.push(vbus_mv * board_vbus_gain);
                power_sample.VbusMv = static_cast<std::uint16_t>(std::clamp(filtered_vbus_mv + 0.5f, 0.0f, 65535.0f));
            }
            else vbus_filter.reset();

            // 读取输出电流。
            float ibus_a;
            power_sample.IbusValid = sc8815ReadIbusCurrent(power_chip, ibus_a);
            if (power_sample.IbusValid)
            {
                const float corrected_a = ibus_a + board_ibus_offset_a;
                power_sample.IbusA = ibus_filter.push(std::max(corrected_a, 0.0f));
            }
            else ibus_filter.reset();

            // 释放状态锁，进入下一周期。
            xSemaphoreGive(state_mutex);
        }
    }
}
