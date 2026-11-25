#pragma once

#include "sc8815.h"

// ESP32-C3 引脚和共享 I2C 总线。
// 旋转编码器 A 相 GPIO。
inline constexpr auto board_encoder_a = 2;
// 旋转编码器 B 相 GPIO。
inline constexpr auto board_encoder_b = 3;
// 旋转编码器按键 GPIO。
inline constexpr auto board_button = 4;
// SC8815 待机控制 GPIO，高电平进入待机。
inline constexpr auto board_power_stop = 5;
// 共享 I²C 数据线 GPIO。
inline constexpr auto board_sda = 8;
// 共享 I²C 时钟线 GPIO。
inline constexpr auto board_scl = 9;
// 共享 I²C 时钟频率，单位 Hz。
inline constexpr auto board_i2c_clock = 400000;
// SSD1306 的 7 位 I²C 地址。
inline constexpr auto board_display_address = 0x3C;

// 板上反馈电阻与采样电阻。
// 外部电压反馈分压比。
inline constexpr auto board_feedback_ratio = 1.0f + 150.0f / 12.0f;
// VBUS 电流采样电阻，单位 mΩ。
inline constexpr auto board_rs1_mohm = 10.0f;
// VBAT 电流采样电阻，单位 mΩ。
inline constexpr auto board_rs2_mohm = 10.0f;

// VBUS 实测值等于量化指令乘增益再加偏移，偏移单位为 mV。
// 输出电压校正增益。
inline constexpr auto board_output_voltage_gain = 1.005490f;
// 输出电压校正偏移，单位 mV。
inline constexpr auto board_output_voltage_offset_mv = -22.26f;

// 采样校正按原始读数乘增益再加偏移，电流偏移以 A 为单位。
// VBUS 采样校正增益。
inline constexpr auto board_vbus_gain = 1.01217f;
// IBUS 采样校正偏移，单位 A。
inline constexpr auto board_ibus_offset_a = -0.020f;
// VBAT 采样校正增益。
inline constexpr auto board_vbat_gain = 1.00972f;

// 输出设定范围、步进和默认值。
// 输出电压设定下限。
inline constexpr auto board_min_voltage_mv = 2600;
// 输出电压设定上限。
inline constexpr auto board_max_voltage_mv = 25600;
// 输出限流设定下限。
inline constexpr auto board_min_current_ma = 300;
// 编码器电压调节步进。
inline constexpr auto board_voltage_step_mv = 100;
// 编码器电流调节步进。
inline constexpr auto board_current_step_ma = 100;
// 编码器功率上限调节步进。
inline constexpr auto board_power_limit_step_w = 10;
// 可设功率上限的最小值。
inline constexpr auto board_min_power_limit_w = 20;
// 可设功率上限的最大值。
inline constexpr auto board_max_power_limit_w = 100;
// 缺省功率上限。
inline constexpr auto board_default_power_limit_w = 100;
// 功率约束换算出的电流上限向下取整步进。
inline constexpr auto board_current_ceiling_step_ma = 500;
// 缺省输出电压。
inline constexpr auto board_default_voltage_mv = 5000;
// 缺省输出限流。
inline constexpr auto board_default_current_ma = 1000;

// FreeRTOS 任务周期与保存延迟。

// 按键轮询与编码器事件处理周期，单位 ms。
inline constexpr auto board_input_period_ms = 10;

// 电源状态和电压、电流采样周期，单位 ms。
inline constexpr auto board_sample_period_ms = 50;

// OLED 仪表界面刷新周期，单位 ms。
inline constexpr auto board_refresh_period_ms = 100;

// 最后一次设定变更到 Flash 写入的等待时间，单位 ms。
inline constexpr auto board_save_period_ms = 500;
inline constexpr auto board_save_delay_ms = 2000;

// 芯片工作配置。

// 电池电压检测与充电参数。
inline constexpr Sc8815VbatSetConfig board_battery =
{
    SC8815_IRCOMP_0mOhm, SC8815_VBAT_SEL_EXTERNAL, SC8815_CSEL_1S, SC8815_VCELL_4V20
};

// 输入、输出电压和电流的检测比例。
inline constexpr Sc8815RatioConfig board_ratios =
{
    SC8815_IBAT_RATIO_6x, SC8815_IBUS_RATIO_6x, SC8815_VBAT_MON_RATIO_12_5x, SC8815_VBUS_RATIO_12_5x
};

// IBUS 限流寄存器的满量程由检测倍率和采样电阻决定。
inline constexpr float board_ibus_ratio = board_ratios.IbusRatio == SC8815_IBUS_RATIO_6x ? 6.0f : 3.0f;
inline constexpr float board_ibus_full_scale_a = board_ibus_ratio * 10.0f / board_rs1_mohm;

// OTG 方向、输入调节比例及开关时序。
inline constexpr Sc8815ModeAndSwitchingConfig board_switching =
{
    SC8815_EN_OTG_ENABLE, SC8815_VINREG_RATIO_100x, SC8815_FREQ_300kHz, SC8815_DT_20ns
};

// 充电电流检测、反馈路径及保护选项。
inline constexpr Sc8815ChargingAndFeedbackConfig board_feedback =
{
    SC8815_ICHAR_SEL_IBUS, SC8815_DIS_TRICKLE_ENABLE, SC8815_DIS_TERM_ENABLE,
    SC8815_FB_SEL_EXTERNAL, SC8815_TRICKLE_SET_70, SC8815_DIS_OVP_DISABLE
};

// 功率路径、ADC 采样及控制环路选项。
inline constexpr Sc8815PowerPathAndMonitoringConfig board_monitoring =
{
    SC8815_EN_PGATE_OFF, SC8815_GPO_CTRL_OD, SC8815_AD_START_ENABLE, SC8815_ILIM_BW_SEL_5K,
    SC8815_LOOP_SET_IMPROVE, SC8815_DIS_SHORT_FOLDBACK_ENABLE, SC8815_EOC_SET_1_10, SC8815_EN_PFM_DISABLE
};
