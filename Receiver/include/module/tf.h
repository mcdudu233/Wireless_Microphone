#pragma once

#define TF_CLK_IO GPIO_NUM_12
#define TF_CMD_IO GPIO_NUM_11
#define TF_D0_IO GPIO_NUM_13
#define TF_D1_IO GPIO_NUM_14
#define TF_D2_IO GPIO_NUM_9
#define TF_D3_IO GPIO_NUM_10

#define TF_1BIT_MODE false
#define TF_FREQ 40000

namespace tf
{
  void setup();
}