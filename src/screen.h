#pragma once

#include "freertos/FreeRTOS.h"

/*
* 在共享总线上初始化板载 SSD1306。
*/
bool screenInitialize();

/*
* 同步显示启动画面，并返回传输结果。
*/
bool screenShowStartup();

// 在 OLED 上显示共享读数与设定。
namespace DisplayTask
{
    inline constexpr UBaseType_t priority = 3;

    /*
    * 周期刷新 OLED 仪表。
    */
    void run(void *);
}
