#include "logger.h"
#include "config.h"
#include "module/power.h"
#include "module/led.h"

#include "driver/rtc_io.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

static void power_handle(void *arg)
{
  // 按钮状态
  bool buttonDown = false;
  unsigned long buttonLastTime;
  uint32_t buttonLastRGB;

  // 电池状态
  bool batterySupply = false;
  bool batteryCharging = false;
  double batteryPercent = 100.0;
  bool batteryCorrected = false;
  bool batteryNotify = false;
  uint32_t batteryNotifyLastRGB;
  bool batteryAlert = false;
  uint32_t batteryAlertLastRGB;
  uint8_t batteryAlertNumber = 0;
  bool batteryAlertBool = 0;

  // 待机状态
  unsigned long waitLastTime = millis();

  // 等待一会才启动
  delay(1000);

  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(TASK_POWER_PERIOD);
  while (true)
  {
    xTaskDelayUntil(&xLastWakeTime, xFrequency);

    // 等待过久没有连接自动进入深睡
    unsigned long nowTime = millis();
    if (nowTime - waitLastTime > SLEEP_WAIT_TIME)
    {
      power::deepSleep();
    }

    // 长按关机
    if (digitalRead(BUTTON_IO) == LOW)
    {
      if (buttonDown)
      {
        if (nowTime - buttonLastTime > BUTTON_SHUTDOWN_TIME)
        {
          power::deepSleep();
        }
        else
        {
          // 颜色渐亮
          uint8_t tmp = (uint8_t)((nowTime - buttonLastTime) * 1.0 / BUTTON_SHUTDOWN_TIME * 255);
          led::rgb(tmp, 0, 0);
        }
      }
      else
      {
        buttonDown = true;
        buttonLastTime = millis();
        // 显示按钮被按下的颜色
        buttonLastRGB = led::getLastColor();
      }
    }
    else
    {
      if (buttonDown)
      {
        buttonDown = false;
        led::rgb(buttonLastRGB);
      }
    }

    // 更新电池信息
    batterySupply = power::isBATSupply();
    batteryCharging = power::isCharging();
    double nowBatteryPercent = power::getBATPercent();
    if (nowBatteryPercent < batteryPercent)
    {
      batteryPercent = nowBatteryPercent;
    }

    // 自动校准电池电压
    if (!batteryCorrected)
    {
      if (!batterySupply && !batteryCharging)
      {
        config::config.battery.isCorrected = true;
        config::config.battery.bias = BATTERY_MAX - power::getBATVoltage();
        config::save();
        batteryCorrected = true;
        logger::infoln("Battery voltage is corrected.");
      }
    }

    // 电量过低提示或者关机
    if (batterySupply && !batteryCharging)
    {
      if (batteryPercent <= BATTERY_LOW_PERCENT)
      {
        // 太低关机
        power::deepSleep();
      }
      else if (batteryPercent <= BATTERY_ALERT_LOW_PERCENT)
      {
        // 太低警告
        if (batteryAlert)
        {
          led::rgb(batteryAlertNumber, 0, 0);
          if (batteryAlertBool)
          {
            batteryAlertNumber += BATTERY_ALERT_LOW_FREQUENCY;
            if (batteryAlertNumber == 0xFF)
            {
              batteryAlertBool = false;
            }
          }
          else
          {
            batteryAlertNumber -= BATTERY_ALERT_LOW_FREQUENCY;
            if (batteryAlertNumber == 0x00)
            {
              batteryAlertBool = true;
            }
          }
        }
        else
        {
          batteryAlertBool = true;
          batteryAlertNumber = 0x00;
          batteryAlert = true;
        }
      }
      else if (batteryPercent <= BATTERY_NOTIFY_LOW_PERCENT)
      {
        // 太低提示
        if (!batteryNotify)
        {
          led::red();
          batteryNotify = true;
        }
      }
      else
      {
        if (batteryAlert)
        {
          led::rgb(batteryAlertLastRGB);
          batteryAlert = false;
        }
        if (batteryNotify)
        {
          led::rgb(batteryNotifyLastRGB);
          batteryNotify = false;
        }
      }
    }
    else
    {
      if (batteryAlert)
      {
        led::rgb(batteryAlertLastRGB);
        batteryAlert = false;
      }
      if (batteryNotify)
      {
        led::rgb(batteryNotifyLastRGB);
        batteryNotify = false;
      }
    }
  }
}

static void wakeUp()
{
  // 判断是怎么被唤醒的
  switch (esp_sleep_get_wakeup_cause())
  {
  case ESP_SLEEP_WAKEUP_TIMER:
  {
    logger::debugln("Power wake up from timer.");
    break;
  }

  case ESP_SLEEP_WAKEUP_EXT0:
  {
    if (power::isBATSupply())
    {
      // 检测电量是否充足
      if (power::getBATPercent() <= BATTERY_LOW_PERCENT)
      {
        for (uint8_t i = 0; i < 2; i++)
        {
          led::black();
          delay(100);
          led::red();
          delay(100);
        }
        led::black();
        power::deepSleep();
      }
    }
    // 长按才能启动
    for (uint16_t i = 0; i < BUTTON_POWERON_TIME; i++)
    {
      if (digitalRead(BUTTON_IO) == LOW)
      {
        uint8_t tmp = (i * 1.0 / BUTTON_POWERON_TIME) * 255;
        led::rgb(0, tmp, 0);
      }
      else
      {
        led::black();
        power::deepSleep(false);
      }
      delay(1);
    }
    // 闪烁提示并深睡
    for (uint8_t i = 0; i < 2; i++)
    {
      led::black();
      delay(100);
      led::green();
      delay(100);
    }
    led::green();
    logger::debugln("Power wake up from button.");
    break;
  }

  case ESP_SLEEP_WAKEUP_UNDEFINED:
  {
    led::green();
    logger::debugln("Power is normal started.");
    break;
  }

  default:
  {
    led::red();
    delay(3000);
    logger::debugln("Power unknow wake up for %d.", esp_sleep_get_wakeup_cause());
    break;
  }
  }
}

void power::deepSleep(bool withLight)
{
  if (withLight)
  {
    // 闪烁提示并深睡
    for (uint8_t i = 0; i < 2; i++)
    {
      led::black();
      delay(100);
      led::red();
      delay(100);
    }
  }
  // 关闭所有灯光
  led::black();

  // // 关闭所有无用引脚
  // for (uint8_t gpio = 0; gpio <= 21; gpio++)
  // {
  //   if (gpio != BUTTON_IO)
  //   {
  //     rtc_gpio_isolate((gpio_num_t)gpio);
  //   }
  // }

  // 设置按钮触发唤醒
  esp_sleep_enable_ext0_wakeup(BUTTON_IO, LOW);
  rtc_gpio_pulldown_dis(BUTTON_IO);
  rtc_gpio_pullup_en(BUTTON_IO);

  // 等待一会 防止马上唤醒
  delay(BUTTON_WAIT_TIME);
  // 开始深睡
  esp_deep_sleep_start();
}

void power::setup()
{
  // 初始化按钮和充电指示
  pinMode(BUTTON_IO, INPUT_PULLUP);
  pinMode(CHARGING_IO, INPUT_PULLUP);

  // 初始化 ADC
  // 分辨率
  analogReadResolution(16);
  // 衰减 0~3100mV
  analogSetAttenuation(ADC_11db);

  // 检查唤醒状态
  wakeUp();

  xTaskCreatePinnedToCore(power_handle, "power_handle", TASK_POWER_STACK, NULL, TASK_POWER_PRIORITY, NULL, TASK_POWER_CORE);
}

double power::getVCCVoltage()
{
  return analogReadMilliVolts(ADC_VCC_IO) / 1000.0 * 3.0;
}

double power::getBATVoltage()
{
  return analogReadMilliVolts(ADC_BAT_IO) / 1000.0 * 2.0;
}

bool power::isUSBSupply()
{
  return (getVCCVoltage() > ADC_POWER_SUPPLY);
}

bool power::isBATSupply()
{
  return (getVCCVoltage() <= ADC_POWER_SUPPLY);
}

bool power::isCharging()
{
  return !digitalRead(CHARGING_IO);
}

double power::getBATPercent()
{
  double vol = getBATVoltage();
  // 如果进行了电压校准
  if (config::config.battery.isCorrected)
  {
    vol += config::config.battery.bias;
  }

  double percent;
  if (vol > BATTERY_MAX)
  {
    percent = 1.0;
  }
  else if (vol < BATTERY_MIN)
  {
    percent = 0.0;
  }
  else
  {
    percent = (vol - BATTERY_MIN) / (BATTERY_MAX - BATTERY_MIN);
  }
  return percent * 100.0;
}

// 内核复位
void power::core_restart()
{
  REG_WRITE(RTC_CNTL_OPTIONS0_REG, RTC_CNTL_SW_SYS_RST);
}

// CPU复位
void power::cpu_restart()
{
  esp_restart();
}