#include "logger.h"
#include "module/led.h"

#include "LiteLED.h"

static LiteLED strip(LED_STRIP_WS2812, false);
static rgb_t lastRGB;

void led::setup()
{
  strip.begin(LED_IO, LED_NUM);
  brightness(50);
  black();
  logger::debugln("LED is started!");
}

void led::black()
{
  rgb(0x000000);
}

void led::red()
{
  rgb(0xff0000);
}

void led::green()
{
  rgb(0x00ff00);
}

void led::blue()
{
  rgb(0x0000ff);
}

void led::rgb(uint8_t r, uint8_t g, uint8_t b)
{
  led::rgb(rgb_from_values(r, g, b));
}

void led::rgb(uint32_t color)
{
  led::rgb(rgb_from_code(color));
}

void led::rgb(rgb_t rgb)
{
  strip.setPixel(0, rgb);
  strip.show();
  lastRGB = rgb;
}

void led::brightness(float v)
{
  strip.brightness((uint8_t)(v * 255.0 / 100.0));
  strip.show();
}

uint32_t led::getLastColor()
{
  return rgb_to_code(lastRGB);
}

void led::backLastColor()
{
  rgb(lastRGB);
}