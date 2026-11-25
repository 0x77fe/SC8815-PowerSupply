#include "Preferences.h"
#include "board_config.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "input.h"
#include "settings.h"
#include "task_context.h"

namespace SaveTask
{
    // 命名空间内以一个键存储电压、限流和功率三元组。
    static constexpr char settings_namespace[] = "sc8815";

    void loadSettings()
    {
        ESP_ERROR_CHECK(nvs_flash_init());

        // 缺失的设定项使用板级默认值。
        std::uint32_t voltage = board_default_voltage_mv;
        std::uint32_t current = board_default_current_ma;
        std::uint32_t power = board_default_power_limit_w;
        Preferences preferences;
        if (preferences.begin(settings_namespace, true))
        {
            voltage = preferences.getUInt("setmvolts", voltage);
            current = preferences.getUInt("setmCurrs", current);
            power = preferences.getUInt("powerLimitW", power);
            preferences.end();
        }
        output_settings = InputTask::normalizeSettings(voltage, current, power);
    }

    void run(void *)
    {
        // 按固定周期运行。
        TickType_t last_wake = xTaskGetTickCount();
        for (;; xTaskDelayUntil(&last_wake, pdMS_TO_TICKS(board_save_period_ms)))
        {
            // 检查待保存标记和静默时长，条件满足时提交当前设定。
            xSemaphoreTake(state_mutex, portMAX_DELAY);
            const auto now_ms = static_cast<std::uint32_t>(esp_timer_get_time() / 1000);
            if (!output_settings.SavePending || now_ms - output_settings.ChangedAtMs <= board_save_delay_ms)
            {
                xSemaphoreGive(state_mutex);
                continue;
            }

            // 在释放状态锁前结束 Preferences 会话，写入失败时保留标记。
            Preferences preferences;
            if (preferences.begin(settings_namespace))
            {
                bool saved =
                preferences.putUInt("setmvolts", output_settings.VoltageMv) &&
                preferences.putUInt("setmCurrs", output_settings.CurrentMa) &&
                preferences.putUInt("powerLimitW", output_settings.PowerLimitW);
                preferences.end();
                output_settings.SavePending = !saved;
            }
            xSemaphoreGive(state_mutex);
        }
    }
}
