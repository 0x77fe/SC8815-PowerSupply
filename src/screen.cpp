#include "screen.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <span>
#include <string_view>

#include "board_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c.h"
#include "startup_logo.h"
#include "task_context.h"

// 板载 SSD1306 使用 128×64 显存，每字节保存一列中的 8 个像素。
static constexpr int screen_width = 128;
static constexpr int screen_height = 64;
static constexpr int screen_pages = screen_height / 8;

// 帧缓冲按页和列索引，每字节保存同列的 8 个垂直像素。
static std::uint8_t frame[screen_pages][screen_width];

// 5×7 字形按列储存，覆盖仪表界面的字符。
static constexpr std::string_view glyph_characters = "-.:0123456789ACFIMNOVWX";
static constexpr std::uint8_t glyphs[][5] =
{
    {0x08,0x08,0x08,0x08,0x08}, // -
    {0,0x60,0x60,0,0}, // .
    {0,0x36,0x36,0,0}, // :
    {0x3E,0x51,0x49,0x45,0x3E}, // 0
    {0,0x42,0x7F,0x40,0}, // 1
    {0x42,0x61,0x51,0x49,0x46}, // 2
    {0x21,0x41,0x45,0x4B,0x31}, // 3
    {0x18,0x14,0x12,0x7F,0x10}, // 4
    {0x27,0x45,0x45,0x45,0x39}, // 5
    {0x3C,0x4A,0x49,0x49,0x30}, // 6
    {0x01,0x71,0x09,0x05,0x03}, // 7
    {0x36,0x49,0x49,0x49,0x36}, // 8
    {0x06,0x49,0x49,0x29,0x1E}, // 9
    {0x7E,0x11,0x11,0x11,0x7E}, // A
    {0x3E,0x41,0x41,0x41,0x22}, // C
    {0x7F,0x09,0x09,0x09,0x01}, // F
    {0,0x41,0x7F,0x41,0}, // I
    {0x7F,0x02,0x0C,0x02,0x7F}, // M
    {0x7F,0x04,0x08,0x10,0x7F}, // N
    {0x3E,0x41,0x41,0x41,0x3E}, // O
    {0x1F,0x20,0x40,0x20,0x1F}, // V
    {0x3F,0x40,0x38,0x40,0x3F}, // W
    {0x63,0x14,0x08,0x14,0x63}, // X
};
static_assert(std::size(glyphs) == glyph_characters.size());

bool screenInitialize()
{
    // 设置扫描方向、复用率、电荷泵和面板时序。
    const std::uint8_t commands[] =
    {
        0x00, 0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
        0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12, 0x81,
        0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0x2E, 0xAF
    };
    return i2cTransfer(board_display_address, commands, {});
}

/*
* 按显存页发送 OLED 帧，每页使用一笔数据事务。
*/
static bool screenSendFrame()
{
    // 设置完整的列和页地址窗口。
    const std::uint8_t address[] = {0x00, 0x21, 0, screen_width - 1, 0x22, 0, screen_pages - 1};
    if (!i2cTransfer(board_display_address, address, {})) return false;

    // 每笔数据事务携带控制字节和当前页的 128 列像素。
    std::uint8_t packet[1 + screen_width] = {0x40};
    for (int page = 0; page < screen_pages; ++page)
    {
        std::ranges::copy(frame[page], packet + 1);

        if (!i2cTransfer(board_display_address, packet, {})) return false;
    }
    return true;
}

/*
* 字符和位图通过像素操作写入帧缓冲。
*/
static void drawPixel(int x, int y, bool white)
{
    if (x >= 0 && x < screen_width && y >= 0 && y < screen_height)
    {
        const std::uint8_t bit = 1u << (y % 8);
        std::uint8_t &pixels = frame[y / 8][x];
        if (white) pixels |= bit;
        else pixels &= static_cast<std::uint8_t>(~bit);
    }
}

/*
* 按字模和缩放倍数绘制文本。
*/
static void drawText(int x, int y, std::string_view text, int scale, bool white)
{
    for (const char character : text)
    {
        const auto glyph_index = glyph_characters.find(character);
        if (glyph_index == std::string_view::npos)
        {
            x += 6 * scale;
            continue;
        }

        const auto &glyph = glyphs[glyph_index];
        for (int column = 0; column < 5 * scale; ++column)
        {
            for (int row = 0; row < 8 * scale; ++row)
            {
                if (glyph[column / scale] & (1u << (row / scale))) drawPixel(x + column, y + row, white);
            }
        }
        x += 6 * scale;
    }
}

/*
* 绘制单色位图。
*/
static void drawBitmap(int x, int y, std::span<const std::uint8_t> bitmap, int width, int height)
{
    const int stride = (width + 7) / 8;
    for (int row = 0; row < height; ++row)
    {
        for (int col = 0; col < width; ++col)
        {
            if (bitmap[row * stride + col / 8] & (0x80u >> (col % 8))) drawPixel(x + col, y + row, true);
        }
    }
}

/*
* 为选中的读数行填充反显背景。
*/
static void highlightReadingRow(int y, bool selected)
{
    if (!selected) return;

    // 覆盖整行，包含右侧小字区域。
    for (int row = y - 1; row < y + 16; ++row)
    {
        for (int col = 0; col < screen_width; ++col) drawPixel(col, row, true);
    }
}

/*
* 按屏幕右边距对齐小字。
*/
static void drawSmallRight(int y, std::string_view text, bool white)
{
    drawText(screen_width - 2 - static_cast<int>(text.size()) * 6, y, text, 1, white);
}

/*
* 将定点数格式化为对齐的小数字段。
*/
static std::string_view formatReading(std::span<char> text, int value, unsigned integer_digits,
                                      unsigned decimal_digits, bool zero_pad, char unit)
{
    const unsigned sign = value < 0 ? 1u : 0u;
    unsigned magnitude = value < 0 ? 0u - static_cast<unsigned>(value) : static_cast<unsigned>(value);
    const unsigned digits = integer_digits + (decimal_digits ? 1 + decimal_digits : 0);
    for (unsigned index = digits; index > 0; --index)
    {
        if (decimal_digits && index == integer_digits + 1) text[sign + index - 1] = '.';
        else if (magnitude == 0 && !zero_pad && index < integer_digits) text[sign + index - 1] = ' ';
        else
        {
            text[sign + index - 1] = '0' + magnitude % 10;
            magnitude /= 10;
        }
    }

    // 负读数在数字字段前显示符号。
    if (sign) text[0] = '-';
    text[sign + digits] = unit;
    return {text.data(), sign + digits + 1};
}

void DisplayTask::run(void *)
{
    // 首帧立即绘制，后续按固定周期刷新。
    TickType_t last_wake = xTaskGetTickCount();
    for (;; xTaskDelayUntil(&last_wake, pdMS_TO_TICKS(board_refresh_period_ms)))
    {
        // 持锁读取运行状态并绘制仪表盘。
        xSemaphoreTake(state_mutex, portMAX_DELAY);

        // 大字显示输出电压、电流和功率。
        for (auto &page : frame) std::ranges::fill(page, 0);

        const bool output_power_valid = power_sample.VbusValid && power_sample.IbusValid;
        bool inverted;

        // 大字读数左对齐，右下方以小字显示设定电压。
        std::array<char, 16> reading;
        constexpr std::string_view unavailable = "--";
        inverted = !power_limit_visible && adjust_mode == ADJUST_Voltage;
        highlightReadingRow(1, inverted);
        const auto voltage_reading = power_sample.VbusValid
            ? formatReading(reading, (power_sample.VbusMv + 5u) / 10u, 2, 2, true, 'V')
            : unavailable;
        drawText(2, 1, voltage_reading, 2, !inverted);
        const auto set_voltage = formatReading(reading, (output_settings.VoltageMv + 50u) / 100u, 2, 1, false, 'V');
        drawSmallRight(8, set_voltage, !inverted);

        // 输出电流与限流设定沿用电压行的布局。
        inverted = !power_limit_visible && adjust_mode == ADJUST_Current;
        highlightReadingRow(19, inverted);
        const auto current_reading = power_sample.IbusValid
            ? formatReading(reading, static_cast<int>(std::lround(power_sample.IbusA * 100.0f)), 2, 2, true, 'A')
            : unavailable;
        drawText(2, 19, current_reading, 2, !inverted);
        const auto set_current = formatReading(reading, (output_settings.CurrentMa + 50u) / 100u, 1, 1, false, 'A');
        drawSmallRight(26, set_current, !inverted);

        // 左侧显示输出功率、功率上限或输出状态。
        highlightReadingRow(37, power_limit_visible);
        std::string_view power = unavailable;
        if (power_limit_visible)
        {
            constexpr std::string_view prefix = "MAX";
            std::ranges::copy(prefix, reading.begin());
            const unsigned limit = output_settings.PowerLimitW;
            const auto limit_reading = formatReading(std::span{reading}.subspan(prefix.size()),
                                                    limit, limit >= 100 ? 3 : 2, 0, false, 'W');
            power = {reading.data(), prefix.size() + limit_reading.size()};
        }
        else if (output_state != OUTPUT_Enabled) power = "OFF";
        else if (output_power_valid)
        {
            const float output_power_w = power_sample.VbusMv * 0.001f * power_sample.IbusA;
            power = formatReading(reading, static_cast<int>(std::lround(output_power_w * 10.0f)), 3, 1, true, 'W');
        }
        drawText(2, 37, power, 2, !power_limit_visible);

        // 比较输出读数距已应用设定的相对差值，达到设定时差值为零。
        std::string_view regulation = unavailable;
        if (output_state == OUTPUT_Enabled && has_applied && output_power_valid)
        {
            const float voltage_gap = std::max(0.0f, 1.0f - static_cast<float>(power_sample.VbusMv) / applied_settings.VoltageMv);
            const float current_gap = std::max(0.0f, 1.0f - power_sample.IbusA * 1000.0f / applied_settings.CurrentMa);
            regulation = current_gap < voltage_gap ? "CC" : "CV";
        }
        drawSmallRight(44, regulation, !power_limit_visible);

        // 底行左对齐输入电压，右对齐输入电流。
        std::array<char, 16> input_reading;
        std::string_view input_text = "IN:--V";
        if (power_sample.VbatValid)
        {
            constexpr std::string_view prefix = "IN:";
            std::ranges::copy(prefix, input_reading.begin());
            const auto vbat_reading = formatReading(std::span{input_reading}.subspan(prefix.size()),
                                                   (power_sample.VbatMv + 5u) / 10u, 2, 2, true, 'V');
            input_text = {input_reading.data(), prefix.size() + vbat_reading.size()};
        }

        drawText(2, 57, input_text, 1, true);
        const auto input_current = power_sample.IbatValid
            ? formatReading(reading, static_cast<int>(std::lround(power_sample.IbatA * 100.0f)), 2, 2, false, 'A')
            : std::string_view{"--A"};
        drawSmallRight(57, input_current, true);

        // 释放状态锁后发送 OLED 帧，再等待下一次刷新。
        xSemaphoreGive(state_mutex);
        screenSendFrame();
    }
}

bool screenShowStartup()
{
    // 左侧放置带单像素描边的半色调标志。
    for (auto &page : frame) std::ranges::fill(page, 0);
    const std::int16_t logo_x = 2;
    const std::int16_t logo_right = logo_x + logo_width;
    drawBitmap(logo_x, 2, logo_bitmap, logo_width, logo_height);

    // 两行署名作为整体在标志右侧留白内居中。
    const std::int16_t credit_x = logo_right + (screen_width - logo_right - logo_credit_width) / 2;
    const std::int16_t credit_y = (screen_height - logo_credit_height) / 2;
    drawBitmap(credit_x, credit_y, logo_credit_bitmap,
               logo_credit_width, logo_credit_height);

    // 将启动画面同步提交到 SSD1306。
    return screenSendFrame();
}
