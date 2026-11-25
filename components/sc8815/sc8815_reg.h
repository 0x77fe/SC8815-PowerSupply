#ifndef SC8815_REG_H
#define SC8815_REG_H

#include <cstdint>

// 寄存器地址 (共 25 个寄存器, 地址 0x00~0x19, 0x18 保留)
enum Sc8815Register : std::uint8_t
{
    SC8815_REG_VBAT_SET            = 0x00,  ///< VBAT 充电目标电压 / 电池配置
    SC8815_REG_VBUSREF_I_SET       = 0x01,  ///< VBUS 内部参考电压 [9:2] (FB_SEL=0)
    SC8815_REG_VBUSREF_I_SET2      = 0x02,  ///< VBUS 内部参考电压 [1:0] 位于寄存器 [7:6]，低 6 位保留
    SC8815_REG_VBUSREF_E_SET       = 0x03,  ///< VBUS 外部参考电压 [9:2] (FB_SEL=1)
    SC8815_REG_VBUSREF_E_SET2      = 0x04,  ///< VBUS 外部参考电压 [1:0] 位于寄存器 [7:6]，低 6 位保留
    SC8815_REG_IBUS_LIM_SET        = 0x05,  ///< IBUS 电流限制 (充/放电均生效)
    SC8815_REG_IBAT_LIM_SET        = 0x06,  ///< IBAT 电流限制 (充/放电均生效)
    SC8815_REG_VINREG_SET          = 0x07,  ///< VINREG 动态电源管理参考电压
    SC8815_REG_RATIO               = 0x08,  ///< 电流/电压比率配置 (含 保留位)
    SC8815_REG_CTRL0_SET           = 0x09,  ///< 控制0: EN_OTG, VINREG_RATIO, FREQ, DT
    SC8815_REG_CTRL1_SET           = 0x0A,  ///< 控制1: FB_SEL, DIS_OVP, 充电相关
    SC8815_REG_CTRL2_SET           = 0x0B,  ///< 控制2: FACTORY, EN_DITHER, SLEW
    SC8815_REG_CTRL3_SET           = 0x0C,  ///< 控制3: AD_START, EN_PFM, 保护配置
    SC8815_REG_VBUS_FB_VALUE       = 0x0D,  ///< VBUS ADC 值 [9:2]
    SC8815_REG_VBUS_FB_VALUE2      = 0x0E,  ///< VBUS ADC 值 [1:0] 位于寄存器 [7:6]，低 6 位保留
    SC8815_REG_VBAT_FB_VALUE       = 0x0F,  ///< VBAT ADC 值 [9:2]
    SC8815_REG_VBAT_FB_VALUE2      = 0x10,  ///< VBAT ADC 值 [1:0] 位于寄存器 [7:6]，低 6 位保留
    SC8815_REG_IBUS_VALUE          = 0x11,  ///< IBUS ADC 值 [9:2]
    SC8815_REG_IBUS_VALUE2         = 0x12,  ///< IBUS ADC 值 [1:0] 位于寄存器 [7:6]，低 6 位保留
    SC8815_REG_IBAT_VALUE          = 0x13,  ///< IBAT ADC 值 [9:2]
    SC8815_REG_IBAT_VALUE2         = 0x14,  ///< IBAT ADC 值 [1:0] 位于寄存器 [7:6]，低 6 位保留
    SC8815_REG_ADIN_VALUE          = 0x15,  ///< ADIN ADC 值 [9:2]
    SC8815_REG_ADIN_VALUE2         = 0x16,  ///< ADIN ADC 值 [1:0] 位于寄存器 [7:6]，低 6 位保留
    SC8815_REG_STATUS              = 0x17,  ///< 中断状态 (AC_OK, INDET, VBUS_SHORT, OTP, EOC)
    SC8815_REG_MASK                = 0x19   ///< 中断掩码
};

//  VBAT_SET 寄存器配置位定义

/** @brief IRCOMP: 电池内阻补偿设置, PSTOP 为高电平时设置 (REG_VBAT_SET[7:6]) */
enum Sc8815Ircomp : std::uint8_t
{
    SC8815_IRCOMP_0mOhm   = 0b00,  ///< 补偿 0mΩ (默认)
    SC8815_IRCOMP_20mOhm  = 0b01,  ///< 补偿 20mΩ
    SC8815_IRCOMP_40mOhm  = 0b10,  ///< 补偿 40mΩ
    SC8815_IRCOMP_80mOhm  = 0b11   ///< 补偿 80mΩ
};

/** @brief VBAT_SEL: VBAT 目标电压设定方式, PSTOP 为高电平时设置 (REG_VBAT_SET[5]) */
enum Sc8815VbatSel : std::uint8_t
{
    SC8815_VBAT_SEL_INTERNAL = 0b0,  ///< 内部设定 (通过 CSEL + VCELL_SET)
    SC8815_VBAT_SEL_EXTERNAL = 0b1   ///< 外部电阻分压设定 (VBATS 引脚)
};

/** @brief CSEL: 电池串联节数, PSTOP 为高电平时设置 (REG_VBAT_SET[4:3]) */
enum Sc8815Csel : std::uint8_t
{
    SC8815_CSEL_1S = 0b00,  ///< 1 节电池
    SC8815_CSEL_2S = 0b01,  ///< 2 节电池串联
    SC8815_CSEL_3S = 0b10,  ///< 3 节电池串联
    SC8815_CSEL_4S = 0b11   ///< 4 节电池串联
};

/** @brief VCELL: 单节电池目标电压, PSTOP 为高电平时设置 (REG_VBAT_SET[2:0]) */
enum Sc8815Vcell : std::uint8_t
{
    SC8815_VCELL_4V10 = 0b000,  ///< 4.10V/节
    SC8815_VCELL_4V20 = 0b001,  ///< 4.20V/节 (默认)
    SC8815_VCELL_4V25 = 0b010,  ///< 4.25V/节
    SC8815_VCELL_4V30 = 0b011,  ///< 4.30V/节
    SC8815_VCELL_4V35 = 0b100,  ///< 4.35V/节
    SC8815_VCELL_4V40 = 0b101,  ///< 4.40V/节
    SC8815_VCELL_4V45 = 0b110,  ///< 4.45V/节
    SC8815_VCELL_4V50 = 0b111   ///< 4.50V/节
};

//  RATIO 寄存器配置位定义

/** @brief IBAT_RATIO: IBAT 电流限制/监控比率, PSTOP 为高电平时设置 (REG_RATIO[4]) */
enum Sc8815IbatRatio : std::uint8_t
{
    SC8815_IBAT_RATIO_6x  = 0b0,  ///< 6x: 最大电流范围 ~6A
    SC8815_IBAT_RATIO_12x = 0b1   ///< 12x (默认): 最大电流范围 ~12A (建议 IBAT<10A 时使用)
};

/**
 * @brief IBUS_RATIO: IBUS 电流限制/监控比率, PSTOP 为高电平时设置 (REG_RATIO[3:2])
 * @note 值 00b 和 11b 不允许 (数据手册禁止)
 */
enum Sc8815IbusRatio : std::uint8_t
{
    SC8815_IBUS_RATIO_3x = 0b10,   ///< 3x (默认): 最大电流范围 ~3A
    SC8815_IBUS_RATIO_6x = 0b01    ///< 6x: 最大电流范围 ~6A
};

/** @brief VBAT_MON_RATIO: VBAT 电压监控比率, PSTOP 为高电平时设置 (REG_RATIO[1]) */
enum Sc8815VbatMonRatio : std::uint8_t
{
    SC8815_VBAT_MON_RATIO_12_5x = 0b0,  ///< 12.5x (默认): VBAT 最大约 25.6V
    SC8815_VBAT_MON_RATIO_5x    = 0b1   ///< 5x: VBAT 最大约 10.24V (建议 VBAT<10.24V 时使用)
};

/** @brief VBUS_RATIO: VBUS 电压设定/监控比率, PSTOP 为高电平时设置 (REG_RATIO[0]) */
enum Sc8815VbusRatio : std::uint8_t
{
    SC8815_VBUS_RATIO_12_5x = 0b0,  ///< 12.5x (默认): VBUS 最大约 25.6V
    SC8815_VBUS_RATIO_5x    = 0b1   ///< 5x: VBUS 最大约 10.24V (建议 VBUS<10.24V 时使用)
};

//  CTRL0_SET 寄存器配置位定义

/** @brief EN_OTG: OTG 放电模式使能 (REG_CTRL0[7]) */
enum Sc8815EnOtg : std::uint8_t
{
    SC8815_EN_OTG_DISABLE = 0b0,  ///< 禁用 OTG 放电模式
    SC8815_EN_OTG_ENABLE  = 0b1   ///< 启用 OTG 放电模式
};

/** @brief VINREG_RATIO: VINREG 电压设定/监控比率 (REG_CTRL0[4], 单比特字段) */
enum Sc8815VinregRatio : std::uint8_t
{
    SC8815_VINREG_RATIO_100x = 0b0,  ///< 100x, 100mV/步, 最大 25.6V (默认, POR=0)
    SC8815_VINREG_RATIO_40x  = 0b1   ///< 40x,  40mV/步, 最大 10.24V (建议 VBUS < 12V 时使用)
};

/** @brief FREQ: 开关频率选择 (REG_CTRL0[3:2]) */
enum Sc8815Freq : std::uint8_t
{
    SC8815_FREQ_150kHz = 0b00,  ///< 150kHz
    SC8815_FREQ_300kHz = 0b01,  ///< 300kHz (默认, 值 01 或 10 均为 300kHz)
    SC8815_FREQ_450kHz = 0b11   ///< 450kHz
};

/** @brief DT: 开关死区时间 (REG_CTRL0[1:0]) */
enum Sc8815Dt : std::uint8_t
{
    SC8815_DT_20ns = 0b00,  ///< 20ns (默认)
    SC8815_DT_40ns = 0b01,  ///< 40ns
    SC8815_DT_60ns = 0b10,  ///< 60ns
    SC8815_DT_80ns = 0b11   ///< 80ns
};

//  CTRL1_SET 寄存器配置位定义

/** @brief ICHAR_SEL: 充电电流选择 (REG_CTRL1[7]) */
enum Sc8815IcharSel : std::uint8_t
{
    SC8815_ICHAR_SEL_IBUS = 0b0,  ///< 充电电流由 IBUS 电流值设定 (默认)
    SC8815_ICHAR_SEL_IBAT = 0b1   ///< 充电电流由 IBAT 电流值设定
};

/** @brief DIS_TRICKLE: 禁用涓流充电 (REG_CTRL1[6]) */
enum Sc8815DisTrickle : std::uint8_t
{
    SC8815_DIS_TRICKLE_ENABLE = 0b1,  ///< 禁用涓流充电
    SC8815_DIS_TRICKLE_DISABLE = 0b0   ///< 启用涓流充电 (默认)
};

/** @brief DIS_TERM: 禁用自动充电终止 (REG_CTRL1[5]) */
enum Sc8815DisTerm : std::uint8_t
{
    SC8815_DIS_TERM_ENABLE = 0b1,  ///< 禁用自动充电终止
    SC8815_DIS_TERM_DISABLE = 0b0   ///< 启用自动充电终止 (默认)
};

/** @brief FB_SEL: VBUS 参考电压选择 (REG_CTRL1[4]) */
enum Sc8815FbSel : std::uint8_t
{
    SC8815_FB_SEL_INTERNAL = 0b0,  ///< 使用内部 VBUS 参考电压 (默认)
    SC8815_FB_SEL_EXTERNAL = 0b1   ///< 使用外部 FB 引脚参考电压
};

/** @brief TRICKLE_SET: 涓流充电阈值设置 (REG_CTRL1[3]) */
enum Sc8815TrickleSet : std::uint8_t
{
    SC8815_TRICKLE_SET_70 = 0b0,  ///< VBAT 电压的 70% (默认)
    SC8815_TRICKLE_SET_60 = 0b1   ///< VBAT 电压的 60%
};

/** @brief DIS_OVP: 禁用过压保护 (REG_CTRL1[2]) */
enum Sc8815DisOvp : std::uint8_t
{
    SC8815_DIS_OVP_ENABLE = 0b1,  ///< 禁用过压保护
    SC8815_DIS_OVP_DISABLE = 0b0   ///< 启用过压保护 (默认)
};

//  CTRL2_SET 寄存器配置位定义

/** @brief FACTORY: 工厂配置 (REG_CTRL2[3]) */
enum Sc8815Factory : std::uint8_t
{
    SC8815_FACTORY_DEFAULT = 0b0,  ///< (默认)
    SC8815_FACTORY_POWERUP = 0b1  ///< 上电后必须写为1
};

/** @brief EN_DITHER: 扩频抖动使能 (REG_CTRL2[2]) */
enum Sc8815EnDither : std::uint8_t
{
    SC8815_EN_DITHER_DISABLE = 0b0,  ///< 禁用扩频抖动 (默认)
    SC8815_EN_DITHER_ENABLE = 0b1   ///< 启用扩频抖动
};

/** @brief SLEW_SET: VBUS 动态电压变化斜率 (REG_CTRL2[1:0], 仅放电模式) */
enum Sc8815Slew : std::uint8_t
{
    SC8815_SLEW_SET_1mV_us = 0b00,  ///< 1mV/µs
    SC8815_SLEW_SET_2mV_us = 0b01,  ///< 2mV/µs (默认)
    SC8815_SLEW_SET_4mV_us = 0b10,  ///< 4mV/µs
    SC8815_SLEW_SET_8mV_us = 0b11   ///< 8mV/µs
};

//  CTRL3_SET 寄存器配置位定义

/** @brief EN_PGATE: PGATE 引脚控制 (REG_CTRL3[7], 枚举值=寄存器位值) */
enum Sc8815EnPgate : std::uint8_t
{
    SC8815_EN_PGATE_OFF = 0b0,  ///< PGATE 输出高电平, 关断外部 PMOS (默认, POR=0)
    SC8815_EN_PGATE_ON  = 0b1   ///< PGATE 输出低电平, 导通外部 PMOS
};

/** @brief GPO_CTRL: GPO 引脚控制 (REG_CTRL3[6], 枚举值=寄存器位值) */
enum Sc8815GpoCtrl : std::uint8_t
{
    SC8815_GPO_CTRL_OD = 0b0,  ///< 开漏输出, 需外部上拉 (默认, POR=0)
    SC8815_GPO_CTRL_LO = 0b1   ///< 内部下拉输出低电平
};

/** @brief AD_START: AD转换启动 (REG_CTRL3[5]) */
enum Sc8815AdStart : std::uint8_t
{
    SC8815_AD_START_DISABLE = 0b0,  ///< 禁用AD转换 (默认)
    SC8815_AD_START_ENABLE = 0b1   ///< 启用AD转换
};

/** @brief ILIM_BW_SEL: ILIM 环路带宽选择 (REG_CTRL3[4]) */
enum Sc8815IlimBwSel : std::uint8_t
{
    SC8815_ILIM_BW_SEL_5K    = 0b0,  ///< 5kHz 带宽 (默认, POR=0)
    SC8815_ILIM_BW_SEL_1_25K = 0b1   ///< 1.25kHz 带宽
};

/** @brief LOOP_SET: 环路响应控制 (REG_CTRL3[3]) */
enum Sc8815LoopSet : std::uint8_t
{
    SC8815_LOOP_SET_NORMAL = 0b0,  ///< 正常响应 (默认)
    SC8815_LOOP_SET_IMPROVE = 0b1   ///< 改进响应
};

/** @brief DIS_ShortFoldBack: 禁用短路电流折返保护 (REG_CTRL3[2]) */
enum Sc8815DisShortFoldback : std::uint8_t
{
    SC8815_DIS_SHORT_FOLDBACK_ENABLE  = 0b1,  ///< 禁用短路电流折返保护
    SC8815_DIS_SHORT_FOLDBACK_DISABLE = 0b0   ///< 启用短路电流折返保护 (默认)
};

/** @brief EOC_SET: 充电终止电流阈值 (REG_CTRL3[1]) */
enum Sc8815Eoc : std::uint8_t
{
    SC8815_EOC_SET_1_25 = 0b0,  ///< 充电电流的 1/25
    SC8815_EOC_SET_1_10 = 0b1   ///< 充电电流的 1/10 (默认)
};

/** @brief EN_PFM: 启用 PFM 模式 (REG_CTRL3[0]) */
enum Sc8815EnPfm : std::uint8_t
{
    SC8815_EN_PFM_DISABLE = 0b0,  ///< 禁用 PFM 模式 (默认)
    SC8815_EN_PFM_ENABLE = 0b1   ///< 启用 PFM 模式
};

/** @brief VBAT_SET 寄存器配置结构体 (地址 0x00, PSTOP 为高电平 时可写) */
struct Sc8815VbatSetConfig
{
    Sc8815Ircomp Ircomp;     /**< [7:6] 电池内阻补偿 (IRCOMP) */
    Sc8815VbatSel Sel;      /**< [5]   VBAT 目标电压设定方式 (VBAT_SEL) */
    Sc8815Csel Csel;         /**< [4:3] 电池串联节数 (CSEL) */
    Sc8815Vcell Vcell;       /**< [2:0] 单节电池目标电压 (VCELL) */
};

/** @brief RATIO 寄存器配置结构体 (地址 0x08, PSTOP 为高电平 时可写) */
struct Sc8815RatioConfig
{
    Sc8815IbatRatio IbatRatio;             /**< [4]   IBAT 电流限制/监控比率 (IBAT_RATIO) */
    Sc8815IbusRatio IbusRatio;             /**< [3:2] IBUS 电流限制/监控比率 (IBUS_RATIO), 00/11 禁止 */
    Sc8815VbatMonRatio VbatMonRatio;     /**< [1]   VBAT 电压监控比率 (VBAT_MON_RATIO) */
    Sc8815VbusRatio VbusRatio;             /**< [0]   VBUS 电压设定/监控比率 (VBUS_RATIO) */
};

/** @brief CTRL0_SET 寄存器配置结构体 (地址 0x09) */
struct Sc8815ModeAndSwitchingConfig
{
    Sc8815EnOtg EnOtg;                 /**< [7]   OTG 放电模式使能 (EN_OTG) */
    Sc8815VinregRatio VinregRatio;     /**< [4]   VINREG 电压设定/监控比率 (VINREG_RATIO) */
    Sc8815Freq Freq;                     /**< [3:2] 开关频率选择 (FREQ) */
    Sc8815Dt Dt;                         /**< [1:0] 开关死区时间 (DT) */
};

/** @brief CTRL1_SET 寄存器配置结构体 (地址 0x0A) */
struct Sc8815ChargingAndFeedbackConfig
{
    Sc8815IcharSel IcharSel;           /**< [7] 充电电流选择 (ICHAR_SEL) */
    Sc8815DisTrickle DisTrickle;       /**< [6] 禁用涓流充电 (DIS_TRICKLE) */
    Sc8815DisTerm DisTerm;            /**< [5] 禁用自动充电终止 (DIS_TERM) */
    Sc8815FbSel FbSel;                /**< [4] VBUS 参考电压选择 (FB_SEL) */
    Sc8815TrickleSet TrickleSet;      /**< [3] 涓流充电阈值 (TRICKLE_SET) */
    Sc8815DisOvp DisOvp;              /**< [2] 禁用过压保护 (DIS_OVP) */
};

/** @brief CTRL2_SET 寄存器配置结构体 (地址 0x0B) */
struct Sc8815DitheringAndSlewRateConfig
{
    Sc8815Factory FactoryBit;  /**< [3]   工厂配置, 上电后必须写1 (FACTORY) */
    Sc8815EnDither EnDither;                           /**< [2]   扩频抖动使能 (EN_DITHER) */
    Sc8815Slew Slew;                                     /**< [1:0] VBUS 动态电压变化斜率, 仅放电模式 (SLEW_SET) */
};

/** @brief CTRL3_SET 寄存器配置结构体 (地址 0x0C) */
struct Sc8815PowerPathAndMonitoringConfig
{
    Sc8815EnPgate EnPgate;                         /**< [7] PGATE 引脚控制 (EN_PGATE) */
    Sc8815GpoCtrl GpoCtrl;                         /**< [6] GPO 引脚输出模式 (GPO_CTRL) */
    Sc8815AdStart AdStart;                         /**< [5] AD 转换启动 (AD_START) */
    Sc8815IlimBwSel IlimBw;                       /**< [4] ILIM 环路带宽选择 (ILIM_BW_SEL) */
    Sc8815LoopSet LoopSet;                         /**< [3] 环路响应控制 (LOOP_SET) */
    Sc8815DisShortFoldback DisShortFoldback;     /**< [2] 禁用短路电流折返保护 (DIS_ShortFoldBack) */
    Sc8815Eoc EocSet;                               /**< [1] 充电终止电流阈值 (EOC_SET) */
    Sc8815EnPfm EnPfm;                             /**< [0] PFM 模式使能 (EN_PFM) */
};

/** @brief 状态列表 */
struct Sc8815Status
{
    bool AcOk;
    bool Indet;
    bool VbusShort;
    bool Otp;
    bool Eoc;
};

/** @brief MASK 中断掩码寄存器配置结构体 (地址 0x19) */
struct Sc8815IrqMaskConfig
{
    bool AcOkMask;        /**< [6] AC 适配器接入中断掩码 (AC_OK) */
    bool IndetMask;        /**< [5] USB-A 负载插入检测中断掩码 (INDET) */
    bool VbusShortMask;   /**< [3] VBUS 短路保护中断掩码 (VBUS_SHORT) */
    bool OtpMask;          /**< [2] 过温保护中断掩码 (OTP) */
    bool EocMask;          /**< [1] 充电终止中断掩码 (EOC) */
};

#endif