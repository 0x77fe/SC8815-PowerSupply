#pragma once

#include <cstdint>
#include <span>

void i2cInitialize();
bool i2cTransfer(std::uint8_t address, std::span<const std::uint8_t> write,
                  std::span<std::uint8_t> read);
