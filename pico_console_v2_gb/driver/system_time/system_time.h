#pragma once

#include <stdint.h>
#include "pico/stdlib.h"

typedef uint32_t time_us_t;
typedef uint32_t time_ms_t;

#ifdef __cplusplus
extern "C" {
#endif

time_us_t get_system_time_us(void);
time_ms_t get_system_time_ms(void);

time_us_t system_time_elapsed_us(time_us_t now, time_us_t since);
time_ms_t system_time_elapsed_ms(time_ms_t now, time_ms_t since);

#ifdef __cplusplus
}
#endif