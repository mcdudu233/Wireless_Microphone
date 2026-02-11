#pragma once

#if __has_include("lvgl.h")
#ifndef LV_LVGL_H_INCLUDE_SIMPLE
#define LV_LVGL_H_INCLUDE_SIMPLE
#endif
#endif

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

// 图片资源
extern const lv_image_dsc_t ui_img_system_info;
extern const lv_image_dsc_t ui_img_about;
extern const lv_image_dsc_t ui_img_microphone;
extern const lv_image_dsc_t ui_img_audio;
extern const lv_image_dsc_t ui_img_usb;
extern const lv_image_dsc_t ui_img_wifi;
extern const lv_image_dsc_t ui_img_bt;
extern const lv_image_dsc_t ui_img_tips;
extern const lv_image_dsc_t lv_img_bluetooth;

// 字体资源
extern const lv_font_t lv_font_harmonyos_12;
extern const lv_font_t lv_font_harmonyos_14;
extern const lv_font_t lv_font_harmonyos_16;
