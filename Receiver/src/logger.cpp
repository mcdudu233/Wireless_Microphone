#include "logger.h"

void logger::setup()
{
  // 根据构建类型设置日志级别
#if defined(BUILD_RELEASE)
  esp_log_level_set("*", ESP_LOG_WARN);
#elif defined(BUILD_DEBUG)
  esp_log_level_set("*", ESP_LOG_INFO);
#endif

  // 重定向 ESP-IDF 日志输出
  // esp_log_set_vprintf(esp_apptrace_vprintf);
  // Log.begin(LOG_LEVEL_VERBOSE, &USBCDCSerial);

  LOGGER_INFO("Logger is started!");
}

void logger::error(char *str)
{
  // TODO: 程序遇到了严重错误 显示屏提示
}