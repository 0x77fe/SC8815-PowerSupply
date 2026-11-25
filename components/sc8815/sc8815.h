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

bool sc8815Begin(const Sc8815 &chip);
bool sc8815ReadRegister(const Sc8815 &chip, Sc8815Register reg, std::uint8_t &data);
bool sc8815ConfigureBatteryCharging(const Sc8815 &chip, Sc8815VbatSetConfig config);
bool sc8815ConfigureMeasurementAndLimitRatios(const Sc8815 &chip, Sc8815RatioConfig config);
bool sc8815ConfigureModeAndSwitching(const Sc8815 &chip, Sc8815ModeAndSwitchingConfig config);
bool sc8815ConfigureChargingAndFeedback(const Sc8815 &chip, Sc8815ChargingAndFeedbackConfig config);
bool sc8815ConfigureDitheringAndSlewRate(const Sc8815 &chip, Sc8815DitheringAndSlewRateConfig config);
bool sc8815ConfigurePowerPathAndMonitoring(const Sc8815 &chip, Sc8815PowerPathAndMonitoringConfig config);
bool sc8815SetVbusRefI(const Sc8815 &chip, std::uint16_t ref_mv);
bool sc8815SetVbusRefE(const Sc8815 &chip, std::uint16_t ref_mv);
bool sc8815SetIbusLimit(const Sc8815 &chip, float current_a);
bool sc8815SetIbatLimit(const Sc8815 &chip, float current_a);
bool sc8815SetVinreg(const Sc8815 &chip, std::uint16_t voltage_mv);
bool sc8815ReadVbusVoltage(const Sc8815 &chip, std::uint16_t &voltage_mv);
bool sc8815ReadVbatVoltage(const Sc8815 &chip, std::uint16_t &voltage_mv);
bool sc8815ReadIbusCurrent(const Sc8815 &chip, float &current_a);
bool sc8815ReadIbatCurrent(const Sc8815 &chip, float &current_a);
bool sc8815ReadAdinVoltage(const Sc8815 &chip, std::uint16_t &voltage_mv);
bool sc8815GetStatus(const Sc8815 &chip, Sc8815Status &status);
bool sc8815SetIrqMask(const Sc8815 &chip, Sc8815IrqMaskConfig config);
