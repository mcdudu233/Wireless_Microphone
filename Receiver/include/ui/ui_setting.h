#pragma once
#include "config.h"

// 系统信息刷新周期 ms
#define SYSTEM_INFO_REFLUSH_TIME 1000
#define AUTHOR "dudu233, tiosa"
#define WEBSITE "www.mcso.top/mic"

// 获取系统信息调用
void ui_setting_system_page_rcb(float &cpu1_pct, float &cpu2_pct, size_t &iram_current, size_t &psram_current, size_t &l_iram_max, size_t &l_psram_max);

// 更新新的配置时调用

// audio页面读取回调
void ui_setting_audio_page_rcb(uint8_t &bit, uint8_t &channel, uint32_t &rate, uint8_t &volumn, uint8_t &volumn_mode);
// audio页面保存回调 now: 是否立即生效
void ui_setting_audio_page_scb(uint8_t bit, uint8_t channel, uint32_t rate, uint8_t volumn, uint8_t &volumn_mode, bool now);

void ui_setting_usb_page_rcb(USBMode &mode);
void ui_setting_usb_page_scb(USBMode mode, bool now);

void ui_setting_rf_page_rcb(bool &mode);
void ui_setting_rf_page_scb(bool mode, bool now);