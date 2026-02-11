#include "config.h"
#include "logger.h"
#include "module/screen.h"
#include "module/button.h"
#include "ui/ui_loading.h"

#include "lvgl.h"
#include "esp_lcd_io_spi.h"
#include "esp_lcd_st7735s.h"
#include "esp_lvgl_port.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// 屏幕接口
static esp_lcd_panel_io_handle_t io_handle = NULL;
static esp_lcd_panel_handle_t lcd_panel_handle;

// 按键回调
static bool left = false;
static bool right = false;
static bool ok = false;
static void button_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
  // 往左往右
  if (button::left() && !left)
  {
    left = true;
    data->enc_diff = -1;
  }
  else if (!button::left() && left)
  {
    left = false;
    data->enc_diff = 0;
  }
  else if (button::right() && !right)
  {
    right = true;
    data->enc_diff = 1;
  }
  else if (!button::right() && right)
  {
    right = false;
    data->enc_diff = 0;
  }
  // 按下
  if (button::ok() && !ok)
  {
    ok = true;
    data->state = LV_INDEV_STATE_PRESSED;
  }
  else if (!button::ok() && ok)
  {
    ok = false;
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

// void log_cb(lv_log_level_t level, const char * buf)
// {
//   LOGGER_INFO("%s", buf);
// }

void screen::backlight(float percent)
{
  ledcWrite(TFT_BLK, (int)(percent / 100 * (pow(2.0, TFT_BLK_BIT) - 1)));
}

// LVGL 互斥锁
static SemaphoreHandle_t lv_mutex;
bool screen::lv_lock()
{
  return lvgl_port_lock(1);
}
void screen::lv_lock_wait()
{
  lvgl_port_lock(0);
}
void screen::lv_unlock()
{
  lvgl_port_unlock();
}

// 初始化显示屏
static void setup_lcd()
{
  esp_err_t ret;

  // 初始化SPI
  spi_bus_config_t spi_cfg = {
      .mosi_io_num = TFT_MOSI,
      .miso_io_num = GPIO_NUM_NC,
      .sclk_io_num = TFT_CLK,
      .quadwp_io_num = GPIO_NUM_NC,
      .quadhd_io_num = GPIO_NUM_NC,
      .max_transfer_sz = TFT_HOR_RES * TFT_VER_RES * sizeof(uint16_t),
  };
  ret = spi_bus_initialize(TFT_SPI_NUM, &spi_cfg, SPI_DMA_CH_AUTO);
  if (ret != ESP_OK)
  {
    LOGGER_WARN("LCD SPI init failed! reason=%d", ret);
    return;
  }

  // 初始化LCD驱动
  const esp_lcd_panel_io_spi_config_t io_config = {
      .cs_gpio_num = TFT_CS,
      .dc_gpio_num = TFT_DC,
      .spi_mode = 0,
      .pclk_hz = TFT_SPI_FREQ,
      .trans_queue_depth = 10,
      .lcd_cmd_bits = 8,
      .lcd_param_bits = 8,
  };
  ret = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)1, &io_config, &io_handle);
  if (ret != ESP_OK)
  {
    LOGGER_WARN("LCD IO init failed! reason=%d", ret);
    return;
  }
  const esp_lcd_panel_dev_config_t panel_config = {
      .reset_gpio_num = TFT_RST,
      .color_space = LCD_RGB_ELEMENT_ORDER_BGR,
      .bits_per_pixel = 16,
  };
  ret = esp_lcd_new_panel_st7735s(io_handle, &panel_config, &lcd_panel_handle);
  if (ret != ESP_OK)
  {
    LOGGER_WARN("LCD Driver init failed! reason=%d", ret);
    return;
  }
  ret = esp_lcd_panel_reset(lcd_panel_handle);
  if (ret != ESP_OK)
  {
    LOGGER_WARN("LCD Driver init failed! reason=%d", ret);
    return;
  }
  ret = esp_lcd_panel_init(lcd_panel_handle);
  if (ret != ESP_OK)
  {
    LOGGER_WARN("LCD Driver init failed! reason=%d", ret);
    return;
  }

  // 初始化背光
  LOGGER_INFO("TFT backlight now starting to init.");
  ledcAttach(TFT_BLK, TFT_BLK_FREQ, TFT_BLK_BIT);
  screen::backlight(0);
  LOGGER_INFO("TFT backlight is inited.");
}

// 初始化图形库
static void setup_lvgl()
{
  esp_err_t ret;

  // 初始化LVGL
  const lvgl_port_cfg_t lvgl_cfg = {
      .task_priority = TASK_SCREEN_PRIORITY, /* LVGL task priority */
      .task_stack = TASK_SCREEN_STACK,       /* LVGL task stack size */
      .task_affinity = TASK_SCREEN_CORE,     /* LVGL task pinned to core (-1 is no affinity) */
      .task_max_sleep_ms = 500,              /* Maximum sleep in LVGL task */
      .timer_period_ms = TASK_SCREEN_PERIOD  /* LVGL timer tick period in ms */
  };
  ret = lvgl_port_init(&lvgl_cfg);
  if (ret != ESP_OK)
  {
    LOGGER_WARN("LVGL init failed!");
    return;
  }

  // 初始化屏幕
  const lvgl_port_display_cfg_t disp_cfg = {
      .io_handle = io_handle,
      .panel_handle = lcd_panel_handle,
      .buffer_size = TFT_HOR_RES * TFT_VER_RES,
      .double_buffer = true,
      .trans_size = TFT_HOR_RES * TFT_VER_RES,
      .hres = TFT_HOR_RES,
      .vres = TFT_VER_RES,
      .monochrome = false,
      .rotation = {
          .swap_xy = true,
          .mirror_x = true,
          .mirror_y = false,
      },
      .color_format = LV_COLOR_FORMAT_RGB565,
      .flags = {
          .buff_dma = true,
          .buff_spiram = true,
          .swap_bytes = true,
      }};
  lvgl_port_add_disp(&disp_cfg);

  // 初始化输入设备
  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_ENCODER);
  lv_indev_set_read_cb(indev, button_cb);
  lv_group_t *group = lv_group_create();
  lv_group_set_default(group);
  lv_indev_set_group(indev, group);

  LOGGER_INFO("LVGL is started.");
}

// 屏幕渐亮线程
static void screen_backlight_on_handle(void *arg)
{
  // 先等待屏幕控件加载再渐亮
  vTaskDelay(pdMS_TO_TICKS(200));
  // 屏幕渐亮
  for (int i = 0; i <= 50; i++)
  {
    screen::backlight(i);
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  vTaskDelete(NULL);
}

void screen::setup()
{
  LOGGER_INFO("Screen is starting...");
  setup_lcd();
  setup_lvgl();

  // 创建图形库处理线程
  xTaskCreatePinnedToCore(screen_backlight_on_handle, "screen_backlight_on_handle", TASK_SCREEN_STACK, NULL, TASK_SCREEN_PRIORITY, NULL, TASK_SCREEN_CORE);

  LV_LOCK();
  ui_loading_init();
  LV_UNLOCK();

  LOGGER_INFO("Screen is started.");
}

/*****************************
       LVGL 内存分配函数
      采用 PSRAM 分配内存
*****************************/
#if LV_USE_STDLIB_MALLOC == LV_STDLIB_CUSTOM

void lv_mem_init(void)
{
  return; /*Nothing to init*/
}

void lv_mem_deinit(void)
{
  return; /*Nothing to deinit*/
}

lv_mem_pool_t lv_mem_add_pool(void *mem, size_t bytes)
{
  /*Not supported*/
  LV_UNUSED(mem);
  LV_UNUSED(bytes);
  return NULL;
}

void lv_mem_remove_pool(lv_mem_pool_t pool)
{
  /*Not supported*/
  LV_UNUSED(pool);
  return;
}

void *lv_malloc_core(size_t size)
{
  return heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

void *lv_realloc_core(void *p, size_t new_size)
{
  return heap_caps_realloc(p, new_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

void lv_free_core(void *p)
{
  heap_caps_free(p);
}

void lv_mem_monitor_core(lv_mem_monitor_t *mon_p)
{
  /*Not supported*/
  LV_UNUSED(mon_p);
  return;
}

lv_result_t lv_mem_test_core(void)
{
  /*Not supported*/
  return LV_RESULT_OK;
}

#endif /*LV_STDLIB_CLIB*/
