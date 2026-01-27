#pragma once

#define AUDIO_ENCODER_WS GPIO_NUM_17
#define AUDIO_ENCODER_CLK GPIO_NUM_16
#define AUDIO_ENCODER_SD GPIO_NUM_15

#define AUDIO_ENCODER_MD0 GPIO_NUM_14
#define AUDIO_ENCODER_MD1 GPIO_NUM_13

#define AUDIO_ENCODER_POLLING_CYCLE 4 // ms 决定了麦克风的延迟
#define AUDIO_ENCODER_RATE 192000     // 最大频率
#define AUDIO_ENCODER_BIT 32          // 固定的比特数
#define AUDIO_ENCODER_CHANNEL 2       // 固定的通道数

// 自动增益
#define AGC_GAIN_TARGET -30       // 目标增益 dB
#define AGC_GAIN_PEAK -5          // 峰值不超过多少 dB
#define AGC_GAIN_MAX 50           // 最大增益 dB
#define AGC_GAIN_MIN 0            // 最小增益 dB
#define AGC_SPEED_ATTACK 0.005f   // 快增加
#define AGC_SPEED_RELEASE 0.0025f // 慢减少

namespace audio::encoder
{
  void setup();
  void on();
  void off();
  bool isOn();

  // rate -> gain -> channel -> bit
  void setRate(uint32_t rate);
  void setBit(uint32_t bit);
  void setChannel(uint8_t channel);
  // 设置自动增益
  void setAuto(bool on);
  // 设置自动降低增益
  void setPeek(bool on);
  // 设置增益(dB)
  void setGain(int8_t db);

  // 默认采用 Linear phase filters ，可以设置为 Low latency filters
  void setLowLatencyFilter(bool on);
  // 设置 Dynamic Range Enhancer
  void setDRE(bool on);
}