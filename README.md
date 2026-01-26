# 无线领夹式麦克风 (Wireless Microphone)

![演示](https://github.com/user-attachments/assets/459fc99b-dca7-4327-b93d-d6d94750cb9f)

本项为开源无线领夹式麦克风的代码，完整开源项目见[立创平台](https://oshwhub.com/dudu233/wireless-microphone)。

无线领夹式麦克风分为发射器和接受器。发射器主要用于采集和传输音频，接受器用于接收、输出和录制音频。发射器和接收器都采用ESP32-S3作为主控芯片，采用WIFI(150Mbps)或蓝牙(2Mbps)传输无损的音频数据。发射器支持编码高达192kHZ(32bit)的无损麦克风音频数据；接收器则支持384kHZ(32bit)的音频输出或者作为USB音频设备输出。

## 目录结构



## 下载&编译

项目基于 PlatformIO 开发，使用 Arduino 和 ESP-IDF 双框架。

### 直接下载

所有版本都将会发布在 **[Release](https://github.com/mcdudu233/Wireless_Microphone/releases)** 里，直接下载即可。

固件下载请用 ESP32 官方提供的 FlashDownloadTool 程序下载器，选择“ESP32-S3”打开主界面，将“Receiver.bin”或者“Transmitter.bin”固件写入到地址“0x0”即可。建议“SPI SPEED”为“80MHz”，“SPI MODE”为“QIO”，以实现最大性能。

### 手动编译

1. 首先用 PlatformIO 打开发射器或者接收器的项目

2. 点击界面左下角或者左边的 Build 进行编译即可


## 相关链接

[项目介绍](https://oshwhub.com/dudu233/wireless-microphone)
[固件下载](https://github.com/mcdudu233/Wireless_Microphone/releases)
[我的博客](https://www.mcso.top/)
