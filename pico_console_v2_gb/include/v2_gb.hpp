#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// pico libraries
#include "pico/stdlib.h"
#include "pico/multicore.h"

// common
#include "common.h"

// drivers
#include "uart_rx.h"
#include "uart_tx.h"
#include "uart_log.h"
#include "uart_bridge.h"
#include "led_builtin.h"
#include "led_status.hpp"
#include "psram_apsxx04.h"
#include "ili9488_40_hstx.hpp"
#include "system_time.h"
#include "temp_builtin.h"
#include "xpt2046.hpp"
#include "sd_card.hpp"
#include "usb_device.h"

// middlewares
#include "bridge_protocol.hpp"
#include "bridge_control.hpp"
#include "power.hpp"
#include "charger.hpp"
#include "gamepad.hpp"
#include "graphic_system.hpp"
#include "audio_system.hpp"
#include "led_control.hpp"
#include "temperature.hpp"
#include "touchscreen.hpp"
#include "vibration.hpp"
#include "imu.hpp"
#include "ir_link.hpp"

enum menu_main {
  MAIN_SYSTEM_INFO,
  MAIN_BTN_TEST,
  MAIN_JOYSTICK_TEST,
#if ENABLE_LED
  MAIN_LED_TEST,
#endif
#if ENABLE_PSRAM
  MAIN_PSRAM_TEST,
#endif
  MAIN_LCD_TEST,
  MAIN_TOUCH_TEST,
  MAIN_AUDIO_TEST,
  MAIN_VIBRATION_TEST,
  MAIN_BAT_TEST,
  MAIN_TEMP_TEST,
  MAIN_IR_TEST,
  MAIN_IMU_TEST,
  MAIN_SD_TEST,
  MAIN_USB_TEST,
};

void menu_system_info();
void menu_btn_test();
void menu_joystick_test();
#if ENABLE_LED
void menu_led_test();
#endif
#if ENABLE_PSRAM
void menu_psram_test();
#endif
void menu_lcd_test();
void menu_touch_test();
void menu_audio_test();
void menu_vibration_test();
void menu_bat_test();
void menu_temp_test();
void menu_ir_test();
void menu_imu_test();
void menu_sd_test();
void menu_usb_test();