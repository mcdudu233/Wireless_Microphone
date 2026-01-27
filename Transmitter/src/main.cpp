#include "logger.h"
#include "config.h"
#include "module/led.h"
#include "module/power.h"
#include "module/audio/power.h"
#include "module/audio/buffer.h"
#include "module/audio/encoder.h"
#include "module/rf.h"

extern "C" void app_main()
{
  logger::setup();
  config::setup();
  led::setup();
  power::setup();
  audio::power::setup();
  audio::buffer::setup();
  audio::encoder::setup();
  rf::setup();

  logger::infoln("All modules are started now!");
}