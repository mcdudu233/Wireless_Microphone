#pragma once

#define BUTTON_LEFT_IO 6
#define BUTTON_RIGHT_IO 8

// 释放左、右键时间(毫秒)
#define BUTTON_LEFT_TIME 50
#define BUTTON_RIGHT_TIME 50
// 判断为OK键的时间(毫秒)
#define BUTTON_OK_TIME 100

namespace button
{
  void setup();

  bool left();
  bool right();
  bool ok();
}