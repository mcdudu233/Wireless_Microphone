# 无线领夹式麦克风-发射器 (Wireless Microphone Transmitter)

本文件夹是开源无线领夹式麦克风的发射器的代码，完整开源项目见[根目录](https://github.com/mcdudu233/Wireless_Microphone.git)。

发射器主要用于采集和传输音频，因此代码主要实现了用 I2S 采集 PCM1822 的音频数据，并用 ESP32-S3 自带的 WIFI 或 BLE 传输无损的音频数据。音频数据采用了缓冲区进行储存，并用代码完成录制频率、比特数和通道数等等参数的调整。受制于硬件限制，发射器最高传输 192kHZ(32bit) 的无损麦克风音频数据。

## 目录结构

本目录下：

- [components](https://github.com/mcdudu233/Wireless_Microphone/blob/main/Transmitter/components) --> 依赖组件目录
- [config](https://github.com/mcdudu233/Wireless_Microphone/blob/main/Transmitter/config) --> 配置目录
  - [board.json](https://github.com/mcdudu233/Wireless_Microphone/blob/main/Transmitter/config/board.json) --> 板子配置
  - [partition.csv](https://github.com/mcdudu233/Wireless_Microphone/blob/main/Transmitter/config/partition.csv) --> 分区表配置
- [include](https://github.com/mcdudu233/Wireless_Microphone/blob/main/Transmitter/include) --> 头文件目录
- [src](https://github.com/mcdudu233/Wireless_Microphone/blob/main/Transmitter/src) --> 源代码目录
- [test](https://github.com/mcdudu233/Wireless_Microphone/blob/main/Transmitter/test) --> 测试文件目录 (无)
- [lib](https://github.com/mcdudu233/Wireless_Microphone/blob/main/Transmitter/lib) --> 库文件目录 (无)
- [.gitignore](https://github.com/mcdudu233/Wireless_Microphone/blob/main/Transmitter/.gitignore) --> Git 忽略文件
- [CMakeLists.txt](https://github.com/mcdudu233/Wireless_Microphone/blob/main/Transmitter/CMakeLists.txt) --> CMake 配置文件 (不需要动)
- [platformio.ini](https://github.com/mcdudu233/Wireless_Microphone/blob/main/Transmitter/platformio.ini) --> PlatformIO 配置文件 (框架, 依赖库, 编译标志)
- [LICENSE](https://github.com/mcdudu233/Wireless_Microphone/blob/main/Transmitter/LICENSE) --> 使用协议
- [README.md](https://github.com/mcdudu233/Wireless_Microphone/blob/main/Transmitter/README.md) --> 介绍

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