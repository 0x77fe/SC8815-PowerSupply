#pragma once

// 在共享总线上初始化板载 SSD1306。
bool screenInitialize();

// 同步显示启动画面，并返回传输结果。
bool screenShowStartup();
