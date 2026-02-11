#pragma once

#include "Arduino.h"
#include "esp_log.h"

#define LOGGER_TAG (__FILE_NAME__)

#define LOGGER_DEBUG(msg, ...) ESP_LOGD(LOGGER_TAG, msg, ##__VA_ARGS__)
#define LOGGER_INFO(msg, ...) ESP_LOGI(LOGGER_TAG, msg, ##__VA_ARGS__)
#define LOGGER_WARN(msg, ...) ESP_LOGW(LOGGER_TAG, msg, ##__VA_ARGS__)
#define LOGGER_ERROR_MAX_LEN 256
#define LOGGER_ERROR(msg, ...)                            \
  do                                                      \
  {                                                       \
    ESP_LOGE(LOGGER_TAG, msg, ##__VA_ARGS__);             \
    char buffer[LOGGER_ERROR_MAX_LEN];                    \
    snprintf(buffer, sizeof(buffer), msg, ##__VA_ARGS__); \
    logger::error(buffer);                                \
  } while (0)

namespace logger
{
  void setup();

  // 当遇到严重错误时调用此函数
  void error(char *str);
}
