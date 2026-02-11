#pragma once

#include "cinttypes"
#include "module/rf.h"
#include "module/audio/decoder.h"

#define AUDIO_BUFFER_MAX_DATA_SIZE (AUDIO_DECODER_RATE * AUDIO_DECODER_BIT * AUDIO_DECODER_CHANNEL / 8 * AUDIO_DECODER_POLLING_CYCLE / 1000)
#define AUDIO_BUFFER_MAX_BUFFER_SIZE 100

// 传输延迟(等待多久才把数据输出,数值过低会导致爆音)
#define AUDIO_BUFFER_DELAY_PACKET 10                                                      // 延迟多少个数据包
#define AUDIO_BUFFER_DELAY_TIME (AUDIO_BUFFER_DELAY_PACKET * AUDIO_DECODER_POLLING_CYCLE) // 计算得到传输延迟

struct AudioData
{
  uint32_t num;
  uint32_t size;
  uint8_t data[AUDIO_BUFFER_MAX_DATA_SIZE];
};

namespace audio::buffer
{
  void setup();
  // 重置指针
  void restart();

  /* 写入数据 */
  // 写入音频数据
  void writeWiFiPacket(WiFiAudioPacket *packet);
  // void writeBLEPacket(BLEAudioPacket *packet);

  /* 读取数据 */
  // 获取音频解码器的数据
  AudioData *getDecoderData();
  // 获取USB的数据
  AudioData *getUSBData();
}

extern AudioData *AudioWhiteData;