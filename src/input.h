#pragma once

#include "task_context.h"

// 处理旋钮与按键操作，配置并控制电源输出。
namespace InputTask
{
    inline constexpr UBaseType_t priority = 4;

    /*
    * 配置芯片并应用输出设定。
    */
    bool powerInitialize();

    /*
    * 初始化编码器与按键。
    */
    void initializeInput();

    /*
    * 按板级约束规范化输出设定。
    */
    OutputSettings normalizeSettings(std::int64_t voltage_mv, std::int64_t current_ma,
                                     std::int64_t power_limit_w);

    /*
    * 轮询编码器和按键。
    */
    void run(void *);
}
