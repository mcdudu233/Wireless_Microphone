#pragma once

#define SLEEP_WAIT_TIME (10 * 60 * 1000) // 自动睡眠时间 10分钟

#define BUTTON_IO GPIO_NUM_9
#define BUTTON_SHUTDOWN_TIME (2 * 1000) // 长按2秒钟关机
#define BUTTON_POWERON_TIME (1 * 1000)  // 长按1秒钟开机
#define BUTTON_WAIT_TIME (1 * 1000)     // 防止唤醒时间

#define CHARGING_IO GPIO_NUM_21
#define BATTERY_MAX 4.2                 // 电池最高电压
#define BATTERY_MIN 3.6                 // 电池最低电压
#define BATTERY_NOTIFY_LOW_PERCENT 20.0 // 电量提示过低百分比
#define BATTERY_ALERT_LOW_PERCENT 5.0   // 电量警告过低百分比
#define BATTERY_ALERT_LOW_FREQUENCY 5   // 电量警告片频率
#define BATTERY_LOW_PERCENT 3.0         // 电量过低百分比

#define ADC_BAT_IO GPIO_NUM_1
#define ADC_VCC_IO GPIO_NUM_4
#define ADC_POWER_SUPPLY 4.3 // 判断USB还是电池供电的分界线 4.3V

namespace power
{
  void setup();

  // 内核复位
  void core_restart();
  // CPU复位
  void cpu_restart();
  // 深度睡眠
  void deepSleep(bool withLight = true);

  // 获取电压
  double getVCCVoltage();
  double getBATVoltage();
  // 获取供电方式
  bool isUSBSupply();
  bool isBATSupply();
  // 是否正在充电
  bool isCharging();
  // 获取电量
  double getBATPercent();
}