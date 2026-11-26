# SC8815 可调电源固件

用 ESP32-C3 控制 SC8815 升降压输出。用旋转编码器调节设定，用 SSD1306 查看读数。

## PCB 制板文件

下载对应板卡的 Gerber 压缩包。

- [功率级 PCB](https://github.com/0x77fe/SC8815-PowerSupply/releases/download/v1.0.0/Gerber_PCB_PowerStage.zip)
- [主控 PCB](https://github.com/0x77fe/SC8815-PowerSupply/releases/download/v1.0.0/Gerber_PCB_MainController.zip)

## 烧录发布版

1. 从 [GitHub Releases](https://github.com/0x77fe/SC8815-PowerSupply/releases/latest) 下载三个镜像和 `SHA256SUMS.txt`。
2. 核对镜像的 SHA-256。连接 4 MB Flash 的 ESP32-C3。
3. 安装 esptool 5。将 `PORT` 换成板卡串口。在镜像目录执行以下命令。

```sh
esptool --chip esp32c3 --port PORT write-flash --flash-mode dio --flash-freq 40m --flash-size 4MB 0x0 bootloader.bin 0x8000 partitions.bin 0x10000 firmware.bin
```

## 构建源码

安装 PlatformIO。构建并烧录。以 115200 波特率查看日志。

```sh
pio run -e esp32-c3-devkitc-02
pio run -e esp32-c3-devkitc-02 -t upload
pio device monitor -e esp32-c3-devkitc-02
```

## 操作

- 短按并松开，切换输出开关。
- 静止按住 1 秒，切换电压或限流调节项。
- 旋转旋钮，调节高亮项。
- 按住旋转，每步调节 10 W 功率上限。
- 停止调节后等待约 2.5 秒，让固件保存设定。

查阅[固件文档](docs/firmware.md)了解接线、任务、设定和 Flash 布局。

## 许可证

项目自有代码采用 [GNU GPL v3.0 only](LICENSE)。随附的 [Espressif Preferences](components/preferences/LICENSE) 使用 Apache-2.0。
