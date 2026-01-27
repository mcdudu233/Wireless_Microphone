#include "logger.h"
#include "module/audio/buffer.h"

// 循环缓冲区
static AudioData *data;
static uint8_t data_pointer;
static uint32_t data_number;

uint8_t *audio::buffer::getWritePointer(uint32_t packet_size)
{
  AudioData *buffer = &data[data_pointer];
  data_pointer = (data_pointer + 1) % AUDIO_BUFFER_MAX_BUFFER_SIZE;
  buffer->num = data_number++ % UINT32_MAX;
  buffer->size = packet_size;
  return buffer->data;
}

// 获取过去的第N个数据
static AudioData *getAudioData(uint8_t last)
{
  return &data[(data_pointer + AUDIO_BUFFER_MAX_BUFFER_SIZE - 1 - last) % AUDIO_BUFFER_MAX_BUFFER_SIZE];
}

AudioData *audio::buffer::getAudioDataFront()
{
  return getAudioData(0);
}

AudioData *audio::buffer::getAudioDataFromNumber(uint32_t number)
{
  int32_t now = getAudioDataFront()->num;
  if (number > now)
  {
    return nullptr;
  }
  else if (number == now)
  {
    return getAudioDataFront();
  }
  else
  {
    if ((now - number) >= AUDIO_BUFFER_MAX_BUFFER_SIZE)
    {
      return nullptr;
    }
    return getAudioData(now - number);
  }
}

uint8_t audio::buffer::getPointer()
{
  return data_pointer;
}

uint32_t audio::buffer::getNumber()
{
  return getAudioDataFront()->num;
}

static uint32_t wifiLastNumber = 0;
uint8_t audio::buffer::getWiFiPacketFront(netbuf ***buffers)
{
  AudioData *audio = getAudioDataFront();
  if (audio->num > wifiLastNumber)
  {
    wifiLastNumber = audio->num; // 已发送

    uint8_t part_max = audio->size / PACKET_WIFI_AUDIO_DATA_MAX_SIZE + 1;
    *buffers = (netbuf **)malloc(sizeof(netbuf *) * part_max);
    if (*buffers == NULL)
    {
      logger::warnln("Socket (netbuf **) malloc failed!");
      return 0;
    }
    // 创建每个包
    for (uint8_t part = 0; part < part_max; part++)
    {
      // 计算包大小
      uint16_t part_size;
      if (part != part_max - 1)
      {
        part_size = PACKET_WIFI_AUDIO_DATA_MAX_SIZE;
      }
      else
      {
        // 最后一个包不一定是满的
        part_size = audio->size - PACKET_WIFI_AUDIO_DATA_MAX_SIZE * (part_max - 1);
      }

      // 初始化结构体
      netbuf *buffer = NULL;
      buffer = netbuf_new();
      if (buffer == NULL)
      {
        // 释放之前已分配的资源
        for (uint8_t i = 0; i < part; i++)
        {
          netbuf_delete((*buffers)[i]);
        }
        free(*buffers);
        *buffers = NULL;
        logger::warnln("Socket netbuf_new() malloc failed!");
        return 0;
      }
      (*buffers)[part] = buffer;

      // 复制数据
      Packet *packet = (Packet *)netbuf_alloc(buffer, PACKET_WIFI_AUDIO_HEAD_SIZE + part_size);
      if (packet == NULL)
      {
        // 释放当前buffer
        netbuf_delete((*buffers)[part]);
        // 释放之前已分配的buffers
        for (uint8_t i = 0; i < part; i++)
        {
          netbuf_delete((*buffers)[i]);
        }
        free(*buffers);
        *buffers = NULL;
        logger::warnln("Socket netbuf_alloc() malloc failed!");
        return 0;
      }
      packet->type = PACKET_TYPE_WIFI_AUDIO;
      packet->packet.audioDataWiFi.size = part_size;
      packet->packet.audioDataWiFi.number = audio->num;
      packet->packet.audioDataWiFi.part = part;
      memcpy(packet->packet.audioDataWiFi.data, audio->data + PACKET_WIFI_AUDIO_DATA_MAX_SIZE * part, part_size);
    }
    return part_max;
  }
  else
  {
    return 0;
  }
}

static uint32_t bleLastNumber = 0;

void audio::buffer::restart()
{
  for (int i = 0; i < AUDIO_BUFFER_MAX_BUFFER_SIZE; i++)
  {
    data[i].num = 0;
    data[i].size = 0;
  }
  data_pointer = 0;
  data_number = 0;
  wifiLastNumber = 0;
  bleLastNumber = 0;
}

void audio::buffer::setup()
{
  data = (AudioData *)heap_caps_malloc(sizeof(AudioData) * AUDIO_BUFFER_MAX_BUFFER_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_32BIT);
  restart();
  logger::debugln("Audio Buffer is started.");
}