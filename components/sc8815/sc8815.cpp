#include "sc8815.h"

#include <algorithm>
#include <array>
#include <cmath>

bool sc8815Begin(const Sc8815 &chip)
{
    return chip.Transfer(chip.Address, {}, {});
}

/*
* 从指定地址连续写入寄存器。
*/
static bool sc8815WriteRegisters(const Sc8815 &chip, Sc8815Register first, std::span<const std::uint8_t> data)
{
    std::array<std::uint8_t, 3> bytes{static_cast<std::uint8_t>(first)};
    if (data.size() > bytes.size() - 1) return false;

    std::ranges::copy(data, bytes.begin() + 1);
    return chip.Transfer(chip.Address, std::span{bytes}.first(data.size() + 1), {});
}

/*
* 从指定地址连续读取寄存器。
*/
static bool sc8815ReadRegisters(const Sc8815 &chip, Sc8815Register first, std::span<std::uint8_t> data)
{
    const std::array address{static_cast<std::uint8_t>(first)};
    return chip.Transfer(chip.Address, std::span{address}, data);
}

/*
* 写入单字节寄存器。
*/
static bool sc8815WriteByte(const Sc8815 &chip, Sc8815Register reg, std::uint8_t data)
{
    const std::array<std::uint8_t, 1> bytes{data};
    return sc8815WriteRegisters(chip, reg, std::span<const std::uint8_t>{bytes});
}

bool sc8815ReadRegister(const Sc8815 &chip, Sc8815Register reg, std::uint8_t &data)
{
    std::array<std::uint8_t, 1> bytes{};
    if (!sc8815ReadRegisters(chip, reg, std::span<std::uint8_t>{bytes})) return false;

    data = bytes[0];
    return true;
}

/*
* 保留掩码指定的位并更新其余配置位。
*/
static bool sc8815WriteMasked(const Sc8815 &chip, Sc8815Register reg, std::uint8_t preserve_mask, std::uint8_t data)
{
    std::uint8_t current;
    return sc8815ReadRegister(chip, reg, current) &&
           sc8815WriteByte(chip, reg, (current & preserve_mask) | (data & ~preserve_mask));
}

bool sc8815ConfigureBatteryCharging(const Sc8815 &chip, Sc8815VbatSetConfig config)
{
    // 寄存器位布局 (表 1):
    //   [7:6] IRCOMP    (00=0mΩ, 01=20mΩ, 10=40mΩ, 11=80mΩ)
    //   [5]   VBAT_SEL  (0=内部设定, 1=外部设定)
    //   [4:3] CSEL      (00=1S, 01=2S, 10=3S, 11=4S)
    //   [2:0] VCELL_SET (000=4.10V ... 111=4.50V)
    std::uint8_t reg = (static_cast<std::uint8_t>(config.Ircomp)  << 6) |
                  (static_cast<std::uint8_t>(config.Sel)     << 5) |
                  (static_cast<std::uint8_t>(config.Csel)    << 3) |
                  (static_cast<std::uint8_t>(config.Vcell)   );
    return sc8815WriteByte(chip, SC8815_REG_VBAT_SET, reg);
}

/*
* 将参考电压换算为寄存器编码并写入。
*/
static bool sc8815WriteVbusReference(const Sc8815 &chip, Sc8815Register high_reg, Sc8815Register low_reg, std::uint16_t ref_mv)
{
    // 将参考电压编码为高 8 位和低 2 位。
    const std::uint16_t code = (ref_mv / 2u - 1u) & 0x03FFu;
    std::uint8_t low_byte;
    if (!sc8815ReadRegister(chip, low_reg, low_byte)) return false;

    // 在一次事务中按地址顺序写入两个字节。
    const std::uint8_t data[] =
    {
        static_cast<std::uint8_t>(code >> 2),
        static_cast<std::uint8_t>((low_byte & 0x3F) | ((code & 0x03) << 6))
    };
    return sc8815WriteRegisters(chip, high_reg, data);
}

bool sc8815SetVbusRefI(const Sc8815 &chip, std::uint16_t ref_mv)
{
    return sc8815WriteVbusReference(chip, SC8815_REG_VBUSREF_I_SET, SC8815_REG_VBUSREF_I_SET2, ref_mv);
}

bool sc8815SetVbusRefE(const Sc8815 &chip, std::uint16_t ref_mv)
{
    // 外部参考电压的推荐工作范围为 0.7 V 至 2.048 V。
    return sc8815WriteVbusReference(chip, SC8815_REG_VBUSREF_E_SET, SC8815_REG_VBUSREF_E_SET2, ref_mv);
}

/*
* 换算并校验限流编码，再写入寄存器。
*/
static bool sc8815WriteCurrentLimit(const Sc8815 &chip, Sc8815Register reg, float current_a, float ratio, float sense_mohm)
{
    // 电流换算要求有效的采样电阻。
    if (sense_mohm == 0.0f || !std::isfinite(sense_mohm)) return false;

    const float code = ((current_a * 256.0f * sense_mohm) / (ratio * 10.0f) - 1.0f) + 0.5f;

    // 转换向零截断，只有截断结果可用 std::uint8_t 表示时才执行。
    if (!std::isfinite(code) || code <= -1.0f || code >= 256.0f) return false;

    return sc8815WriteByte(chip, reg, static_cast<std::uint8_t>(code));
}

bool sc8815SetIbusLimit(const Sc8815 &chip, float current_a)
{
    // 使用芯片当前的比例配置。
    std::uint8_t config;
    if (!sc8815ReadRegister(chip, SC8815_REG_RATIO, config)) return false;

    // 数据手册公式 (表 6):
    //   IBUS_LIM(A) = (IBUS_LIM_SET+1)/256 × IBUS_RATIO × 10mΩ/RS1
    //
    Sc8815IbusRatio ibus_ratio =
        static_cast<Sc8815IbusRatio>((config >> 2) & 0b11);

    // 公式反解:
    //   reg_val = I_LIM × 256 × RS1 / (IBUS_RATIO × 10) - 1
    float ratio_val = (ibus_ratio == SC8815_IBUS_RATIO_3x) ? 3.0f : 6.0f;
    return sc8815WriteCurrentLimit(chip, SC8815_REG_IBUS_LIM_SET, current_a, ratio_val, chip.Rs1MOhm);
}

bool sc8815SetIbatLimit(const Sc8815 &chip, float current_a)
{
    // 使用芯片当前的比例配置。
    std::uint8_t config;
    if (!sc8815ReadRegister(chip, SC8815_REG_RATIO, config)) return false;

    // 数据手册公式 (表 7):
    //   IBAT_LIM(A) = (IBAT_LIM_SET+1)/256 × IBAT_RATIO × 10mΩ/RS2
    Sc8815IbatRatio ibat_ratio =
        static_cast<Sc8815IbatRatio>((config >> 4) & 0b1);

    float ratio_val = (ibat_ratio == SC8815_IBAT_RATIO_6x) ? 6.0f : 12.0f;
    return sc8815WriteCurrentLimit(chip, SC8815_REG_IBAT_LIM_SET, current_a, ratio_val, chip.Rs2MOhm);
}

bool sc8815SetVinreg(const Sc8815 &chip, std::uint16_t voltage_mv)
{
    // 使用芯片当前的比例配置。
    std::uint8_t config;
    if (!sc8815ReadRegister(chip, SC8815_REG_CTRL0_SET, config)) return false;

    // 数据手册公式 (表 8):
    //   VINREG = (VINREG_SET + 1) × VINREG_RATIO (mV)
    //   VINREG_RATIO = 40x  → 40mV/步,  最大 10.24V
    //   VINREG_RATIO = 100x → 100mV/步, 最大 25.6V

    Sc8815VinregRatio vinreg_ratio = static_cast<Sc8815VinregRatio>((config >> 4) & 0b1);

    std::uint16_t step = (vinreg_ratio == SC8815_VINREG_RATIO_40x) ? 40 : 100;
    const std::uint8_t code = static_cast<std::uint8_t>(voltage_mv / step - 1u);
    return sc8815WriteByte(chip, SC8815_REG_VINREG_SET, code);
}

bool sc8815ConfigureMeasurementAndLimitRatios(const Sc8815 &chip, Sc8815RatioConfig config)
{
    // 寄存器位布局 (表 9):
    //   [7:6] 保留位 (保持原值)
    //   [5]   保留位 (保持原值, POR=1)
    //   [4]   IBAT_RATIO  (0=6x, 1=12x)
    //   [3:2] IBUS_RATIO  (01=6x, 10=3x; 00/11=禁止)
    //   [1]   VBAT_MON_RATIO (0=12.5x, 1=5x)
    //   [0]   VBUS_RATIO  (0=12.5x, 1=5x)
    //
    std::uint8_t reg = 0;

    // 写入新的配置位
    reg |= (static_cast<std::uint8_t>(config.IbatRatio)      << 4);          // 第 4 位
    reg |= (static_cast<std::uint8_t>(config.IbusRatio)      << 2);          // 第 3～2 位
    reg |= (static_cast<std::uint8_t>(config.VbatMonRatio)  << 1);      // 第 1 位
    reg |= static_cast<std::uint8_t>(config.VbusRatio);                 // 第 0 位

    return sc8815WriteMasked(chip, SC8815_REG_RATIO, 0xE0, reg);
}

bool sc8815ConfigureModeAndSwitching(const Sc8815 &chip, Sc8815ModeAndSwitchingConfig config)
{
    // 寄存器位布局 (表 10):
    //   [7]   EN_OTG        (0=充电模式, 1=放电模式)
    //   [6:5] 保留位      (保持原值, 禁止覆写)
    //   [4]   VINREG_RATIO  (0=100x, 1=40x)
    //   [3:2] FREQ_SET      (00=150kHz, 01=300kHz, 10=300kHz, 11=450kHz)
    //   [1:0] DT_SET        (00=20ns, 01=40ns, 10=60ns, 11=80ns)
    //
    std::uint8_t reg = 0;

    // 设置可配置位
    reg |= (static_cast<std::uint8_t>(config.EnOtg)         << 7)|      // 第 7 位
           (static_cast<std::uint8_t>(config.VinregRatio)   << 4)|      // 第 4 位
           (static_cast<std::uint8_t>(config.Freq)           << 2)|      // 第 3～2 位
           (static_cast<std::uint8_t>(config.Dt)                 );      // 第 1～0 位

    return sc8815WriteMasked(chip, SC8815_REG_CTRL0_SET, 0x60, reg);
}

bool sc8815ConfigureChargingAndFeedback(const Sc8815 &chip, Sc8815ChargingAndFeedbackConfig config)
{
    // 寄存器位布局 (表 11):
    //   [7]   ICHAR_SEL     (0=IBUS 电流基准, 1=IBAT 电流基准)
    //   [6]   DIS_TRICKLE   (0=启用涓流充电, 1=禁用涓流充电)
    //   [5]   DIS_TERM      (0=启用自动终止, 1=禁用自动终止)
    //   [4]   FB_SEL        (0=内部 I2C 设定 VBUS, 1=外部 FB 电阻设定)
    //   [3]   TRICKLE_SET   (0=70% VBAT 阈值, 1=60% VBAT 阈值)
    //   [2]   DIS_OVP       (0=启用 OVP 保护, 1=禁用 OVP 保护)
    //   [1:0] 保留位      (保持原值, 第 0 位 POR=1)
    //
    std::uint8_t reg = 0;

    // 设置可配置位
    reg |= (static_cast<std::uint8_t>(config.IcharSel)      << 7)|      // 第 7 位
           (static_cast<std::uint8_t>(config.DisTrickle)    << 6)|      // 第 6 位
           (static_cast<std::uint8_t>(config.DisTerm)       << 5)|      // 第 5 位
           (static_cast<std::uint8_t>(config.FbSel)         << 4)|      // 第 4 位
           (static_cast<std::uint8_t>(config.TrickleSet)    << 3)|      // 第 3 位
           (static_cast<std::uint8_t>(config.DisOvp)        << 2);      // 第 2 位

    return sc8815WriteMasked(chip, SC8815_REG_CTRL1_SET, 0x03, reg);
}

bool sc8815ConfigureDitheringAndSlewRate(const Sc8815 &chip, Sc8815DitheringAndSlewRateConfig config)
{
    // 寄存器位布局 (表 12):
    //   [7:4] 保留位  (保持原值, 禁止覆写, POR=0000)
    //   [3]   FACTORY   (0=默认, 1=上电后 MCU 必须置 1)
    //   [2]   EN_DITHER (0=禁用频率抖动/PGATE 正常功能, 1=使能频率抖动)
    //   [1:0] SLEW_SET  (00=1mV/µs, 01=2mV/µs, 10=4mV/µs, 11=8mV/µs)
    //
    std::uint8_t reg = 0;

    // 设置可配置位
    reg |= (static_cast<std::uint8_t>(config.FactoryBit) << 3)|      // 第 3 位: FACTORY (默认=1)
           (static_cast<std::uint8_t>(config.EnDither)   << 2)|      // 第 2 位: EN_DITHER
           (static_cast<std::uint8_t>(config.Slew)            );      // 第 1～0 位: SLEW_SET

    return sc8815WriteMasked(chip, SC8815_REG_CTRL2_SET, 0xF0, reg);
}

bool sc8815ConfigurePowerPathAndMonitoring(const Sc8815 &chip, Sc8815PowerPathAndMonitoringConfig config)
{
    // 寄存器位布局 (表 13):
    //   [7] EN_PGATE           (0=PGATE输出高电平关断PMOS, 1=PGATE输出低电平导通PMOS)
    //   [6] GPO_CTRL           (0=开漏输出需外部上拉, 1=内部下拉输出低电平)
    //   [5] AD_START           (0=停止ADC, 1=启动ADC)
    //   [4] ILIM_BW_SEL        (0=5kHz带宽, 1=1.25kHz带宽)
    //   [3] LOOP_SET           (0=正常环路响应, 1=改善环路响应)
    //   [2] DIS_ShortFoldBack  (0=启用短路电流折返, 1=禁用短路电流折返)
    //   [1] EOC_SET            (0=充电电流1/25终止, 1=充电电流1/10终止)
    //   [0] EN_PFM             (0=强制PWM模式, 1=使能PFM模式)
    //
    // 该寄存器无 保留位, 全部 8 位均可直接写入
    std::uint8_t reg = 0;
    reg |= (static_cast<std::uint8_t>(config.EnPgate)            << 7)|      // 第 7 位
           (static_cast<std::uint8_t>(config.GpoCtrl)            << 6)|      // 第 6 位
           (static_cast<std::uint8_t>(config.AdStart)            << 5)|      // 第 5 位
           (static_cast<std::uint8_t>(config.IlimBw)             << 4)|      // 第 4 位
           (static_cast<std::uint8_t>(config.LoopSet)            << 3)|      // 第 3 位
           (static_cast<std::uint8_t>(config.DisShortFoldback)  << 2)|      // 第 2 位
           (static_cast<std::uint8_t>(config.EocSet)             << 1)|      // 第 1 位
           (static_cast<std::uint8_t>(config.EnPfm)              << 0);      // 第 0 位
    return sc8815WriteByte(chip, SC8815_REG_CTRL3_SET, reg);
}

/*
* 拼接 ADC 高低位并返回换算步数。
*/
static bool sc8815ReadAdc(const Sc8815 &chip, Sc8815Register high_reg, std::uint16_t &steps)
{
    std::uint8_t data[2];
    if (!sc8815ReadRegisters(chip, high_reg, data)) return false;
    steps = (data[0] << 2) + (data[1] >> 6) + 1;
    return true;
}

bool sc8815ReadVbusVoltage(const Sc8815 &chip, std::uint16_t &voltage_mv)
{
    // 使用芯片当前的比例配置。
    std::uint8_t config;
    if (!sc8815ReadRegister(chip, SC8815_REG_RATIO, config)) return false;

    // 数据手册公式 (表 14/15):
    //   VBUS = (4 × VBUS_FB_VALUE + VBUS_FB_VALUE2 + 1) × VBUS_RATIO × 2mV
    //
    // VBUS_RATIO: 12.5x → 25mV/步, 5x → 10mV/步
    std::uint16_t steps;
    if (!sc8815ReadAdc(chip, SC8815_REG_VBUS_FB_VALUE, steps)) return false;

    Sc8815VbusRatio ratio = static_cast<Sc8815VbusRatio>(config & 0x01);

    if (ratio == SC8815_VBUS_RATIO_12_5x)
    {
        voltage_mv = steps * 25;  // 2mV × 12.5 = 25mV
    }
    else
    {
        voltage_mv = steps * 10;  // 2mV × 5 = 10mV
    }
    return true;
}

bool sc8815ReadVbatVoltage(const Sc8815 &chip, std::uint16_t &voltage_mv)
{
    // 使用芯片当前的比例配置。
    std::uint8_t config;
    if (!sc8815ReadRegister(chip, SC8815_REG_RATIO, config)) return false;

    // 数据手册公式 (表 16/17):
    //   VBAT = (4 × VBAT_FB_VALUE + VBAT_FB_VALUE2 + 1) × VBAT_MON_RATIO × 2mV
    std::uint16_t steps;
    if (!sc8815ReadAdc(chip, SC8815_REG_VBAT_FB_VALUE, steps)) return false;

    bool mon_5x = (config & 0x02) != 0;

    std::uint16_t step = mon_5x ? 10 : 25;  // 2mV×5=10mV, 2mV×12.5=25mV
    voltage_mv = steps * step;
    return true;
}

bool sc8815ReadIbusCurrent(const Sc8815 &chip, float &current_a)
{
    // 使用芯片当前的比例配置。
    std::uint8_t config;
    if (!sc8815ReadRegister(chip, SC8815_REG_RATIO, config)) return false;

    // 数据手册公式 (表 18/19):
    //   IBUS(A) = (4 × IBUS_VALUE + IBUS_VALUE2 + 1) × 2mV × IBUS_RATIO × 10mΩ / (1200 × RS1)
    std::uint16_t steps;
    if (!sc8815ReadAdc(chip, SC8815_REG_IBUS_VALUE, steps)) return false;

    Sc8815IbusRatio ibus_ratio =
        static_cast<Sc8815IbusRatio>((config >> 2) & 0x03);

    std::uint16_t ratio_val = (ibus_ratio == SC8815_IBUS_RATIO_3x) ? 3 : 6;

    // IBUS(A) = steps × (2/1200) × ratio_val × 10/RS1
    if (chip.Rs1MOhm == 0.0f || !std::isfinite(chip.Rs1MOhm)) return false;

    const float measured_a = steps * (2.0f / 1200.0f) * ratio_val * 10.0f / chip.Rs1MOhm;
    if (!std::isfinite(measured_a)) return false;
    current_a = measured_a;
    return true;
}

bool sc8815ReadIbatCurrent(const Sc8815 &chip, float &current_a)
{
    // 使用芯片当前的比例配置。
    std::uint8_t config;
    if (!sc8815ReadRegister(chip, SC8815_REG_RATIO, config)) return false;

    // 数据手册公式 (表 20/21):
    //   IBAT(A) = (4 × IBAT_VALUE + IBAT_VALUE2 + 1) × 2mV × IBAT_RATIO × 10mΩ / (1200 × RS2)
    std::uint16_t steps;
    if (!sc8815ReadAdc(chip, SC8815_REG_IBAT_VALUE, steps)) return false;

    Sc8815IbatRatio ibat_ratio =
        static_cast<Sc8815IbatRatio>((config >> 4) & 0x01);

    std::uint16_t ratio_val = (ibat_ratio == SC8815_IBAT_RATIO_6x) ? 6 : 12;
    if (chip.Rs2MOhm == 0.0f || !std::isfinite(chip.Rs2MOhm)) return false;

    const float measured_a = steps * (2.0f / 1200.0f) * ratio_val * 10.0f / chip.Rs2MOhm;
    if (!std::isfinite(measured_a)) return false;
    current_a = measured_a;
    return true;
}

bool sc8815ReadAdinVoltage(const Sc8815 &chip, std::uint16_t &voltage_mv)
{
    // 数据手册公式 (表 22/23):
    //   ADIN = (4 × ADIN_VALUE + ADIN_VALUE2 + 1) × 2mV
    std::uint16_t steps;
    if (!sc8815ReadAdc(chip, SC8815_REG_ADIN_VALUE, steps)) return false;

    voltage_mv = steps * 2;  // 2mV/步
    return true;
}

bool sc8815GetStatus(const Sc8815 &chip, Sc8815Status &status)
{
    std::uint8_t reg;
    if (!sc8815ReadRegister(chip, SC8815_REG_STATUS, reg)) return false;
    status.AcOk      = (reg & 0x40) != 0;
    status.Indet     = (reg & 0x20) != 0;
    status.VbusShort = (reg & 0x08) != 0;
    status.Otp       = (reg & 0x04) != 0;
    status.Eoc       = (reg & 0x02) != 0;
    return true;
}

bool sc8815SetIrqMask(const Sc8815 &chip, Sc8815IrqMaskConfig config)
{
    // 寄存器位布局:
    //   [7] 保留位  (保持原值, POR=1)
    //   [6] AC_OK_Mask
    //   [5] INDET_Mask
    //   [4] 保留位  (保持原值)
    //   [3] VBUS_SHORT_Mask
    //   [2] OTP_Mask
    //   [1] EOC_Mask
    //   [0] 保留位  (保持原值, POR=0, 上电后将此位置 1)
    //
    std::uint8_t reg = 0;

    reg |= ((config.AcOkMask      ? 1 : 0) << 6)|  // 第 6 位
           ((config.IndetMask      ? 1 : 0) << 5)|  // 第 5 位
           ((config.VbusShortMask ? 1 : 0) << 3)|  // 第 3 位
           ((config.OtpMask        ? 1 : 0) << 2)|  // 第 2 位
           ((config.EocMask        ? 1 : 0) << 1)|  // 第 1 位
             1;                                      // 第 0 位

    return sc8815WriteMasked(chip, SC8815_REG_MASK, 0x90, reg);
}
