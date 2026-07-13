#pragma once

#define ENABLE_LED 1
  #define ENABLE_HW_LED 1
  #define ENABLE_SW_LED 0
#define ENABLE_PSRAM 1

#define ENABLE_RFBRIDGE 1
#if ENABLE_RFBRIDGE
  #ifdef ENABLE_HW_LED
    #undef ENABLE_HW_LED
  #endif
  #define ENABLE_HW_LED 0
  #ifdef ENABLE_SW_LED
    #undef ENABLE_SW_LED
  #endif
  #define ENABLE_SW_LED 1
#endif