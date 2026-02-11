#pragma once

// 屏幕分辨率
#define TFT_HOR_RES 160
#define TFT_VER_RES 80

// 屏幕引脚
#define TFT_SPI_NUM SPI2_HOST
#define TFT_SPI_FREQ 80 * 1000 * 1000
#define TFT_RST GPIO_NUM_16
#define TFT_CS GPIO_NUM_15
#define TFT_DC GPIO_NUM_17
#define TFT_MOSI GPIO_NUM_21
#define TFT_CLK GPIO_NUM_18

// 屏幕背光
#define TFT_BLK GPIO_NUM_7
#define TFT_BLK_FREQ 100 * 1000
#define TFT_BLK_BIT 8
#define TFT_BLK_TIME 100

// LVGL 互斥锁
#define LV_LOCK() screen::lv_lock_wait()
#define LV_UNLOCK() screen::lv_unlock()

namespace screen
{
  void setup();

  // 设置屏幕背光亮度
  void backlight(float percent);

  // LVGL 互斥锁
  bool lv_lock();
  void lv_lock_wait();
  void lv_unlock();
}