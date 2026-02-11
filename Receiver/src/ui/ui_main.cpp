#include "ui/ui_main.h"
#include "module/screen.h"
#include <cstring>
#include <algorithm>

static lv_style_t style_indic_h; // 横向bar样式
static lv_obj_t *info_widget;
static lv_obj_t *main_widget;
static lv_obj_t *transmit_speed_label;
static lv_obj_t *tabview;                                                                           // 最多4个                                                                              // 纵向bar样式
static lv_obj_t *ui_create_device_card(lv_obj_t *parent, std::string &device_mac, bool color_test); // 创建自定义card容器组件

static std::vector<std::string> linked_devices;
static std::vector<device_card_data *> cards;

static lv_timer_t *timer_update;

static void setting_widget_cb(lv_event_t *e); // 设置按钮回调

void ui_main_init()
{
    lv_group_t *g = lv_group_create();
    lv_group_set_default(g);

    lv_style_init(&style_indic_h);
    lv_style_set_bg_opa(&style_indic_h, LV_OPA_COVER);
    lv_style_set_bg_color(&style_indic_h, lv_palette_main(LV_PALETTE_GREEN));
    lv_style_set_bg_grad_color(&style_indic_h, lv_palette_main(LV_PALETTE_RED));
    lv_style_set_bg_grad_dir(&style_indic_h, LV_GRAD_DIR_HOR);

    lv_obj_t *btn;
    lv_obj_t *label;
    lv_obj_t *last;

    main_widget = ui_add_win();
    info_widget = lv_obj_create(main_widget);
    lv_obj_set_style_bg_opa(info_widget, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(info_widget, 0, 0); // 去除内边距
    lv_obj_set_style_border_width(info_widget, 1, 0);
    lv_obj_set_size(info_widget, 160 * 0.6, 70);
    lv_obj_align_to(info_widget, main_widget, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_obj_set_style_border_width(info_widget, 0, 0); // 去除边框

    tabview = lv_tabview_create(info_widget);
    lv_tabview_set_tab_bar_size(tabview, 20);
    lv_obj_align(tabview, LV_ALIGN_CENTER, 0, 0);

    int i = 0;

    linked_devices = ui_bt_get_linked();
    for (std::string dev : linked_devices)
    {
        // logger::infoln(dev.c_str());
        lv_obj_t *tab = lv_tabview_add_tab(tabview, std::to_string(i + 1).c_str());
        lv_obj_t *card = ui_create_device_card(tab, dev, false);
        lv_obj_set_style_pad_all(tab, 0, 0);
        lv_obj_align(card, LV_ALIGN_CENTER, 0, 0);
        device_card_data *data = (device_card_data *)lv_obj_get_user_data(card);
        data->tab = tab;
        i++;
    }

    lv_obj_t *tab_bar = lv_tabview_get_tab_bar(tabview); // 标题栏
    uint32_t cnt = lv_obj_get_child_count_by_type(tab_bar, &lv_button_class);
    for (int i = 0; i < cnt; i++)
    {
        btn = lv_obj_get_child_by_type(tab_bar, i, &lv_button_class);
        label = lv_obj_get_child(btn, 0);
        lv_obj_set_style_text_font(label, &lv_font_harmonyos_12, 0);
    }

    lv_obj_t *label_widget = lv_obj_create(main_widget);
    lv_obj_set_style_pad_all(label_widget, 0, 0);      // 去除内边距
    lv_obj_set_style_border_width(label_widget, 0, 0); // 去除边框
    lv_obj_set_size(label_widget, 160 * 0.3 + 5, 35);
    lv_obj_align_to(label_widget, info_widget, LV_ALIGN_OUT_RIGHT_MID, 2, -5);

    transmit_speed_label = lv_label_create(label_widget);
    lv_label_set_recolor(transmit_speed_label, true);
    lv_label_set_text_fmt(transmit_speed_label, "#333333 %s#%.1fb/s", LV_SYMBOL_DOWNLOAD, 1.1);
    lv_obj_set_style_text_font(transmit_speed_label, &lv_font_harmonyos_12, 0);
    lv_obj_align(transmit_speed_label, LV_ALIGN_CENTER, 0, 0);

    btn = ui_add_button(main_widget, "设置", 160 * 0.3, 25, NULL);
    lv_obj_align_to(btn, info_widget, LV_ALIGN_OUT_RIGHT_MID, 3, 20);
    lv_obj_add_event_cb(btn, setting_widget_cb, LV_EVENT_CLICKED, main_widget); // 切换窗体并隐藏

    lv_obj_t *img;
    img = lv_label_create(main_widget);
    lv_label_set_text(img, LV_SYMBOL_USB);
    lv_obj_set_style_text_font(img, &lv_font_harmonyos_12, 0);
    lv_obj_align_to(img, label_widget, LV_ALIGN_OUT_TOP_LEFT, 3, 0);
    last = img;
    img = lv_label_create(main_widget);
    lv_label_set_text(img, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_font(img, &lv_font_harmonyos_12, 0);
    lv_obj_align_to(img, last, LV_ALIGN_OUT_RIGHT_MID, 3, 0);
    last = img;
    img = lv_image_create(main_widget);
    lv_image_set_src(img, &lv_img_bluetooth);
    lv_img_set_zoom(img, 32);
    lv_obj_set_size(img, 16, 16);
    lv_obj_align_to(img, last, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

    ui_bind_group_to_all_encoders(lv_group_get_default());

    timer_update = lv_timer_create([](lv_timer_t *t)
                                   {
                                        // 更新数据
                                        std::string speed = ui_info_get_transmit_speed();

                                        uint32_t act = lv_tabview_get_tab_active(tabview);
                                        lv_obj_t *content = lv_tabview_get_content(tabview);
                                        lv_obj_t *cur_tab = lv_obj_get_child(content, act);
                                        lv_obj_t *card = lv_obj_get_child(cur_tab, 0);
                                        device_card_data *card_data = (device_card_data *)lv_obj_get_user_data(card);
                                        
                                        lv_label_set_text_fmt(transmit_speed_label, "#00FF00 %s#%s", LV_SYMBOL_DOWNLOAD, speed.c_str());

                                        // 设置卡片内信息
                                        if (card_data == nullptr)
                                            return;

                                        lv_bar_set_value(card_data->left_voice_bar, ui_info_get_left_voice(card_data->device_mac), LV_ANIM_ON);
                                        lv_bar_set_value(card_data->right_voice_bar, ui_info_get_right_voice(card_data->device_mac), LV_ANIM_ON);
                                        lv_label_set_text_fmt(card_data->power_label, "电量%d", ui_info_get_power(card_data->device_mac));
                                        lv_label_set_text_fmt(card_data->signal_label, "信号%d", ui_info_get_signal(card_data->device_mac)); },
                                   UPDATE_TIMER_PERIOD, NULL);
}

// 设置按钮回调 -> 进入设置界面
static void setting_widget_cb(lv_event_t *e)
{
    ui_setting_init(main_widget);
}

// 创建设备卡片
static lv_obj_t *ui_create_device_card(lv_obj_t *parent, std::string &device_mac, bool color_test)
{
    device_card_data *data = (device_card_data *)lv_malloc(sizeof(device_card_data));
    LV_ASSERT_MALLOC(data);

    lv_obj_t *card = lv_obj_create(parent);

    // lv_obj_set_style_border_width(card, 0, 0); // 去除边框
    lv_obj_set_style_pad_all(card, 0, 0);
    // 使用百分比自适应大小，而不是在创建时读取parent尺寸
    lv_obj_set_width(card, lv_pct(100));
    lv_obj_set_height(card, lv_pct(100));

    lv_obj_t *left_label = lv_label_create(card);
    lv_label_set_text(left_label, "L");
    lv_obj_align(left_label, LV_ALIGN_CENTER, -40, -15);
    lv_obj_set_style_text_font(left_label, &lv_font_harmonyos_12, 0);
    lv_obj_t *right_label = lv_label_create(card);
    lv_label_set_text(right_label, "R");
    lv_obj_align(right_label, LV_ALIGN_CENTER, -40, 0);
    lv_obj_set_style_text_font(right_label, &lv_font_harmonyos_12, 0);

    data->left_voice_bar = lv_bar_create(card);
    lv_obj_add_style(data->left_voice_bar, &style_indic_h, LV_PART_INDICATOR);
    lv_obj_set_size(data->left_voice_bar, 160 * 0.6 - 15, 10);
    lv_obj_align_to(data->left_voice_bar, left_label, LV_ALIGN_OUT_RIGHT_MID, 5, 3);
    lv_bar_set_range(data->left_voice_bar, 0, 100);

    data->right_voice_bar = lv_bar_create(card);
    lv_obj_add_style(data->right_voice_bar, &style_indic_h, LV_PART_INDICATOR);
    lv_obj_set_size(data->right_voice_bar, 160 * 0.6 - 15, 10);
    lv_obj_align_to(data->right_voice_bar, right_label, LV_ALIGN_OUT_RIGHT_MID, 5, 3);
    lv_bar_set_range(data->right_voice_bar, 0, 100);

    data->power_label = lv_label_create(card);
    lv_obj_align(data->power_label, LV_ALIGN_BOTTOM_LEFT, 0, -1);
    lv_obj_set_style_text_font(data->power_label, &lv_font_harmonyos_12, 0);
    lv_label_set_text_fmt(data->power_label, "电量%d", 100);

    data->signal_label = lv_label_create(card);
    lv_obj_align(data->signal_label, LV_ALIGN_BOTTOM_RIGHT, 0, -1);
    lv_obj_set_style_text_font(data->signal_label, &lv_font_harmonyos_12, 0);
    lv_label_set_text_fmt(data->signal_label, "信号%d", 100);

    if (color_test)
    {
        // 颜色测试
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_exec_cb(&a, ui_set_bar_val);
        lv_anim_set_duration(&a, 3000);
        lv_anim_set_reverse_duration(&a, 3000);
        lv_anim_set_var(&a, data->left_voice_bar);
        lv_anim_set_values(&a, 0, 100);
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
        lv_anim_start(&a);

        lv_anim_t b;
        lv_anim_init(&b);
        lv_anim_set_exec_cb(&b, ui_set_bar_val);
        lv_anim_set_duration(&b, 2000);
        lv_anim_set_var(&b, data->right_voice_bar);
        lv_anim_set_values(&b, 0, 100);
        lv_anim_set_repeat_count(&b, LV_ANIM_REPEAT_INFINITE);
        lv_anim_start(&b);
    }

    data->device_mac = device_mac;

    lv_obj_set_user_data(card, data);
    cards.push_back(data);
    return card;
}

void ui_free_main_widget()
{
    lv_timer_delete(timer_update);
    for (size_t i = 0; i < cards.size(); ++i)
    {
        device_card_data *card_data = cards[i];
        if (card_data)
        {
            lv_free(card_data);
        }
    }
    cards.clear();

    if (main_widget && lv_obj_is_valid(main_widget))
    {
        lv_obj_delete(main_widget);
        main_widget = NULL;
    }
}

// api

device_card_data *ui_info_get_obj(const std::string &mac)
{
    int i = 0;
    for (device_card_data *dev : cards)
    {
        if (dev->device_mac == mac)
        {
            return dev;
        }
        i++;
    }
    return nullptr;
}

void ui_info_set_bar_pct(lv_obj_t *bar, int8_t pct)
{
    LV_LOCK();
    lv_bar_set_value(bar, pct, LV_ANIM_ON);
    LV_UNLOCK();
}

void ui_info_del_card(const std::string &mac)
{
    int i = 0;
    for (device_card_data *card : cards)
    {
        if (card->device_mac == mac)
        {
            LV_LOCK();
            lv_obj_del(card->tab);
            lv_free(card);
            cards.erase(cards.begin() + i);
            LV_UNLOCK();

            // 如果所有卡片都被删除，返回蓝牙连接窗口
            if (cards.empty())
            {
                ui_free_main_widget();
                ui_bt_init();
            }
            break;
        }
        i++;
    }
}
void ui_info_del_card(device_card_data *card)
{
    ui_info_del_card(std::string(card->device_mac));
}