#include "logger.h"
#include "module/audio/buffer.h"

// 循环缓冲区
static AudioData *data;
static uint8_t data_pointer;

// 使用互斥锁保护缓冲区
static SemaphoreHandle_t mutex = NULL;

// 获取过去的第N个数据(不带缓冲)
static AudioData *getAudioData(uint8_t last)
{
  if (last >= (AUDIO_BUFFER_MAX_BUFFER_SIZE - 1))
  {
    return nullptr;
  }
  return &data[(data_pointer + AUDIO_BUFFER_MAX_BUFFER_SIZE - 1 - last) % AUDIO_BUFFER_MAX_BUFFER_SIZE];
}

// 获取目前的音频原始数据包(不带缓冲)
static AudioData *getAudioDataFront()
{
  return getAudioData(0);
}

void audio::buffer::writeWiFiPacket(WiFiAudioPacket *packet)
{
  xSemaphoreTake(mutex, portMAX_DELAY);
  uint32_t now_number = getAudioDataFront()->num;
  if (packet->number <= now_number)
  {
    // 将分包合在一个音频数据包里
    AudioData *audio = getAudioData(now_number - packet->number);
    if (audio != nullptr)
    {
      audio->size += packet->size;
      memcpy(audio->data + PACKET_WIFI_AUDIO_DATA_MAX_SIZE * packet->part, packet->data, packet->size);
    }
  }
  else
  {
    // 接收到新的包 创建一个包
    uint8_t last = packet->number - now_number;

    // 如果中间丢了N个包 填充
    AudioData *audio;
    for (uint32_t num = packet->number - last + 1; num <= packet->number; num++)
    {
      audio = &data[data_pointer];
      data_pointer = (data_pointer + 1) % AUDIO_BUFFER_MAX_BUFFER_SIZE;
      audio->num = num;
      audio->size = 0;
    }

    // 写入最新包的数据
    audio->size = packet->size;
    // 这里可能接收到的不是第一个part 可能丢包
    if (packet->part != 0)
    {
      // 前面的part填充为0
      memset(audio->data, 0, (packet->part - 1) * PACKET_WIFI_AUDIO_DATA_MAX_SIZE);
    }
    memcpy(audio->data + packet->part * PACKET_WIFI_AUDIO_DATA_MAX_SIZE, packet->data, packet->size);
  }
  xSemaphoreGive(mutex);
}

static uint32_t decoderLastNumber = 0;
static bool decoderDelayFlag = false;
AudioData *audio::buffer::getDecoderData()
{
  // 第一次读取最前面的包
  if (decoderLastNumber == 0)
  {
    xSemaphoreTake(mutex, portMAX_DELAY);
    AudioData *audio = getAudioData(AUDIO_BUFFER_DELAY_PACKET);
    xSemaphoreGive(mutex);
    if (audio->num != 0)
    {
      decoderLastNumber = audio->num;
      return audio;
    }
    return nullptr;
  }

  xSemaphoreTake(mutex, portMAX_DELAY);
  int32_t now_number = getAudioDataFront()->num;
  xSemaphoreGive(mutex);
  // 判断读取和写入的关系
  int32_t number = decoderLastNumber + 1;
  if (number >= now_number)
  {
    // 读取太快了
    decoderDelayFlag = true; // 启动延迟读取
    return nullptr;
  }
  if (decoderDelayFlag)
  {
    // 延迟读取
    if ((now_number - number) >= AUDIO_BUFFER_DELAY_PACKET)
    {
      decoderDelayFlag = false;
    }
    return nullptr;
  }

  // 正常读取数据
  xSemaphoreTake(mutex, portMAX_DELAY);
  AudioData *audio = getAudioData(getAudioDataFront()->num - number);
  xSemaphoreGive(mutex);
  if (audio == nullptr)
  {
    // 写入太快了 缓存不足
    xSemaphoreTake(mutex, portMAX_DELAY);
    audio = getAudioData(AUDIO_BUFFER_DELAY_PACKET);
    xSemaphoreGive(mutex);
    LOGGER_WARN("Audio Buffer decoder miss packet from %d to %d!", decoderLastNumber, audio->num);
  }
  if (audio != nullptr)
  {
    decoderLastNumber = audio->num;
  }
  return audio;
}

static uint32_t usbLastNumber = 0;
static bool usbDelayFlag = false;
AudioData *audio::buffer::getUSBData()
{
  // 第一次读取最前面的包
  if (usbLastNumber == 0)
  {
    xSemaphoreTake(mutex, portMAX_DELAY);
    AudioData *audio = getAudioData(AUDIO_BUFFER_DELAY_PACKET);
    xSemaphoreGive(mutex);
    if (audio->num != 0)
    {
      usbLastNumber = audio->num;
      return audio;
    }
    return nullptr;
  }

  xSemaphoreTake(mutex, portMAX_DELAY);
  int32_t now_number = getAudioDataFront()->num;
  xSemaphoreGive(mutex);
  // 判断读取和写入的关系
  int32_t number = usbLastNumber + 1;
  if (number >= now_number)
  {
    // 读取太快了
    usbDelayFlag = true; // 启动延迟读取
    return nullptr;
  }
  if (usbDelayFlag)
  {
    // 延迟读取
    if ((now_number - number) >= AUDIO_BUFFER_DELAY_PACKET)
    {
      usbDelayFlag = false;
    }
    return nullptr;
  }

  // 正常读取数据
  xSemaphoreTake(mutex, portMAX_DELAY);
  AudioData *audio = getAudioData(getAudioDataFront()->num - number);
  xSemaphoreGive(mutex);
  if (audio == nullptr)
  {
    // 写入太快了 缓存不足
    xSemaphoreTake(mutex, portMAX_DELAY);
    audio = getAudioData(AUDIO_BUFFER_DELAY_PACKET);
    xSemaphoreGive(mutex);
  }
  if (audio != nullptr)
  {
    usbLastNumber = audio->num;
  }
  return audio;
}

void audio::buffer::restart()
{
  xSemaphoreTake(mutex, portMAX_DELAY);
  for (int i = 0; i < AUDIO_BUFFER_MAX_BUFFER_SIZE; i++)
  {
    data[i].num = 0;
    data[i].size = 0;
  }
  data_pointer = 0;
  decoderLastNumber = 0;
  usbLastNumber = 0;
  decoderDelayFlag = false;
  usbDelayFlag = false;
  xSemaphoreGive(mutex);
}

void audio::buffer::setup()
{
  // 创建互斥锁
  mutex = xSemaphoreCreateMutex();
  data = (AudioData *)heap_caps_malloc(sizeof(AudioData) * AUDIO_BUFFER_MAX_BUFFER_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_32BIT);
  restart();
  LOGGER_INFO("Audio Buffer is started.");
}