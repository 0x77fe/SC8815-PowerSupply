#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "i2c.h"
#include "screen.h"
#include "task_context.h"

/*
* 初始化硬件、共享状态和运行任务。
*/
extern "C" void app_main()
{
    vTaskDelay(pdMS_TO_TICKS(1000)); // 等待 PD 握手
    i2cInitialize();
    if (!screenInitialize() || !screenShowStartup()) ESP_LOGW("power_supply", "SSD1306 initialization failed");
    vTaskDelay(pdMS_TO_TICKS(1000));

    // 加载设定并初始化电源和输入设备。
    SaveTask::loadSettings();
    InputTask::powerInitialize();
    InputTask::initializeInput();

    // 在启动任务前创建状态互斥锁。
    state_mutex = xSemaphoreCreateMutex();
    ESP_ERROR_CHECK(state_mutex != nullptr ? ESP_OK : ESP_ERR_NO_MEM);

    // 采样优先于输入，显示和保存使用较低优先级。
    xTaskCreate(SampleTask::run, "power_sample", task_stack_bytes, nullptr, priority_sample, nullptr);
    xTaskCreate(SaveTask::run, "settings_save", task_stack_bytes, nullptr, priority_save, nullptr);
    xTaskCreate(DisplayTask::run, "oled_display", task_stack_bytes, nullptr, priority_display, nullptr);
    xTaskCreate(InputTask::run, "user_input", task_stack_bytes, nullptr, priority_input, nullptr);
}
