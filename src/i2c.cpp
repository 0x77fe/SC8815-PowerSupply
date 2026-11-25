#include "i2c.h"

#include "board_config.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static i2c_master_bus_handle_t bus;
static i2c_master_dev_handle_t converter;
static i2c_master_dev_handle_t display;
static SemaphoreHandle_t bus_mutex;

void i2cInitialize()
{
    bus_mutex = xSemaphoreCreateMutex();
    ESP_ERROR_CHECK(bus_mutex != nullptr ? ESP_OK : ESP_ERR_NO_MEM);

    // 从默认配置出发，设置板级引脚、时钟和上拉。
    i2c_master_bus_config_t config{};
    config.i2c_port = I2C_NUM_0;
    config.sda_io_num = static_cast<gpio_num_t>(board_sda);
    config.scl_io_num = static_cast<gpio_num_t>(board_scl);
    config.clk_source = I2C_CLK_SRC_DEFAULT;
    config.glitch_ignore_cnt = 7;
    config.flags.enable_internal_pullup = true;
    ESP_ERROR_CHECK(i2c_new_master_bus(&config, &bus));

    // 两个设备共用地址长度和时钟频率。
    i2c_device_config_t device{};
    device.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    device.device_address = sc8815_i2c_address;
    device.scl_speed_hz = board_i2c_clock;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &device, &converter));
    device.device_address = board_display_address;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &device, &display));
}

bool i2cTransfer(std::uint8_t address, std::span<const std::uint8_t> write,
                  std::span<std::uint8_t> read)
{
    if (address != sc8815_i2c_address && address != board_display_address) return false;

    // OLED 整页传输在 400 kHz 下约需 3 ms，页间释放总线。
    const bool is_converter = address == sc8815_i2c_address;
    const int timeout_ms = is_converter ? 2 : 5;
    const i2c_master_dev_handle_t device = is_converter ? converter : display;
    xSemaphoreTake(bus_mutex, portMAX_DELAY);
    esp_err_t result;
    if (!write.empty() && !read.empty()) result = i2c_master_transmit_receive(device, write.data(), write.size(), read.data(), read.size(), timeout_ms);
    else if (!write.empty()) result = i2c_master_transmit(device, write.data(), write.size(), timeout_ms);
    else if (!read.empty()) result = i2c_master_receive(device, read.data(), read.size(), timeout_ms);
    else result = i2c_master_probe(bus, address, timeout_ms);
    xSemaphoreGive(bus_mutex);
    return result == ESP_OK;
}
