#pragma once

#include <stdint.h>
#include "uart_log.h"

#include "bsp/board.h"
#include "tusb.h"
#include "usb_descriptors.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _usb_device_mouse_t {
  uint8_t buttons;
  int16_t x;
  int16_t y;
  int8_t vertical;
  int8_t horizontal;
} usb_device_mouse_t;

typedef struct _usb_device_gamepad_t {
  int8_t s1_x;
  int8_t s1_y;
  int8_t s2_x;
  int8_t s2_y;
  uint8_t hat;
  uint32_t buttons;
} usb_device_gamepad_t;

enum hid_mode_t {
  HID_MODE_KEYBOARD_MOUSE,
  HID_MODE_GAMEPAD,
};

void usbDevice_init(void);
void usbDevice_update(void);

void usbDevice_update_keyboard(const bool keys_pressed, const uint8_t* key_codes);
void usbDevice_update_mouse(const usb_device_mouse_t* mouse);
void usbDevice_update_gamepad(const usb_device_gamepad_t* gamepad);

void usbDevice_set_hid_mode(enum hid_mode_t mode);

#ifdef __cplusplus
}
#endif