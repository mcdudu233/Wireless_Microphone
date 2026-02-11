#pragma once
#include "ui/ui.h"
#include <vector>

#define UPDATE_TIMER_PERIOD 1000

typedef struct
{
    lv_obj_t *left_voice_bar;
    lv_obj_t *right_voice_bar;
    lv_obj_t *power_label;
    lv_obj_t *signal_label;
    lv_obj_t *tab;
    std::string device_mac;

} device_card_data;

// api
device_card_data *ui_info_get_obj(const std::string &mac);
void ui_info_set_bar_pct(lv_obj_t *bar, int8_t pct);

const std::string ui_info_get_transmit_speed();
int8_t ui_info_get_power(const std::string &mac);
int8_t ui_info_get_signal(const std::string &mac);
int8_t ui_info_get_left_voice(const std::string &mac);
int8_t ui_info_get_right_voice(const std::string &mac);

void ui_info_del_card(device_card_data *card);
void ui_info_del_card(const std::string &mac);

// 定义
std::vector<std::string> ui_bt_get_linked();
