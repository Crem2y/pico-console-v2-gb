#pragma once

#include "pico/stdlib.h"
#include "hardware/adc.h"

#ifdef __cplusplus
extern "C" {
#endif

void built_in_temp_init(void);
float built_in_temp_read(void);

#ifdef __cplusplus
}
#endif