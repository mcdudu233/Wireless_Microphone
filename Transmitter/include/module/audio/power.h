#pragma once

#define AUDIO_POWER_IO GPIO_NUM_38

namespace audio::power
{
  void setup();
  void on();
  void off();
  bool isOn();
}