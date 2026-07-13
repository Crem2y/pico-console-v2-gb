#pragma once

#include <stdint.h>

#include "common.h"
#include "led_builtin.h"
#include "led_status.hpp"
#include "system_time.h"
#include "bridge_protocol.hpp"

#define LED_CTRL_NUM 5 // built-in LED + 4 external LEDs
#define LED_CTRL_BRIGHTNESS_MAX 255
#define LED_CTRL_BRIGHTNESS_MIN 0

enum led_ctrl_name {
  LED_CTRL_BUILT_IN = 0,
  LED_CTRL_1,
  LED_CTRL_2,
  LED_CTRL_3,
  LED_CTRL_4,
};

enum led_ctrl_mode {
  LED_OFF = 0,
  LED_ON,
  LED_BLINK_ONCE,
  LED_BLINK_REPEAT,
  LED_BRIGHTER,
  LED_DARKER,
  LED_BREATHING_ONCE,
  LED_BREATHING_REPEAT,
};

typedef struct _led_control_t {
  bool pwm_available;  
  enum led_ctrl_mode mode;
  uint8_t brightness;
  uint8_t current_brightness;
  time_ms_t update_interval_ms;
  time_ms_t last_update_ms;
  uint8_t breathing_step;
  uint32_t breathing_direction; // 1 for increasing brightness, 0 for decreasing brightness
} led_control_t;

typedef struct _led_config_t {
  enum led_ctrl_mode mode;
  uint8_t brightness;
  time_ms_t update_interval_ms;
  uint8_t breathing_step;
} led_config_t;

class ledControl {
  public:
#if !ENABLE_HW_LED && ENABLE_SW_LED
    ledControl(bridgeProtocol* bridge);
#else
    ledControl(ledStatus* led_ptr);
#endif

    void init(void);
    void update(void);

    void update_led(enum led_ctrl_name led_name);
    void write_leds(void);

    void set_config(enum led_ctrl_name led_name, led_config_t config);
    void set_mode(enum led_ctrl_name led_name, enum led_ctrl_mode mode);
    void set_brightness(enum led_ctrl_name led_name, uint8_t brightness); // for LED_ON, LED_BLINK, LED_BREATHING
    void set_blink_interval(enum led_ctrl_name led_name, time_ms_t interval_ms); // for LED_BLINK, LED_BREATHING
    void set_breathing_step(enum led_ctrl_name led_name, uint8_t step); // for LED_BREATHING

#if ENABLE_SW_LED
    void send_bridge_led_control(void);
#endif

  private:
#if ENABLE_HW_LED
    ledStatus* led;
#endif
#if ENABLE_SW_LED
    bridgeProtocol* Bridge;
#endif
    led_control_t led_controls[LED_CTRL_NUM];
    time_ms_t current_time_ms;
};