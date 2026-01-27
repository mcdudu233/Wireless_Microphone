#include "logger.h"
#include "module/audio/power.h"

static bool powerOn = false;

void audio::power::setup()
{
  pinMode(AUDIO_POWER_IO, OUTPUT);
  digitalWrite(AUDIO_POWER_IO, LOW);
  logger::debugln("Audio Power is started!");
}

void audio::power::on()
{
  if (!powerOn)
  {
    digitalWrite(AUDIO_POWER_IO, HIGH);
    powerOn = true;
    logger::debugln("Audio Power is on.");
  }
}

void audio::power::off()
{
  if (powerOn)
  {
    digitalWrite(AUDIO_POWER_IO, LOW);
    powerOn = false;
    logger::debugln("Audio Power is off.");
  }
}

bool audio::power::isOn()
{
  return powerOn;
}