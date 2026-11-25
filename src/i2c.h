#pragma once

#include <cstdint>
#include <span>

/*
* 初始化 I²C 总线及两个设备。
*/
void i2cInitialize();

/*
* 按读写缓冲区执行探测、读写或复合 I²C 事务。
*/
bool i2cTransfer(std::uint8_t address, std::span<const std::uint8_t> write,
                  std::span<std::uint8_t> read);
