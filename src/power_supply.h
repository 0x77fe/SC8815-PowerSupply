#pragma once

#include "freertos/FreeRTOS.h"

// 采集电压和电流并维护滤波状态。
namespace SampleTask
{
    inline constexpr UBaseType_t priority = 5;

    /*
    * 清除输出采样状态和滤波窗口。
    */
    void resetOutputSamples();

    /*
    * 清除全部采样状态和滤波窗口。
    */
    void resetSamples();

    /*
    * 周期采集电压和电流。
    */
    void run(void *);
}
