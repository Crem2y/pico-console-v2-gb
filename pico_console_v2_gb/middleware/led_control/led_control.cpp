#include "led_control.hpp"

#if !ENABLE_HW_LED && ENABLE_SW_LED
ledControl::ledControl(bridgeProtocol* bridge) {
  Bridge = bridge;
}
#else
ledControl::ledControl(ledStatus* led_ptr) {
  led = led_ptr;
}
#endif

void ledControl::init(void) {
  built_in_led_init();
#if ENABLE_HW_LED
  led->init();
#endif

  for(int i=0; i<LED_CTRL_NUM; i++) {
    led_controls[i].pwm_available = true;
    led_controls[i].mode = LED_OFF;
    led_controls[i].brightness = LED_CTRL_BRIGHTNESS_MIN;
    led_controls[i].current_brightness = LED_CTRL_BRIGHTNESS_MIN;
    led_controls[i].update_interval_ms = 500;
    led_controls[i].last_update_ms = 0;
    led_controls[i].breathing_step = 10;
  }
  led_controls[LED_CTRL_BUILT_IN].pwm_available = false;

  update();
}

void ledControl::update(void) {
  current_time_ms = get_system_time_ms();

  for(int i=0; i<LED_CTRL_NUM; i++) {
    update_led((enum led_ctrl_name)i);
  }
  write_leds();
}

void ledControl::update_led(enum led_ctrl_name led_name) {
  uint32_t num = led_name;

  if(!led_controls[num].pwm_available) {
    switch (led_controls[num].mode) {
      case LED_OFF:
        led_controls[num].current_brightness = LED_CTRL_BRIGHTNESS_MIN;
        break;
      case LED_ON:
        led_controls[num].current_brightness = LED_CTRL_BRIGHTNESS_MAX;
        break;
      case LED_BLINK_ONCE:
        if(system_time_elapsed_ms(current_time_ms, led_controls[num].last_update_ms) >= led_controls[num].update_interval_ms) {
          if (led_controls[num].current_brightness == 0) {
            led_controls[num].current_brightness = LED_CTRL_BRIGHTNESS_MAX;
          } else {
            led_controls[num].mode = LED_OFF;
            led_controls[num].current_brightness = LED_CTRL_BRIGHTNESS_MIN;
          }
          led_controls[num].last_update_ms = current_time_ms;
        }
        break;
      case LED_BLINK_REPEAT:
        if(system_time_elapsed_ms(current_time_ms, led_controls[num].last_update_ms) >= led_controls[num].update_interval_ms) {
          if (led_controls[num].current_brightness == 0) {
            led_controls[num].current_brightness = LED_CTRL_BRIGHTNESS_MAX;
          } else {
            led_controls[num].current_brightness = LED_CTRL_BRIGHTNESS_MIN;
          }
          led_controls[num].last_update_ms = current_time_ms;
        }
        break;
      // not supported at non-pwm led
      // case LED_BRIGHTER:
      // case LED_DARKER:
      // case LED_BREATHING_ONCE:
      // case LED_BREATHING_REPEAT:
      default:
        break;
    }
  } else {
    switch (led_controls[num].mode) {
    case LED_OFF:
      led_controls[num].current_brightness = LED_CTRL_BRIGHTNESS_MIN;
      break;
    case LED_ON:
      led_controls[num].current_brightness = led_controls[num].brightness;
      break;
    case LED_BLINK_ONCE:
      if(system_time_elapsed_ms(current_time_ms, led_controls[num].last_update_ms) >= led_controls[num].update_interval_ms) {
        if (led_controls[num].current_brightness == LED_CTRL_BRIGHTNESS_MIN) {
          led_controls[num].current_brightness = led_controls[num].brightness;
        } else {
          led_controls[num].mode = LED_OFF;
          led_controls[num].current_brightness = LED_CTRL_BRIGHTNESS_MIN;
        }
        led_controls[num].last_update_ms = current_time_ms;
      }
      break;
    case LED_BLINK_REPEAT:
      if(system_time_elapsed_ms(current_time_ms, led_controls[num].last_update_ms) >= led_controls[num].update_interval_ms) {
        if (led_controls[num].current_brightness == LED_CTRL_BRIGHTNESS_MIN) {
          led_controls[num].current_brightness = led_controls[num].brightness;
        } else {
          led_controls[num].current_brightness = LED_CTRL_BRIGHTNESS_MIN;
        }
        led_controls[num].last_update_ms = current_time_ms;
      }
      break;
    case LED_BRIGHTER:
      if(system_time_elapsed_ms(current_time_ms, led_controls[num].last_update_ms) >= led_controls[num].update_interval_ms) {
        if(led_controls[num].current_brightness < LED_CTRL_BRIGHTNESS_MAX) {
          if (led_controls[num].current_brightness < LED_CTRL_BRIGHTNESS_MAX - led_controls[num].breathing_step) {
            led_controls[num].current_brightness += led_controls[num].breathing_step;
          } else {
            led_controls[num].current_brightness = LED_CTRL_BRIGHTNESS_MAX;
            led_controls[num].mode = LED_ON;
          }
        }
        led_controls[num].last_update_ms = current_time_ms;
      }
      break;
    case LED_DARKER:
      if(system_time_elapsed_ms(current_time_ms, led_controls[num].last_update_ms) >= led_controls[num].update_interval_ms) {
        if(led_controls[num].current_brightness > LED_CTRL_BRIGHTNESS_MIN) {
          if (led_controls[num].current_brightness > led_controls[num].breathing_step) {
            led_controls[num].current_brightness -= led_controls[num].breathing_step;
          } else {
            led_controls[num].current_brightness = LED_CTRL_BRIGHTNESS_MIN;
            led_controls[num].mode = LED_OFF;
          }
        }
        led_controls[num].last_update_ms = current_time_ms;
      }
      break;
    case LED_BREATHING_ONCE:
      if(system_time_elapsed_ms(current_time_ms, led_controls[num].last_update_ms) >= led_controls[num].update_interval_ms) {
        if (led_controls[num].breathing_direction) {
          if (led_controls[num].current_brightness < LED_CTRL_BRIGHTNESS_MAX - led_controls[num].breathing_step) {
            led_controls[num].current_brightness += led_controls[num].breathing_step;
          } else {
            led_controls[num].current_brightness = LED_CTRL_BRIGHTNESS_MAX;
            led_controls[num].breathing_direction = 0;
          }
        } else {
          if (led_controls[num].current_brightness > led_controls[num].breathing_step) {
            led_controls[num].current_brightness -= led_controls[num].breathing_step;
          } else {
            led_controls[num].current_brightness = LED_CTRL_BRIGHTNESS_MIN;
            led_controls[num].mode = LED_OFF;
          }
        }
        led_controls[num].last_update_ms = current_time_ms;
      }
      break;
    case LED_BREATHING_REPEAT:
      if(system_time_elapsed_ms(current_time_ms, led_controls[num].last_update_ms) >= led_controls[num].update_interval_ms) {
        if (led_controls[num].breathing_direction) {
          if (led_controls[num].current_brightness < LED_CTRL_BRIGHTNESS_MAX - led_controls[num].breathing_step) {
            led_controls[num].current_brightness += led_controls[num].breathing_step;
          } else {
            led_controls[num].current_brightness = LED_CTRL_BRIGHTNESS_MAX;
            led_controls[num].breathing_direction = 0;
          }
        } else {
          if (led_controls[num].current_brightness > led_controls[num].breathing_step) {
            led_controls[num].current_brightness -= led_controls[num].breathing_step;
          } else {
            led_controls[num].current_brightness = LED_CTRL_BRIGHTNESS_MIN;
            led_controls[num].breathing_direction = 1;
          }
        }
        led_controls[num].last_update_ms = current_time_ms;
      }
      break;
    default:
      break;
    }
  }
}

void ledControl::write_leds(void) {
  for(int i=0; i<LED_CTRL_NUM; i++) {
    if(i == LED_CTRL_BUILT_IN) {
      bool state = (led_controls[i].current_brightness != LED_CTRL_BRIGHTNESS_MIN);
      built_in_led_set(state);
    } else {
#if !ENABLE_HW_LED && ENABLE_SW_LED
      send_bridge_led_control();
      i+=3;
#else
      uint16_t led_pwm = (uint16_t)(led_controls[i].current_brightness * ((float)LED_PWM_MAX / LED_CTRL_BRIGHTNESS_MAX));
      led->set_bright(i, led_pwm);
#endif
    }
  }
}

void ledControl::set_config(enum led_ctrl_name led_name, led_config_t config) {
  uint32_t num = led_name;
  led_controls[num].mode = config.mode;
  led_controls[num].brightness = config.brightness;
  led_controls[num].update_interval_ms = config.update_interval_ms;
  led_controls[num].breathing_step = config.breathing_step;

  led_controls[num].last_update_ms = current_time_ms;
}

void ledControl::set_mode(enum led_ctrl_name led_name, enum led_ctrl_mode mode) {
  uint32_t num = led_name;
  led_controls[num].mode = mode;
  led_controls[num].last_update_ms = current_time_ms;
  led_controls[num].breathing_direction = 1;
}

void ledControl::set_brightness(enum led_ctrl_name led_name, uint8_t brightness) {
  uint32_t num = led_name;
  led_controls[num].brightness = brightness;
}

void ledControl::set_blink_interval(enum led_ctrl_name led_name, uint32_t interval_ms) {
  uint32_t num = led_name;
  led_controls[num].update_interval_ms = interval_ms;
}

void ledControl::set_breathing_step(enum led_ctrl_name led_name, uint8_t step) {
  uint32_t num = led_name;
  led_controls[num].breathing_step = step;
}

#if ENABLE_SW_LED
void ledControl::send_bridge_led_control(void) {
  int payload_size = 4;
  uint8_t payload_buf[PAYLOAD_MAX_SIZE];

  payload_buf[0] = led_controls[1].current_brightness;
  payload_buf[1] = led_controls[2].current_brightness;
  payload_buf[2] = led_controls[3].current_brightness;
  payload_buf[3] = led_controls[4].current_brightness;

  Bridge->send(CMD_LED_CONTROL, payload_size, payload_buf);
}
#endif