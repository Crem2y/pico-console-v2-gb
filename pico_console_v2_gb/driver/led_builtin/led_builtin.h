#pragma once

#include "pico/stdlib.h"

#if 0
#define LED_CYW43_ENABLE 1
#include "pico/cyw43_arch.h"
#else
#define LED_CYW43_ENABLE 0
#include "hardware/gpio.h"
#define LED_BUILTIN_PIN 10
//#define LED_BUILTIN_PIN PICO_DEFAULT_LED_PIN
#endif

#ifdef __cplusplus
extern "C" {
#endif

int built_in_led_init(void);
void built_in_led_set(int state);

#ifdef __cplusplus
}
#endif