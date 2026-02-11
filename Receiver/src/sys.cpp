#include "logger.h"
#include "config.h"
#include "sys.h"
#include "ui/ui.h"

SystemInfo systemInfo;

/* 每秒定时任务 */
static void system_handle(void *arg)
{
  // 记录CPU信息
  TaskStatus_t *tasks = NULL;
  UBaseType_t tasks_size;
  uint32_t tasktime;

  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(TASK_SYSTEM_PERIOD);
  while (true)
  {
    // 精准延时
    xTaskDelayUntil(&xLastWakeTime, xFrequency);

    // 写入系统信息
    // CPU信息
    tasks_size = uxTaskGetNumberOfTasks();
    tasks = (TaskStatus_t *)malloc(sizeof(TaskStatus_t) * tasks_size);
    tasks_size = uxTaskGetSystemState(tasks, tasks_size, &tasktime);
#ifdef SYSTEM_PRINT_INFORMATION
    logger::debugln("CPU info:\n");
    logger::debugln("  | Task | Percentage | Stack High |");
#endif
    for (int i = 0; i < tasks_size; i++)
    {
#ifdef SYSTEM_PRINT_INFORMATION
      logger::debugln("  | %s | %F | %d |", tasks[i].pcTaskName, tasks[i].ulRunTimeCounter * 100.0 / tasktime, uxTaskGetStackHighWaterMark(tasks[i].xHandle));
#endif
      // 找到空闲任务
      if (strcmp(tasks[i].pcTaskName, "IDLE0") == 0)
      {
        systemInfo.cpu0Usage = 100.0 - (tasks[i].ulRunTimeCounter * 100.0 / tasktime);
      }
      else if (strcmp(tasks[i].pcTaskName, "IDLE1") == 0)
      {
        systemInfo.cpu1Usage = 100.0 - (tasks[i].ulRunTimeCounter * 100.0 / tasktime);
      }
    }
    free(tasks);
#ifdef SYSTEM_PRINT_INFORMATION
    logger::debugln("  CPU0:%F%, CPU1:%F%", systemInfo.cpu0Usage, systemInfo.cpu1Usage);
#endif

    // IRAM内存信息
    multi_heap_info_t heapInfo;
    heap_caps_get_info(&heapInfo, MALLOC_CAP_INTERNAL);
    systemInfo.iramUsedSize = heapInfo.total_allocated_bytes;
    systemInfo.iramTotalSize = heapInfo.total_allocated_bytes + heapInfo.total_free_bytes;

    // PSRAM内存信息
    heap_caps_get_info(&heapInfo, MALLOC_CAP_SPIRAM);
    systemInfo.psramUsedSize = heapInfo.total_allocated_bytes;
    systemInfo.psramTotalSize = heapInfo.total_allocated_bytes + heapInfo.total_free_bytes;

#ifdef SYSTEM_PRINT_INFORMATION
    logger::debugln("Memory Info:");
    logger::debugln("  IRAM: %d/%dKB", systemInfo.iramUsedSize / 1024, systemInfo.iramTotalSize / 1024);
    logger::debugln("  PSRAM: %d/%dKB", systemInfo.psramUsedSize / 1024, systemInfo.psramTotalSize / 1024);
#endif
  }
}

void sys::setup()
{
  esp_reset_reason_t reason = esp_reset_reason();
  switch (reason)
  {
  case ESP_RST_PANIC: //!< Software reset due to exception/panic
  {
    break;
  }

  case ESP_RST_INT_WDT:  //!< Reset (software or hardware) due to interrupt watchdog
  case ESP_RST_TASK_WDT: //!< Reset due to task watchdog
  case ESP_RST_WDT:      //!< Reset due to other watchdogs
  {
    break;
  }

  case ESP_RST_UNKNOWN: //!< Reset reason can not be determined
  {
    break;
  }

  case ESP_RST_EFUSE: //!< Reset due to efuse error
  {
    break;
  }

  case ESP_RST_PWR_GLITCH: //!< Reset due to power glitch detected
  {
    break;
  }

  case ESP_RST_CPU_LOCKUP: //!< Reset due to CPU lock up (double exception)
  {
    break;
  }

  // 不需要提示
  case ESP_RST_POWERON:   //!< Reset due to power-on event
  case ESP_RST_EXT:       //!< Reset by external pin (not applicable for ESP32)
  case ESP_RST_SW:        //!< Software reset via esp_restart
  case ESP_RST_DEEPSLEEP: //!< Reset after exiting deep sleep mode
  case ESP_RST_BROWNOUT:  //!< Brownout reset (software or hardware)
  case ESP_RST_SDIO:      //!< Reset over SDIO
  case ESP_RST_USB:       //!< Reset by USB peripheral
  case ESP_RST_JTAG:      //!< Reset by JTAG
  default:
  {
    break;
  }
  }
  xTaskCreatePinnedToCore(system_handle, "system_handle", TASK_SYSTEM_STACK, NULL, TASK_SYSTEM_PRIORITY, NULL, TASK_SYSTEM_CORE);
}

/*****************************
            界面
*****************************/
// 界面读取系统信息
void ui_setting_system_page_rcb(float &cpu1_pct, float &cpu2_pct, size_t &iram_current, size_t &psram_current, size_t &l_iram_max, size_t &l_psram_max)
{
  cpu1_pct = systemInfo.cpu0Usage;
  cpu2_pct = systemInfo.cpu1Usage;
  iram_current = systemInfo.iramUsedSize;
  psram_current = systemInfo.psramUsedSize;
  l_iram_max = systemInfo.iramTotalSize;
  l_psram_max = systemInfo.psramTotalSize;
}
/****************************/