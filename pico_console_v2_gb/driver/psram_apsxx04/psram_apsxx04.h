#pragma once

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/structs/qmi.h"
#include "hardware/structs/xip.h"

#define PSRAM_BASE_CACHED   ((void *)0x11000000u)
#define PSRAM_BASE_UNCACHED ((void *)0x15000000u)
#define PSRAM_BASE          PSRAM_BASE_CACHED

#define APSXX04_MAX_HZ 133000000u
#define APS1604_MAX_ADDR  ((2  * 1024 * 1024) - 1) //  16Mb =  2MB
#define APS3204_MAX_ADDR  ((4  * 1024 * 1024) - 1) //  32Mb =  4MB
#define APS6404_MAX_ADDR  ((8  * 1024 * 1024) - 1) //  64Mb =  8MB
#define APS12804_MAX_ADDR ((16 * 1024 * 1024) - 1) // 128Mb = 16MB

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t psram_size;

int32_t psram_init(int cs_pin);
int32_t psram_test(void);
uint32_t psram_read_size(void);

#ifdef __cplusplus
}
#endif