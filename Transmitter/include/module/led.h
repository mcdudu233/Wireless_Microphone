#pragma once

#include "stdint.h"
#include "llrgb.h"

#define LED_NUM 1
#define LED_IO GPIO_NUM_8

namespace led
{
  void setup();

  // 设置灯珠颜色
  void black();
  void red();
  void green();
  void blue();
  void rgb(uint8_t r, uint8_t g, uint8_t b);
  void rgb(uint32_t color);
  void rgb(rgb_t rgb);
  // 上一个颜色
  uint32_t getLastColor();
  void backLastColor();
  // 设置亮度 0~100%
  void brightness(float v);
}