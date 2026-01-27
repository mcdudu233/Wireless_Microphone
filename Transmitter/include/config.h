#pragma once

#include "cinttypes"

// 配置文件名
#define CONFIG_NAME "config"
#define CONFIG_DATA_NAME "config"
#define CONFIG_VERSION_NAME "version"
#define CONFIG_VERSION_VALUE 0x0009 // 前两位大版本号 后两位小版本号

#define TASK_SYSTEM_CORE 1
#define TASK_SYSTEM_PERIOD 1000
#define TASK_SYSTEM_PRIORITY 1
#define TASK_SYSTEM_STACK 3072

#define TASK_POWER_CORE 1
#define TASK_POWER_PERIOD 10
#define TASK_POWER_PRIORITY 10
#define TASK_POWER_STACK 3072

#define TASK_AUDIO_ENCODER_CORE 1
#define TASK_AUDIO_ENCODER_PERIOD 4 // 这里必须和编码器轮询周期一样！
#define TASK_AUDIO_ENCODER_PRIORITY 5
#define TASK_AUDIO_ENCODER_STACK 3072

#define TASK_TUSB_CORE 1
#define TASK_TUSB_PRIORITY 3
#define TASK_TUSB_STACK 4096

#define TASK_RF_CORE 0
#define TASK_RF_PERIOD 1
#define TASK_RF_PRIORITY 8
#define TASK_RF_STACK 3072

namespace config
{
  enum TransmitMode
  {
    TRANSMIT_MODE_BLE = 0,
    TRANSMIT_MODE_WIFI = 1,
  };

  // 全局配置
  struct StatusValue
  {
    // 设备信息
    struct
    {
      bool start = false;
      bool startWiFi = false;
      bool startBLE = true;
      bool mode = false; // true: WiFi模式; false: BLE模式
      char name[32] = "";
      char password[32] = "";
    } device;
    // 音频信息
    struct
    {
      bool start = false;
      uint8_t channel = 2;
      uint32_t rate = 48000;
      uint8_t bit = 16;
      bool autoVolumn = false;
      bool peekVolumn = false;
      uint8_t volumn = 0;
    } audio;
  };
  extern StatusValue status;

  // 储存的配置
  struct ConfigValue
  {
    struct
    {
      bool isCorrected = false;
      float bias = 0;
    } battery;
  };
  extern ConfigValue config;

  void setup();
  void save();
}
