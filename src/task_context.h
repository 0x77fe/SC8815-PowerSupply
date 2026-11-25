#pragma once

#include <cstdint>

#include "board_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "i2c.h"
#include "sc8815.h"

// 各任务共用的栈大小。
inline constexpr std::uint32_t task_stack_bytes = 4096;

enum AdjustMode
{
    ADJUST_Voltage,
    ADJUST_Current
};

struct OutputSettings
{
    std::uint16_t VoltageMv;
    std::uint16_t CurrentMa;
    std::uint16_t PowerLimitW;
    std::uint32_t ChangedAtMs = 0;
    bool SavePending = false;
};

enum OutputState
{
    OUTPUT_Standby,
    OUTPUT_Enabled
};

struct PowerSample
{
    std::uint16_t VbusMv;
    std::uint16_t VbatMv;
    float IbusA;
    float IbatA;
    bool VbusValid;
    bool VbatValid;
    bool IbusValid;
    bool IbatValid;
};

// 共享的 SC8815 总线与采样电阻配置。
inline constexpr Sc8815 power_chip{i2cTransfer, sc8815_i2c_address, board_rs1_mohm, board_rs2_mohm};

// 启动后由 state_mutex 保护任务间的共享状态。
inline constinit SemaphoreHandle_t state_mutex = nullptr;
inline constinit OutputSettings output_settings{};
inline constinit AdjustMode adjust_mode = ADJUST_Voltage;
inline constinit bool power_limit_visible = false;
inline constinit OutputState output_state = OUTPUT_Standby;
inline constinit bool has_applied = false;
inline constinit OutputSettings applied_settings{};
inline constinit PowerSample power_sample{};
