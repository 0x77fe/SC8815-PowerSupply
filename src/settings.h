#pragma once

#include "freertos/FreeRTOS.h"

// 加载和保存用户输出设定。
namespace SaveTask
{
    inline constexpr UBaseType_t priority = 2;

    /*
    * 加载已保存的输出设定。
    */
    void loadSettings();

    /*
    * 周期检查并保存设定。
    */
    void run(void *);
}
