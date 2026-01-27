#pragma once

#include "module/rf.h"
#include "module/audio/encoder.h"
#include "cinttypes"
#include "lwip/api.h"

#define AUDIO_BUFFER_MAX_DATA_SIZE (AUDIO_ENCODER_RATE * AUDIO_ENCODER_BIT * AUDIO_ENCODER_CHANNEL / 8 * AUDIO_ENCODER_POLLING_CYCLE / 1000)
#define AUDIO_BUFFER_MAX_BUFFER_SIZE 25

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

  /* 原始方法 */
  // 获取当前指针
  uint8_t getPointer();
  // 获取当前音频包号码
  uint32_t getNumber();
  // 获取目前的音频原始数据包
  AudioData *getAudioDataFront();
  // 根据音频包号码获取音频原始数据包
  AudioData *getAudioDataFromNumber(uint32_t number);

  /* 写入数据 */
  // 获取写入数据的指针
  uint8_t *getWritePointer(uint32_t packet_size);

  /* 读取WiFi数据 */
  // 获取目前的音频数据包分包
  uint8_t getWiFiPacketFront(netbuf ***buffers);
  // uint8_t getWiFiPacketFromNumber(uint32_t number, netbuf **buffer);
}
