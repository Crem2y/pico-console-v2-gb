#include "led_builtin.h"

int built_in_led_init(void) {
  #if LED_CYW43_ENABLE
  if (cyw43_arch_init()) {
    return -1;
  }
  #else
  gpio_init(LED_BUILTIN_PIN);
  gpio_set_dir(LED_BUILTIN_PIN, GPIO_OUT);
  #endif
  return 0;
}

void built_in_led_set(int state) {
  if(state) {
    #if LED_CYW43_ENABLE
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    #else
    gpio_put(LED_BUILTIN_PIN, 1);
    #endif
  } else {
    #if LED_CYW43_ENABLE
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    #else
    gpio_put(LED_BUILTIN_PIN, 0);
    #endif
  }
}