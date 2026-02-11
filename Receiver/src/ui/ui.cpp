#include "ui/ui.h"

lv_obj_t *pop_win;

typedef struct
{
    lv_group_t *g;
    lv_obj_t *cont;
    lv_obj_t *obj;
} back_val;
// 创建一个基础控件容器
lv_obj_t *ui_add_win()
{
    lv_obj_t *widget = lv_obj_create(lv_screen_active());
    lv_obj_set_size(widget, WIDGET_H, WIDGET_V);
    lv_obj_set_style_pad_all(widget, 0, 0); // 去除内边距
    return widget;
}
// 添加带标题按钮
lv_obj_t *ui_add_button(lv_obj_t *parent, std::string title, int32_t w, int32_t h, const lv_font_t *font)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_t *label = lv_label_create(btn);
    lv_obj_set_size(btn, w, h);
    // lv_obj_set_size(label,w,h);
    lv_label_set_text(label, title.c_str());
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    if (font != NULL)
    {
        lv_obj_set_style_text_font(label, font, 0);
    }
    return btn;
}

// 添加列表按钮
lv_obj_t *ui_add_list_obj(lv_obj_t *list, std::string content, lv_event_cb_t cb, const lv_font_t *font, lv_palette_t bg_color)
{
    lv_obj_t *btn = lv_list_add_button(list, NULL, content.c_str());
    lv_obj_set_style_pad_all(btn, 0, 0);

    // lv_obj_set_style_pad_all(btn, 1, 0);

    // lv_obj_set_size(btn,lv_pct(90),15);
    if (font != NULL)
    {
        lv_obj_set_style_text_font(btn, font, 0);
    }
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    if (bg_color != LV_PALETTE_NONE)
    {
        lv_obj_set_style_bg_color(btn, lv_palette_main(bg_color), 0);
    }
    return btn;
}

// 将指定分组绑定到所有编码器输入设备
void ui_bind_group_to_all_encoders(lv_group_t *g)
{
    lv_indev_t *indev = NULL;
    while ((indev = lv_indev_get_next(indev)) != NULL)
    {
        if (lv_indev_get_type(indev) == LV_INDEV_TYPE_ENCODER)
        {
            lv_indev_set_group(indev, g);
        }
    }
}
void ui_set_opa(void *obj, int32_t val)
{
    lv_obj_set_style_opa((lv_obj_t *)obj, val, 0);
}
void ui_set_bar_val(void *bar, int32_t val)
{
    lv_bar_set_value((lv_obj_t *)bar, val, LV_ANIM_ON);
}

lv_obj_t **ui_popwin(bool has_bg, lv_group_t *g, lv_obj_t *obj)
{
    lv_obj_t *cont = lv_obj_create(lv_screen_active());
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_size(cont, lv_pct(100), lv_pct(100));
    lv_obj_align(cont, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_50, 0);
    lv_obj_set_style_bg_color(cont, lv_color_black(), 0);
    lv_obj_set_flag(cont, LV_OBJ_FLAG_CLICK_FOCUSABLE, true);
    lv_obj_set_flag(cont, LV_OBJ_FLAG_SCROLLABLE, false);

    if (lv_obj_is_valid(pop_win))
    {
        lv_obj_del(pop_win);
        pop_win = nullptr;
    }
    pop_win = cont;

    // 弹窗生命周期结束时清理全局指针，避免悬空引用
    lv_obj_add_event_cb(cont, [](lv_event_t *e)
                        {
        if(lv_event_get_code(e) == LV_EVENT_DELETE)
        {
            if (pop_win == lv_event_get_target_obj(e)) pop_win = nullptr;
        } }, LV_EVENT_DELETE, NULL);

    lv_obj_t *win = lv_obj_create(cont);
    lv_obj_set_style_pad_all(win, 0, 0);
    lv_obj_set_size(win, lv_pct(60), lv_pct(67));
    lv_obj_align(win, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_flag(win, LV_OBJ_FLAG_SCROLLABLE, false);
    if (!has_bg)
    {
        lv_obj_set_style_bg_opa(win, 0, 0);
        lv_obj_set_style_border_width(win, 0, 0);
    }

    back_val *v = (back_val *)lv_malloc(sizeof(back_val));
    if (g == nullptr)
    {
        g = lv_group_get_default();
    }
    v->g = g;
    v->obj = obj;
    lv_obj_set_user_data(cont, v);

    lv_group_t *new_g = lv_group_create();
    lv_group_add_obj(new_g, cont);
    // lv_group_set_default(new_g);
    ui_bind_group_to_all_encoders(new_g);
    lv_group_focus_obj(cont);

    lv_obj_add_event_cb(cont, [](lv_event_t *e)
                        {
        lv_obj_t* target = lv_event_get_target_obj(e);
        back_val* v = (back_val *)lv_obj_get_user_data(target);
        lv_group_set_default(v->g);
        ui_bind_group_to_all_encoders(v->g);
        if(lv_obj_is_valid(v->obj)) lv_group_focus_obj(v->obj);
        if (lv_obj_is_valid(target)) lv_obj_del(target);
        if (pop_win == target) pop_win = nullptr;
        lv_free(v); }, LV_EVENT_CLICKED, NULL);
    static lv_obj_t *ret[2];
    ret[0] = cont;
    ret[1] = win;

    return ret;
}

lv_obj_t *ui_popwin_msgbox(const char *text, lv_group_t *g, lv_obj_t *obj, const void *icon, const char *title, bool is_from_svg, const char *btn1_title, lv_event_cb_t event_cb1, void *user_data1, const char *btn2_title, lv_event_cb_t event_cb2, void *user_data2)
{
    lv_obj_t **ret = ui_popwin(true, g, obj);
    lv_obj_t *label;

    lv_obj_set_size(ret[1], 120, 60);
    // 图标
    if (icon)
    {
        lv_obj_t *img = lv_image_create(ret[1]);
        lv_image_set_src(img, icon);
        if (is_from_svg)
        {
            lv_img_set_zoom(img, 32);
            lv_obj_set_size(img, 16, 16);
        }
        lv_obj_align(img, LV_ALIGN_TOP_LEFT, 0, 0);
    }

    // 标题
    label = lv_label_create(ret[1]);
    lv_label_set_text(label, title);
    lv_obj_set_style_text_font(label, &lv_font_harmonyos_12, 0);
    lv_obj_align(label, LV_ALIGN_OUT_RIGHT_MID, 21, 0);

    static lv_style_t style_line;
    lv_style_init(&style_line);
    lv_style_set_line_width(&style_line, 1);
    lv_style_set_line_color(&style_line, lv_color_hex(333333));
    lv_style_set_line_rounded(&style_line, true);
    static lv_point_precise_t line_points[] = {{3, 0}, {157, 0}};

    lv_obj_t *line;

    line = lv_line_create(ret[1]);
    lv_line_set_points(line, line_points, 2);
    lv_obj_add_style(line, &style_line, 0);
    lv_obj_align(line, LV_ALIGN_TOP_LEFT, 0, 17);

    // content
    label = lv_label_create(ret[1]);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 3);
    lv_label_set_text(label, text);
    lv_obj_set_size(label, lv_pct(90), 30);
    lv_obj_set_style_text_font(label, &lv_font_harmonyos_12, 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_WRAP);
    if (btn1_title || btn2_title)
    {
        lv_obj_set_size(ret[1], 120, 75);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, -2);

        lv_group_t *g = lv_group_create();
        ui_bind_group_to_all_encoders(g);

        lv_obj_t *btn;
        lv_obj_t *btn1;
        lv_obj_t *btn2;
        if (btn1_title)
        {
            btn = ui_add_button(ret[1], btn1_title, 30, 16, &lv_font_harmonyos_12);
            lv_group_add_obj(g, btn);
            lv_group_focus_obj(btn);
            lv_obj_add_event_cb(btn, [](lv_event_t *e)
                                {
                                    lv_obj_t *target = (lv_obj_t *)lv_event_get_user_data(e);
                                    if (target && lv_obj_is_valid(target))
                                        lv_obj_send_event(target, LV_EVENT_CLICKED, NULL); }, LV_EVENT_CLICKED, ret[0]);
            lv_obj_add_event_cb(btn, event_cb1, LV_EVENT_CLICKED, user_data1);
            btn1 = btn;
        }

        if (btn2_title)
        {
            btn = ui_add_button(ret[1], btn2_title, 30, 16, &lv_font_harmonyos_12);
            lv_group_add_obj(g, btn);

            lv_obj_add_event_cb(btn, [](lv_event_t *e)
                                {
                                lv_obj_t *target = (lv_obj_t *)lv_event_get_user_data(e);
                                if (target && lv_obj_is_valid(target))
                                    lv_obj_send_event(target, LV_EVENT_CLICKED, NULL); }, LV_EVENT_CLICKED, ret[0]);
            lv_obj_add_event_cb(btn, event_cb2, LV_EVENT_CLICKED, user_data2);
            btn2 = btn;
        }
        if (btn1_title and btn2_title)
        {
            lv_obj_align(btn1, LV_ALIGN_BOTTOM_MID, -20, -2);
            lv_obj_align(btn2, LV_ALIGN_BOTTOM_MID, 20, -2);
        }
        else
            lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -2);
    }

    return ret[0];
}