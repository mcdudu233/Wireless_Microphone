# 无线领夹式麦克风 (Wireless Microphone)

![演示](https://github.com/user-attachments/assets/459fc99b-dca7-4327-b93d-d6d94750cb9f)

本项为开源无线领夹式麦克风的代码，完整开源项目见[立创平台](https://oshwhub.com/dudu233/wireless-microphone)。

无线领夹式麦克风分为发射器和接受器。发射器主要用于采集和传输音频，接受器用于接收、输出和录制音频。发射器和接收器都采用ESP32-S3作为主控芯片，采用WIFI(150Mbps)或蓝牙(2Mbps)传输无损的音频数据。发射器支持编码高达192kHZ(32bit)的无损麦克风音频数据；接收器则支持384kHZ(32bit)的音频输出或者作为USB音频设备输出。

## 目录结构

项目根目录下：

- Transmitter --> 发射器源代码
- Receiver --> 接收器源代码
- LICENSE --> 使用协议
- README.md --> 介绍


## 下载&编译

项目基于 PlatformIO 开发，使用 Arduino 和 ESP-IDF 双框架。

### 直接下载

所有版本都将会发布在 **[Release](https://github.com/mcdudu233/Wireless_Microphone/releases)** 里，直接下载即可。

固件下载请用 ESP32 官方提供的 FlashDownloadTool 程序下载器，选择“ESP32-S3”打开主界面，将“Receiver.bin”或者“Transmitter.bin”固件写入到地址“0x0”即可。建议“SPI SPEED”为“80MHz”，“SPI MODE”为“QIO”，以实现最大性能。

### 手动编译

1. 用 Visual Studio Code 打开 **发射器(Transmitter)** 或者 **接收器(Receiver)** 的项目

   本项目需要使用 **Visual Studio Code** 打开，如没有请先下载安装。

   安装好 Visual Studio Code 后，请在左边界面安装 **PlatformIO** 插件。

   根据提示重新打开这个项目， PlatformIO 会自动下载依赖等等数据，这时候请不要编译或者上传固件。

2. 修改代码 *(如有需要)*

   根据 **README.md** 提供的 **目录结构** ，可以快速理解代码的原理，根据自己需要修改代码。

3. 点击界面左下角或者左边的 Build 进行编译即可

   找到 PlatformIO 自带的 **Build** 按钮编译即可，这时候会自动下载依赖编译程序，如有问题欢迎提交 **Issues** 。


## 相关链接

- [项目介绍](https://oshwhub.com/dudu233/wireless-microphone)
- [固件开源](https://github.com/mcdudu233/Wireless_Microphone.git)
- [固件下载](https://github.com/mcdudu233/Wireless_Microphone/releases)
- [我的博客](https://www.mcso.top/)
