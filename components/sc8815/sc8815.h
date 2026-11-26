#pragma once

#include <cstdint>
#include <span>

#include "sc8815_reg.h"

inline constexpr std::uint8_t sc8815_i2c_address = 0x74;

struct Sc8815
{
    // 同步传输接口支持写入、重复起始读取和仅发送地址的探测。
    bool (&Transfer)(std::uint8_t address, std::span<const std::uint8_t> write,
                     std::span<std::uint8_t> read);
    std::uint8_t Address;
    float Rs1MOhm;
    float Rs2MOhm;
};

/*
* 探测 I²C 总线上的 SC8815。
*/
bool sc8815Begin(const Sc8815 &chip);

/*
* 读取单字节寄存器。
*/
bool sc8815ReadRegister(const Sc8815 &chip, Sc8815Register reg, std::uint8_t &data);

/*
* 配置电池内阻补偿、节数及目标电压。
*/
bool sc8815VbatSet(const Sc8815 &chip, Sc8815VbatSetConfig config);

/*
* 配置电压监测及电流检测倍率。
*/
bool sc8815Ratio(const Sc8815 &chip, Sc8815RatioConfig config);

/*
* 配置 OTG、VINREG 倍率、开关频率及死区。
*/
bool sc8815Ctrl0Set(const Sc8815 &chip, Sc8815ModeAndSwitchingConfig config);

/*
* 配置充电电流检测、反馈路径及保护选项。
*/
bool sc8815Ctrl1Set(const Sc8815 &chip, Sc8815ChargingAndFeedbackConfig config);

/*
* 配置工厂位、扩频抖动及电压变化斜率。
*/
bool sc8815Ctrl2Set(const Sc8815 &chip, Sc8815DitheringAndSlewRateConfig config);

/*
* 配置功率路径、ADC 和控制环路。
*/
bool sc8815Ctrl3Set(const Sc8815 &chip, Sc8815PowerPathAndMonitoringConfig config);

/*
* 写入 VBUS 内部参考电压。
*/
bool sc8815VbusRefISet(const Sc8815 &chip, std::uint16_t ref_mv);

/*
* 写入 VBUS 外部参考电压。
*/
bool sc8815VbusRefESet(const Sc8815 &chip, std::uint16_t ref_mv);

/*
* 根据检测倍率及采样电阻设定 IBUS 电流上限。
*/
bool sc8815IbusLimSet(const Sc8815 &chip, float current_a);

/*
* 根据检测倍率及采样电阻设定 IBAT 电流上限。
*/
bool sc8815IbatLimSet(const Sc8815 &chip, float current_a);

/*
* 按当前倍率设定 VINREG 电压。
*/
bool sc8815VinregSet(const Sc8815 &chip, std::uint16_t voltage_mv);

/*
* 读取 VBUS 电压，单位为毫伏。
*/
bool sc8815ReadVbusVoltage(const Sc8815 &chip, std::uint16_t &voltage_mv);

/*
* 读取 VBAT 电压，单位为毫伏。
*/
bool sc8815ReadVbatVoltage(const Sc8815 &chip, std::uint16_t &voltage_mv);

/*
* 读取 IBUS 电流，单位为安培。
*/
bool sc8815ReadIbusCurrent(const Sc8815 &chip, float &current_a);

/*
* 读取 IBAT 电流，单位为安培。
*/
bool sc8815ReadIbatCurrent(const Sc8815 &chip, float &current_a);

/*
* 读取 ADIN 电压，单位为毫伏。
*/
bool sc8815ReadAdinVoltage(const Sc8815 &chip, std::uint16_t &voltage_mv);

/*
* 读取并解析芯片状态。
*/
bool sc8815GetStatus(const Sc8815 &chip, Sc8815Status &status);

/*
* 写入芯片中断掩码。
*/
bool sc8815SetIrqMask(const Sc8815 &chip, Sc8815IrqMaskConfig config);
