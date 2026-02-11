#include "ui/ui.h"
#include "ui/ui_setting.h"

static lv_obj_t *last_widget;

static lv_obj_t *menu;
static lv_obj_t *setting_widget;
static lv_obj_t *last_enter_btn = NULL; // 记录进入子页面所用的条目
static lv_timer_t *sys_info_timer;
static lv_style_t scroll_style;

static bool enter_bottom = false;
static lv_obj_t *root_page_ref = NULL;

static lv_obj_t *dd_audio_bit;
static lv_obj_t *dd_audio_channel;
static lv_obj_t *dd_audio_rate;
static lv_obj_t *dd_audio_volumn_mode;
static lv_obj_t *slider_audio_volumn;

static lv_obj_t *dd_rf_mode;

static lv_obj_t *dd_usb_mode;

static lv_obj_t *cpu1;
static lv_obj_t *cpu2;
static lv_obj_t *iram;
static lv_obj_t *psram;
// static lv_obj_t *sd;

static void save_config(uint8_t &p, bool now);
static void back_cb(lv_event_t *e);          // 设置页面back按钮回调
static void relink_bt_cb(lv_event_t *e);     // 重新链接蓝牙回调
static void enter_subpage_cb(lv_event_t *e); // 记录进入子页的来源条目
static void focus_async_cb(void *obj_p);     // 异步将焦点移回来源条目
static void sys_info_timer_cb(lv_timer_t *); // 系统信息更新timer
static void back_btn_focus_cb(lv_event_t *e);
static void scroll_event_cb(lv_event_t *e);

static lv_obj_t *ui_create_text(lv_obj_t *parent, const void *icon, const char *txt, lv_obj_t **label_o = nullptr, bool is_from_svg = false);
static lv_obj_t *ui_create_slider(lv_obj_t *parent, const void *icon, const char *txt, int32_t min, int32_t max,
                                  int32_t val, lv_obj_t **slider_obj = nullptr);
static lv_obj_t *ui_create_dropdown(lv_obj_t *parent, const void *icon, const char *txt, const char *options, lv_obj_t **dd_o = nullptr);
static lv_obj_t *ui_create_sub_page(lv_obj_t *parent, const char *title, bool display_scroll = true); // 新建子页面

enum e_page
{
    usb_page = 1,
    audio_page = 2,
    rf_page = 3,
    about_page = 4,
    system_page = 5,
    file_page = 6
};

void ui_setting_init(lv_obj_t *ui_from)
{
    lv_obj_t *btn;
    char *from_data;

    lv_style_init(&scroll_style);
    lv_style_set_pad_right(&scroll_style, 0);
    last_widget = ui_from;
    from_data = (char *)lv_obj_get_user_data(last_widget);
    lv_obj_set_flag(last_widget, LV_OBJ_FLAG_HIDDEN, true);

    ui_bind_group_to_all_encoders(lv_group_get_default());
    setting_widget = ui_add_win();
    menu = lv_menu_create(setting_widget);
    lv_obj_set_style_bg_color(menu, lv_color_hex(0xF5F7FA), 0);
    lv_menu_set_mode_root_back_button(menu, LV_MENU_ROOT_BACK_BUTTON_ENABLED);

    lv_obj_add_event_cb(menu, back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_size(menu, lv_display_get_horizontal_resolution(NULL) - 5, lv_display_get_vertical_resolution(NULL) - 5);
    lv_obj_center(menu);

    // 获取返回按钮并应用焦点样式
    lv_obj_t *main_header = lv_menu_get_main_header(menu);
    if (main_header)
    {
        lv_obj_t *back_btn = lv_obj_get_child_by_type(main_header, 0, &lv_button_class);
        if (back_btn)
        {
            // 去除按钮本身的焦点边框
            lv_obj_set_style_outline_width(back_btn, 0, LV_STATE_FOCUSED);
            lv_obj_set_style_outline_width(back_btn, 0, LV_STATE_FOCUS_KEY);
            // 添加焦点事件回调来改变图标颜色
            lv_obj_add_event_cb(back_btn, back_btn_focus_cb, LV_EVENT_FOCUSED, NULL);
            lv_obj_add_event_cb(back_btn, back_btn_focus_cb, LV_EVENT_DEFOCUSED, NULL);
        }
    }

    lv_obj_t *cont;

    /*Create sub pages*/
    lv_obj_t *sub_about_page = ui_create_sub_page(menu, "关于", false);
    lv_obj_set_user_data(sub_about_page, (void *)e_page::about_page);
    lv_obj_t *sub_usb_page = ui_create_sub_page(menu, "USB传输设置");
    lv_obj_set_user_data(sub_usb_page, (void *)e_page::usb_page);
    lv_obj_t *sub_file_page = ui_create_sub_page(menu, "文件系统管理");
    lv_obj_set_user_data(sub_usb_page, (void *)e_page::file_page);
    lv_obj_t *sub_system_page = ui_create_sub_page(menu, "系统信息");
    lv_obj_set_user_data(sub_system_page, (void *)e_page::system_page);
    lv_obj_t *sub_audio_page = ui_create_sub_page(menu, "音频设置");
    lv_obj_set_user_data(sub_audio_page, (void *)e_page::audio_page);
    lv_obj_t *sub_rf_page = ui_create_sub_page(menu, "无线连接设置");
    lv_obj_set_user_data(sub_rf_page, (void *)e_page::rf_page);

    // 关于
    lv_obj_t *label_title = lv_label_create(sub_about_page);
    lv_label_set_text_fmt(label_title, "无线麦克风 V%u.%u",
                          CONFIG_VERSION_VALUE >> 8, CONFIG_VERSION_VALUE & 0xFF);
    lv_obj_set_width(label_title, lv_pct(100));
    lv_obj_set_style_text_align(label_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(label_title, lv_color_hex(0x626367), 0);
    lv_obj_set_style_text_font(label_title, &lv_font_harmonyos_14, 0);

    lv_obj_t *label_author = lv_label_create(sub_about_page);
    lv_label_set_text_fmt(label_author, "作者:%s", AUTHOR);
    lv_obj_set_width(label_author, lv_pct(100));
    lv_obj_set_style_text_align(label_author, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(label_author, lv_color_hex(0x626367), 0);
    lv_obj_set_style_text_font(label_author, &lv_font_harmonyos_12, 0);

    lv_obj_t *label_link = lv_label_create(sub_about_page);
    lv_label_set_text(label_link, WEBSITE);
    lv_obj_set_width(label_link, lv_pct(100));
    lv_obj_set_style_text_align(label_link, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(label_link, lv_color_hex(0x626367), 0);
    lv_obj_set_style_text_font(label_link, &lv_font_harmonyos_12, 0);

    lv_obj_align(label_title, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_align_to(label_author, label_title, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    lv_obj_align_to(label_link, label_author, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

    // 系统信息
    ui_create_text(sub_system_page, NULL, "CPU 1", &cpu1);
    ui_create_text(sub_system_page, NULL, "CPU 2", &cpu2);
    ui_create_text(sub_system_page, NULL, "IRAM", &iram);
    ui_create_text(sub_system_page, NULL, "PSRAM", &psram);
    // ui_create_text(sub_system_page, NULL, "外置存储", &sd);

    lv_label_set_recolor(cpu1, true);
    lv_label_set_recolor(cpu2, true);
    lv_label_set_recolor(iram, true);
    lv_label_set_recolor(psram, true);
    // lv_label_set_recolor(sd, true);

    // section = lv_menu_section_create(sub_usb_page);
    ui_create_dropdown(sub_usb_page, NULL, "传输模式", "默认\n"
                                                       "音频\n"
                                                       "读卡器\n"
                                                       "JTAG",

                       &dd_usb_mode);
    // lv_obj_add_event_cb(dd_usb_mode, &choose_cb, LV_EVENT_VALUE_CHANGED, (void *)1);
    // 音频 频率 比特 通道 下拉菜单
    // 48000 96000 192000 Hz
    // 16 24 32 bit
    // 单声道 立体
    // section = lv_menu_section_create(sub_audio_page);
    ui_create_dropdown(sub_audio_page, NULL, "采样率", "48000Hz\n"
                                                       "96000Hz\n"
                                                       "192000Hz",
                       &dd_audio_rate);
    // lv_obj_add_event_cb(dd_audio_rate, &choose_cb, LV_EVENT_VALUE_CHANGED, (void *)2);
    // section = lv_menu_section_create(sub_audio_page);
    ui_create_dropdown(sub_audio_page, NULL, "位深度", "16bit\n"
                                                       "24bit\n"
                                                       "32bit",
                       &dd_audio_bit);
    // lv_obj_add_event_cb(dd_audio_bit, &choose_cb, LV_EVENT_VALUE_CHANGED, (void *)3);
    // section = lv_menu_section_create(sub_audio_page);
    ui_create_dropdown(sub_audio_page, NULL, "通道数", "单通道\n"
                                                       "立体声",
                       &dd_audio_channel);
    // lv_obj_add_event_cb(dd_audio_channel, &choose_cb, LV_EVENT_VALUE_CHANGED, (void *)4);
    ui_create_dropdown(sub_audio_page, NULL, "增益模式", "自动增益\n"
                                                         "峰值减少\n"
                                                         "手动",
                       &dd_audio_volumn_mode);
    ui_create_slider(sub_audio_page, NULL, "增益", 0, 60, 0, &slider_audio_volumn);

    ui_create_dropdown(sub_rf_page, NULL, "传输协议", "BLE\n"
                                                      "WIFI",
                       &dd_rf_mode);
    // lv_obj_add_event_cb(dd_rf_mode, &choose_cb, LV_EVENT_VALUE_CHANGED, (void *)6);

    lv_obj_t *root_page = lv_menu_page_create(menu, "设置");
    root_page_ref = root_page; // 保存引用
    lv_obj_add_style(root_page, &scroll_style, LV_PART_SCROLLBAR);
    lv_obj_add_event_cb(root_page, scroll_event_cb, LV_EVENT_SCROLL, NULL);

    cont = ui_create_text(root_page, &ui_img_audio, "音频设置", nullptr, true);
    lv_group_add_obj(lv_group_get_default(), cont);
    lv_menu_set_load_page_event(menu, cont, sub_audio_page);
    lv_obj_add_event_cb(cont, enter_subpage_cb, LV_EVENT_CLICKED, sub_audio_page);
    lv_obj_set_style_translate_x(cont, 100, 0);
    lv_obj_set_style_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_user_data(cont, (void *)0); // 标记未播放动画

    // section = lv_menu_section_create(root_page);
    if (!(from_data && lv_strcmp(from_data, "bt_list") == 0)) // 判断是否为蓝牙连接界面过来的
    {
        cont = ui_create_text(root_page, &ui_img_bt, "设备连接", nullptr, true); // finish
        lv_group_add_obj(lv_group_get_default(), cont);
        // lv_menu_set_load_page_event(menu, cont, sub_bt_page);
        lv_obj_add_event_cb(cont, relink_bt_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_set_style_translate_x(cont, 100, 0);
        lv_obj_set_style_opa(cont, LV_OPA_TRANSP, 0);
        lv_obj_set_user_data(cont, (void *)0); // 标记未播放动画
    }
    cont = ui_create_text(root_page, &ui_img_wifi, "无线传输设置", nullptr, true);
    lv_group_add_obj(lv_group_get_default(), cont);
    lv_menu_set_load_page_event(menu, cont, sub_rf_page);
    lv_obj_add_event_cb(cont, enter_subpage_cb, LV_EVENT_CLICKED, sub_rf_page);
    lv_obj_set_style_translate_x(cont, 100, 0);
    lv_obj_set_style_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_user_data(cont, (void *)0); // 标记未播放动画

    // section = lv_menu_section_create(root_page);
    cont = ui_create_text(root_page, &ui_img_usb, "USB传输设置");
    lv_group_add_obj(lv_group_get_default(), cont);
    lv_menu_set_load_page_event(menu, cont, sub_usb_page);
    lv_obj_add_event_cb(cont, enter_subpage_cb, LV_EVENT_CLICKED, sub_usb_page);
    lv_obj_set_style_translate_x(cont, 100, 0);
    lv_obj_set_style_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_user_data(cont, (void *)0); // 标记未播放动画

    // section = lv_menu_section_create(root_page);
    cont = ui_create_text(root_page, &ui_img_system_info, "系统信息", nullptr, true);
    lv_group_add_obj(lv_group_get_default(), cont);
    lv_menu_set_load_page_event(menu, cont, sub_system_page);
    lv_obj_add_event_cb(cont, enter_subpage_cb, LV_EVENT_CLICKED, sub_system_page);
    lv_obj_set_style_translate_x(cont, 100, 0);
    lv_obj_set_style_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_user_data(cont, (void *)0); // 标记未播放动画

    // section = lv_menu_section_create(root_page);
    cont = ui_create_text(root_page, &ui_img_about, "关于", nullptr, true);
    lv_menu_set_load_page_event(menu, cont, sub_about_page);
    lv_obj_add_event_cb(cont, enter_subpage_cb, LV_EVENT_CLICKED, sub_about_page);
    lv_obj_set_style_translate_x(cont, 100, 0);
    lv_obj_set_style_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_user_data(cont, (void *)0); // 标记未播放动画

    // TODO: 文件系统页面
    //  lv_menu_set_sidebar_page(menu, root_page);

    // lv_obj_send_event(lv_obj_get_child(lv_obj_get_child(lv_menu_get_cur_sidebar_page(menu), 0), 0), LV_EVENT_CLICKED,
    //                   NULL);
    lv_menu_set_page(menu, root_page);
    lv_obj_set_scroll_dir(root_page, LV_DIR_VER);
    lv_obj_send_event(root_page, LV_EVENT_SCROLL, NULL);
    lv_obj_scroll_to_view(lv_obj_get_child(root_page, 0), LV_ANIM_OFF);

    sys_info_timer = lv_timer_create(sys_info_timer_cb, SYSTEM_INFO_REFLUSH_TIME, NULL);
    lv_timer_pause(sys_info_timer);
}

// 设置页面back按钮回调
static void back_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target_obj(e);
    if (obj && lv_menu_back_button_is_root(menu, obj))
    {
        lv_obj_set_flag(last_widget, LV_OBJ_FLAG_HIDDEN, false);
        lv_obj_send_event(last_widget, (lv_event_code_t)(LV_EVENT_LAST + 1), NULL);
        lv_timer_delete(sys_info_timer);
        lv_obj_del(setting_widget);
    }
    else
    {
        // 返回到上一级（主页面），将焦点移回进入该子页的条目
        if (last_enter_btn && lv_obj_is_valid(last_enter_btn))
        {
            // 异步执行，确保菜单已完成页面切换
            lv_obj_t *page_obj = (lv_obj_t *)lv_obj_get_user_data(last_enter_btn);
            uint8_t p = (uint8_t)(intptr_t)lv_obj_get_user_data(page_obj);
            if (p == e_page::system_page)
            {
                lv_timer_pause(sys_info_timer);
            }
            else if (p == e_page::audio_page || p == e_page::rf_page || p == e_page::usb_page)
            {
                ui_popwin_msgbox("是否立即生效", nullptr, last_enter_btn, LV_SYMBOL_BELL, "请选择:", false, "是", [](lv_event_t *e)
                                 {
                                    uint8_t p = (uint8_t)(intptr_t) lv_event_get_user_data(e);
                                    save_config(p,true); }, (void *)(intptr_t)p, "否", [](lv_event_t *e)
                                 {
                                    uint8_t p = (uint8_t)(intptr_t) lv_event_get_user_data(e);
                                    save_config(p,false); }, (void *)(intptr_t)p);
            }

            lv_async_call(focus_async_cb, last_enter_btn);
        }
    }
}
// 重新连接蓝牙点击
static void relink_bt_cb(lv_event_t *e)
{
    lv_obj_del(setting_widget);
    ui_free_main_widget();
    ui_bt_init();
}

static lv_obj_t *ui_create_text(lv_obj_t *parent, const void *icon, const char *txt, lv_obj_t **label_o, bool is_from_svg)
{
    lv_obj_t *obj = lv_menu_cont_create(parent);
    lv_obj_t *img = NULL;
    lv_obj_t *label = NULL;

    // 菜单项样式
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xFFFFFF), 0);                // 默认背景
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xF0F0F0), LV_STATE_PRESSED); // 按下背景
    // lv_obj_set_style_bg_color(obj, lv_color_hex(0xE8E8E8), LV_STATE_FOCUS_KEY); // 焦点背景
    lv_obj_set_style_radius(obj, 8, 0);
    lv_obj_set_style_pad_all(obj, 5, 0);
    lv_obj_set_style_margin_bottom(obj, 2, 0); // 增加间距
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_shadow_width(obj, 10, 0);
    lv_obj_set_style_shadow_opa(obj, LV_OPA_10, 0);
    lv_obj_set_style_shadow_ofs_y(obj, 2, 0);

    if (icon)
    {
        img = lv_image_create(obj);
        lv_image_set_src(img, icon);
        if (is_from_svg)
        {
            lv_img_set_zoom(img, 32);
            lv_obj_set_size(img, 16, 16);
        }
    }

    if (txt)
    {
        label = lv_label_create(obj);
        lv_label_set_text(label, txt);
        lv_obj_set_style_text_color(label, lv_color_hex(0x333333), 0);
    }

    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_group_add_obj(lv_group_get_default(), obj);

    if (label_o)
    {
        *label_o = label;
    }
    return obj;
}
static lv_obj_t *ui_create_slider(lv_obj_t *parent, const void *icon, const char *txt, int32_t min, int32_t max,
                                  int32_t val, lv_obj_t **slider_obj)
{
    lv_obj_t *title;
    lv_obj_t *obj = ui_create_text(parent, icon, txt, &title);
    lv_group_remove_obj(obj);
    lv_obj_t *slider = lv_slider_create(obj);
    lv_obj_set_style_pad_all(slider, 0, LV_PART_KNOB);
    lv_obj_align_to(slider, title, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_set_size(slider, lv_pct(73), 20);
    lv_slider_set_range(slider, min, max);
    lv_slider_set_value(slider, val, LV_ANIM_OFF);

    lv_obj_t *pct = lv_label_create(slider);
    lv_label_set_text_fmt(pct, "%d%%", 0);
    lv_obj_set_style_text_color(pct, lv_color_hex(0x333333), 0);
    lv_obj_align(pct, LV_ALIGN_CENTER, 0, 0);

    lv_obj_add_event_cb(slider, [](lv_event_t *e)
                        {
        lv_obj_t * slider = (lv_obj_t*)lv_event_get_target(e);
        lv_obj_t* pct = (lv_obj_t*) lv_event_get_user_data(e);
        lv_label_set_text_fmt(pct,"%d",lv_slider_get_value(slider)); }, LV_EVENT_VALUE_CHANGED, pct);
    lv_obj_add_event_cb(slider, [](lv_event_t *e)
                        {
        lv_obj_t* target = lv_event_get_target_obj(e);
        lv_obj_t* parent = lv_obj_get_parent(target);
        lv_obj_scroll_to_view_recursive(parent,LV_ANIM_ON); }, LV_EVENT_FOCUSED, NULL);
    if (slider_obj)
    {
        *slider_obj = slider;
    }

    return obj;
}
// 创建下拉列表， options: apple\nbanana...
static lv_obj_t *ui_create_dropdown(lv_obj_t *parent, const void *icon, const char *txt, const char *options, lv_obj_t **dd_o)
{
    lv_obj_t *label;
    lv_obj_t *obj = ui_create_text(parent, icon, txt, &label);

    // 从焦点组中移除标签容器，使其无法被编码器选中
    lv_group_remove_obj(obj);

    lv_obj_t *dd = lv_dropdown_create(obj);
    lv_obj_add_style(dd, &scroll_style, LV_PART_SCROLLBAR);
    lv_dropdown_set_options(dd, options);
    lv_obj_set_size(dd, 85, 20);
    lv_obj_add_flag(dd, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_align_to(dd, label, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
    lv_group_add_obj(lv_group_get_default(), dd);

    lv_obj_t *list = lv_dropdown_get_list(dd);
    for (int i = 0; i < lv_obj_get_child_count_by_type(list, &lv_button_class); i++)
    {
        lv_obj_t *child = lv_obj_get_child_by_type(list, i, &lv_button_class);
        lv_obj_add_event_cb(child, [](lv_event_t *e)
                            {
            lv_obj_t* target = (lv_obj_t*)lv_event_get_target_obj(e);
            lv_obj_scroll_to_view_recursive(target,LV_ANIM_ON); }, LV_EVENT_FOCUSED, NULL);
    }

    lv_obj_add_event_cb(dd, [](lv_event_t *e)
                        {
        lv_obj_t *dd = lv_event_get_target_obj(e);
        lv_obj_t *container = lv_obj_get_parent(dd); 
        if (container && lv_obj_is_valid(container)) {
            lv_obj_scroll_to_view_recursive(container, LV_ANIM_ON);
        } }, LV_EVENT_FOCUSED, NULL);

    if (dd_o)
    {
        *dd_o = dd;
    }
    return obj;
}
static lv_obj_t *ui_create_sub_page(lv_obj_t *parent, const char *title, bool display_scroll)
{
    lv_obj_t *sub_page = lv_menu_page_create(parent, title);
    // lv_obj_set_style_pad_hor(sub_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(parent), LV_PART_MAIN), 0);
    // lv_menu_separator_create(sub_page);
    lv_obj_set_scroll_dir(sub_page, LV_DIR_VER);
    lv_obj_add_style(sub_page, &scroll_style, LV_PART_SCROLLBAR);
    if (display_scroll)
        lv_obj_set_scrollbar_mode(sub_page, LV_SCROLLBAR_MODE_AUTO);
    else
        lv_obj_set_scrollbar_mode(sub_page, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_event_cb(sub_page, scroll_event_cb, LV_EVENT_SCROLL, NULL);
    return sub_page;
}

// 记录进入子页的来源条目
static void enter_subpage_cb(lv_event_t *e)
{
    last_enter_btn = lv_event_get_target_obj(e);
    lv_obj_t *page = (lv_obj_t *)lv_event_get_user_data(e);
    uint8_t p = (uint8_t)(intptr_t)lv_obj_get_user_data(page);
    if (p == e_page::system_page)
    {
        lv_timer_resume(sys_info_timer);
    }
    else if (p == e_page::audio_page)
    {
        uint8_t l_bit = 16, l_channel = 2, l_volumn = 30, l_volumn_mode = USB_MODE_AUDIO;
        uint32_t l_rate = 48000;
        ui_setting_audio_page_rcb(l_bit, l_channel, l_rate, l_volumn, l_volumn_mode);

        int sel_bit = 0;
        switch (l_bit)
        {
        case 16:
            sel_bit = 0;
            break; // 16bit
        case 24:
            sel_bit = 1;
            break;
        case 32:
            sel_bit = 2;
            break;
        }
        lv_dropdown_set_selected(dd_audio_bit, sel_bit);

        int sel_channel = (l_channel == 2) ? 1 : 0; // 0: 单通道, 1: 立体声
        lv_dropdown_set_selected(dd_audio_channel, sel_channel);

        int sel_rate = 0;
        switch (l_rate)
        {
        case 48000:
            sel_rate = 0;
            break;
        case 96000:
            sel_rate = 1;
            break;
        case 192000:
            sel_rate = 2;
            break;
        }
        lv_dropdown_set_selected(dd_audio_rate, sel_rate);

        int sel_vol_mode = 0;
        switch (l_volumn_mode)
        {
        case 0:
            sel_vol_mode = 0;
            break; // 自动增益
        case 1:
            sel_vol_mode = 1;
            break; // 峰值减少
        case 2:
            sel_vol_mode = 2;
            break; // 手动
        }
        lv_dropdown_set_selected(dd_audio_volumn_mode, sel_vol_mode);

        if (slider_audio_volumn)
        {
            lv_slider_set_value(slider_audio_volumn, l_volumn, LV_ANIM_OFF);
        }
    }
    else if (p == e_page::usb_page)
    {
        USBMode mode = USB_MODE_NONE;
        ui_setting_usb_page_rcb(mode);
        int sel = 0;
        switch (mode)
        {
        case USB_MODE_NONE:
            sel = 0;
            break;
        case USB_MODE_AUDIO:
            sel = 1;
            break;
        case USB_MODE_SD:
            sel = 2;
            break;
        case USB_MODE_JTAG:
            sel = 3;
            break;
        }
        lv_dropdown_set_selected(dd_usb_mode, sel);
    }
    else if (p == e_page::rf_page)
    {
        bool mode = false; // false: BLE, true: WIFI
        ui_setting_rf_page_rcb(mode);
        lv_dropdown_set_selected(dd_rf_mode, mode ? 1 : 0);
    }

    lv_obj_set_user_data(last_enter_btn, page); // 记录btn对应的page
    lv_obj_send_event(page, LV_EVENT_SCROLL, NULL);
    lv_obj_scroll_to_view(lv_obj_get_child(page, 0), LV_ANIM_OFF);
}

// 异步把焦点放回来源条目（并滚动可见）
static void focus_async_cb(void *obj_p)
{
    lv_obj_t *obj = (lv_obj_t *)obj_p;
    if (obj && lv_obj_is_valid(obj))
    {
        lv_group_focus_obj(obj);
        lv_obj_scroll_to_view(obj, LV_ANIM_ON);
        lv_obj_add_state(obj, LV_STATE_FOCUS_KEY); // 使返回焦点正常显示
    }
}

static void sys_info_timer_cb(lv_timer_t *)
{
    float l_cpu1, l_cpu2;
    size_t l_iram, l_psram, l_iram_max, l_psram_max;

    ui_setting_system_page_rcb(l_cpu1, l_cpu2, l_iram, l_psram, l_iram_max, l_psram_max);
    lv_label_set_text_fmt(cpu1, "CPU1   #626367 %.2f%%#", l_cpu1);
    lv_label_set_text_fmt(cpu2, "CPU2   #626367 %.2f%%#", l_cpu2);
    lv_label_set_text_fmt(iram, "IRAM   #626367 %d/%dKB#", l_iram / 1024, l_iram_max / 1024);
    lv_label_set_text_fmt(psram, "PSRAM   #626367 %d/%dKB#", l_psram / 1024, l_psram_max / 1024);
    // lv_label_set_text_fmt(sd, "SD   #626367 %dbit#", l_sd);
}
// 返回按钮焦点事件回调
static void back_btn_focus_cb(lv_event_t *e)
{
    lv_obj_t *back_icon = lv_obj_get_child(lv_event_get_target_obj(e), 0);
    if (!back_icon)
        return;
    if (lv_event_get_code(e) == LV_EVENT_FOCUSED)
        lv_obj_set_style_text_color(back_icon, lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN);
    else
        lv_obj_set_style_text_color(back_icon, lv_color_black(), LV_PART_MAIN);
}
static void scroll_event_cb(lv_event_t *e)
{
    lv_obj_t *cont = lv_event_get_target_obj(e);

    // 只对root_page应用滑入动画
    if (cont == root_page_ref)
    {
        lv_area_t cont_a;
        lv_obj_get_coords(cont, &cont_a);
        int32_t cont_bottom = cont_a.y2;
        int32_t cont_top = cont_a.y1;

        int32_t child_cnt = (int32_t)lv_obj_get_child_count(cont);
        for (int32_t i = 0; i < child_cnt; i++)
        {
            lv_obj_t *child = lv_obj_get_child(cont, i);
            lv_area_t child_a;
            lv_obj_get_coords(child, &child_a);

            // 当子项离开可视区域时，重置状态
            if (child_a.y1 > cont_bottom || child_a.y2 < cont_top)
            {
                lv_obj_set_user_data(child, (void *)0);
                lv_obj_set_style_translate_x(child, 100, 0);
                lv_obj_set_style_opa(child, LV_OPA_TRANSP, 0);
            }
            // 当子项进入可视区域时触发动画
            else if (child_a.y1 <= cont_bottom && child_a.y2 >= cont_top)
            {
                // 检查是否已播放过动画
                if (lv_obj_get_user_data(child) == (void *)1)
                    continue;

                lv_obj_set_user_data(child, (void *)1);

                lv_anim_t a;
                lv_anim_init(&a);
                lv_anim_set_var(&a, child);
                lv_anim_set_values(&a, 100, 0);
                lv_anim_set_duration(&a, 300);
                lv_anim_set_exec_cb(&a, [](void *var, int32_t v)
                                    { lv_obj_set_style_translate_x((lv_obj_t *)var, v, 0); });
                lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
                lv_anim_start(&a);

                // 透明度动画
                lv_anim_set_exec_cb(&a, [](void *var, int32_t v)
                                    { lv_obj_set_style_opa((lv_obj_t *)var, v, 0); });
                lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
                lv_anim_start(&a);
            }
        }
    }
    else
    {
        // 其他页面保持原有的滚轮效果
        lv_area_t cont_a;
        lv_obj_get_coords(cont, &cont_a);
        int32_t cont_y_center = cont_a.y1 + lv_area_get_height(&cont_a) / 2;

        int32_t r = lv_obj_get_height(cont) * 7 / 10;
        int32_t i;
        int32_t child_cnt = (int32_t)lv_obj_get_child_count(cont);
        for (i = 0; i < child_cnt; i++)
        {
            lv_obj_t *child = lv_obj_get_child(cont, i);
            lv_area_t child_a;
            lv_obj_get_coords(child, &child_a);

            int32_t child_y_center = child_a.y1 + lv_area_get_height(&child_a) / 2;

            int32_t diff_y = child_y_center - cont_y_center;
            diff_y = LV_ABS(diff_y);

            /*Get the x of diff_y on a circle.*/
            int32_t x;
            /*If diff_y is out of the circle use the last point of the circle (the radius)*/
            if (diff_y >= r)
            {
                x = r;
            }
            else
            {
                /*Use Pythagoras theorem to get x from radius and y*/
                uint32_t x_sqr = r * r - diff_y * diff_y;
                lv_sqrt_res_t res;
                lv_sqrt(x_sqr, &res, 0x8000); /*Use lvgl's built in sqrt root function*/
                x = r - res.i;
            }

            /*Translate the item by the calculated X coordinate*/
            lv_obj_set_style_translate_x(child, x, 0);

            /*Use some opacity with larger translations*/
            lv_opa_t opa = (lv_opa_t)lv_map(x, 0, r, LV_OPA_TRANSP, LV_OPA_COVER);
            lv_obj_set_style_opa(child, LV_OPA_COVER - opa, 0);
        }
    }
}

static void save_config(uint8_t &p, bool now)
{
    switch (p)
    {
    case e_page::usb_page:
        switch (lv_dropdown_get_selected(dd_usb_mode))
        {
        case 0:
            ui_setting_usb_page_scb(USB_MODE_NONE, now);
            break;
        case 1:
            ui_setting_usb_page_scb(USB_MODE_AUDIO, now);
            break;
        case 2:
            ui_setting_usb_page_scb(USB_MODE_SD, now);
            break;
        case 3:
            ui_setting_usb_page_scb(USB_MODE_JTAG, now);
            break;
        }
        break;
    case e_page::audio_page:
        uint8_t v_bit, v_channel, v_volumn, v_volumn_mode;
        uint32_t v_rate;

        switch (lv_dropdown_get_selected(dd_audio_bit))
        {
        case 0:
            v_bit = 16;
            break;
        case 1:
            v_bit = 24;
            break;
        case 2:
            v_bit = 32;
            break;
        }
        switch (lv_dropdown_get_selected(dd_audio_channel))
        {
        case 0:
            v_channel = 1;
            break;
        case 1:
            v_channel = 2;
            break;
        }
        switch (lv_dropdown_get_selected(dd_audio_rate))
        {
        case 0:
            v_rate = 48000;
            break;
        case 1:
            v_rate = 96000;
            break;
        case 2:
            v_rate = 192000;
            break;
        }
        switch (lv_dropdown_get_selected(dd_audio_volumn_mode))
        {
        case 0:
            v_volumn_mode = 0;
            break;
        case 1:
            v_volumn_mode = 1;
            break;
        case 2:
            v_volumn_mode = 2;
            break;
        }

        v_volumn = lv_slider_get_value(slider_audio_volumn);
        ui_setting_audio_page_scb(v_bit, v_channel, v_rate, v_volumn, v_volumn_mode, now);
        break;
    case e_page::rf_page:
        ui_setting_rf_page_scb(lv_dropdown_get_selected(dd_rf_mode) == 1, now);
        break;
    }
}