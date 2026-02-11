#pragma once

#include "ui/ui.h"

#define BT_LINKED ((void *)1)
#define BT_UNLINKED ((void *)0)

#define COLOR_LINKED lv_palette_main(LV_PALETTE_GREEN)
#define COLOR_LINKING lv_palette_main(LV_PALETTE_BLUE)
#define COLOR_SELECTED lv_palette_main(LV_PALETTE_RED)
#define COLOR_NONE LV_PALETTE_NONE

// api
void ui_bt_update(const std::string &mac, bool is_link = false);

// 定义
bool ui_bt_unlink(const std::string &mac);
bool ui_bt_link(const std::string &mac);
void ui_bt_search();
void ui_bt_pause_search();