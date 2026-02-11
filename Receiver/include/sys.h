#pragma once

#include "cinttypes"

// #define SYSTEM_PRINT_INFORMATION

struct SystemInfo
{
  float cpu0Usage;
  float cpu1Usage;
  size_t iramUsedSize;
  size_t iramTotalSize;
  size_t psramUsedSize;
  size_t psramTotalSize;
};

extern SystemInfo systemInfo;

namespace sys
{
  void setup();
}