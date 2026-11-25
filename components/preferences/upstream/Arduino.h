#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>

#include "esp_log.h"

// Preferences 通过构造函数和 c_str() 使用 Arduino 字符串类型。
using String = std::string;

// 将 Preferences 诊断信息发送到 ESP-IDF 控制台。
#define log_e(...) ESP_LOGE("Preferences", __VA_ARGS__)
#define log_v(...) ESP_LOGV("Preferences", __VA_ARGS__)
