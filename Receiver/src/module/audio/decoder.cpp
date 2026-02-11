#include "config.h"
#include "logger.h"
#include "module/audio/buffer.h"
#include "module/audio/decoder.h"

#include "cctype"
#include "driver/i2s_std.h"

// I2S配置
static const i2s_chan_config_t i2s_chan_cfg = {
    .id = I2S_NUM_AUTO,
    .role = I2S_ROLE_MASTER,
    .dma_desc_num = 10,   // 多少个DMA
    .dma_frame_num = 384, // 每个DMA大小
    .auto_clear_after_cb = true,
    .auto_clear_before_cb = false,
    .allow_pd = false,
    .intr_priority = 0,
};
static const i2s_std_gpio_config_t i2s_gpio_cfg = {
    .mclk = I2S_GPIO_UNUSED,
    .bclk = AUDIO_DECODER_CLK,
    .ws = AUDIO_DECODER_WS,
    .dout = AUDIO_DECODER_SD,
    .din = I2S_GPIO_UNUSED,
    .invert_flags = {
        .mclk_inv = false,
        .bclk_inv = false,
        .ws_inv = false,
    }};

static i2s_chan_handle_t i2s_tx_handle;
static uint8_t i2s_channel;
static uint32_t i2s_rate;
static i2s_data_bit_width_t i2s_bit;
static bool powerOn = false;
static bool plugin = false;

// 记录丢包率
// static unsigned long last_time = millis();
// static int last_ok = 0;
// static int last_fail = 0;

// 实时处理音频数据
static void audioHandle(void *arg)
{
  // 音频数据
  AudioData *data;
  AudioData *data_channel = (AudioData *)heap_caps_malloc(sizeof(AudioData), MALLOC_CAP_SPIRAM);

  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(TASK_AUDIO_DECODER_PERIOD);
  while (true)
  {
    xTaskDelayUntil(&xLastWakeTime, xFrequency);

    // 判断是否插入3.5mm接口
    bool isPlugin = !digitalRead(AUDIO_DECODER_ON);
    if (isPlugin != plugin)
    {
      plugin = isPlugin;
      if (isPlugin)
      {
        LOGGER_INFO("Audio Decoder found 3.5mm plug in!");
        audio::decoder::on(config::config.audio.rate, config::config.audio.bit, config::config.audio.channel);
      }
      else
      {
        LOGGER_INFO("Audio Decoder found 3.5mm plug out!");
        audio::decoder::off();
      }
    }

    // 启动了芯片才读取数据
    if (powerOn)
    {
      // 判断音频配置是否更改
      if (config::config.audio.rate != i2s_rate || config::config.audio.bit != i2s_bit || config::config.audio.channel != i2s_channel)
      {
        audio::decoder::off();
        audio::decoder::on(config::config.audio.rate, config::config.audio.bit, config::config.audio.channel);
        LOGGER_INFO("Audio Decoder found audio config changed.");
      }

      // 获取数据
      data = audio::buffer::getDecoderData();
      if (data != nullptr)
      {
        // LOGGER_INFO("Audio Decoder num=%d size=%d!", data->num, data->size);
        if (i2s_channel_write(i2s_tx_handle, data->data, data->size, NULL, AUDIO_DECODER_POLLING_CYCLE * 2) != ESP_OK)
        {
          LOGGER_INFO("Audio Decoder write fail!");
          // last_fail++;
        }
        // else
        // {
        //   last_ok++;
        // }

        // unsigned long now_time = millis();
        // if (now_time - last_time > 1000)
        // {
        //   last_time = now_time;
        //   logger::warnln("Audio Decoder write %d/%d!", last_ok, last_fail + last_ok);
        //   last_ok = 0;
        //   last_fail = 0;
        // }
      }
    }
  }
}

void audio::decoder::setup()
{
  LOGGER_INFO("Audio Decoder is starting...");
  pinMode(AUDIO_DECODER_MUTE, OUTPUT);
  pinMode(AUDIO_DECODER_FLT, OUTPUT);
  digitalWrite(AUDIO_DECODER_MUTE, LOW);
  digitalWrite(AUDIO_DECODER_FLT, LOW);
  // 启动插入检测
  pinMode(AUDIO_DECODER_ON, INPUT);
  xTaskCreatePinnedToCore(audioHandle, "audio_decoder_handle", TASK_AUDIO_DECODER_STACK, NULL, TASK_AUDIO_DECODER_PRIORITY, NULL, TASK_AUDIO_DECODER_CORE);
  LOGGER_INFO("Audio Decoder is started!");
}

void audio::decoder::on(uint32_t rate, uint32_t bit, uint8_t channel)
{
  // 启动 i2s
  i2s_rate = rate;
  i2s_bit = (i2s_data_bit_width_t)bit;
  i2s_channel = channel;
  i2s_new_channel(&i2s_chan_cfg, &i2s_tx_handle, NULL);
  i2s_std_config_t std_cfg = {
      .clk_cfg = {
          .sample_rate_hz = i2s_rate,
          .clk_src = I2S_CLK_SRC_PLL_240M,
          .ext_clk_freq_hz = 0,
          .mclk_multiple = ((i2s_bit == 24) ? I2S_MCLK_MULTIPLE_576 : I2S_MCLK_MULTIPLE_512),
          .bclk_div = 0,
      },
      .slot_cfg = {.data_bit_width = i2s_bit, .slot_bit_width = (i2s_slot_bit_width_t)i2s_bit, .slot_mode = (i2s_slot_mode_t)i2s_channel, .slot_mask = I2S_STD_SLOT_BOTH, .ws_width = i2s_bit, .ws_pol = false, .bit_shift = true, .left_align = true, .big_endian = false, .bit_order_lsb = false},
      .gpio_cfg = i2s_gpio_cfg,
  };
  i2s_channel_init_std_mode(i2s_tx_handle, &std_cfg);
  i2s_channel_enable(i2s_tx_handle);
  powerOn = true;
  setMute(false);
  LOGGER_INFO("Audio Decoder is on.");
}

void audio::decoder::off()
{
  setMute(true);
  i2s_channel_disable(i2s_tx_handle);
  i2s_del_channel(i2s_tx_handle);
  powerOn = false;
  LOGGER_INFO("Audio Decoder is off.");
}

bool audio::decoder::isOn()
{
  return powerOn;
}

void audio::decoder::setMute(bool on)
{
  if (on)
  {
    digitalWrite(AUDIO_DECODER_MUTE, LOW);
  }
  else
  {
    digitalWrite(AUDIO_DECODER_MUTE, HIGH);
  }
}

void audio::decoder::setFLT(bool on)
{
  if (on)
  {
    digitalWrite(AUDIO_DECODER_FLT, HIGH);
  }
  else
  {
    digitalWrite(AUDIO_DECODER_FLT, LOW);
  }
}