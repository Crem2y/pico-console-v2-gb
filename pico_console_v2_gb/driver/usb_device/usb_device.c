#include "usb_device.h"

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) {
  // not Implemented
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;

  return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {
  (void)instance;

  if (report_type == HID_REPORT_TYPE_OUTPUT) {
    // Set keyboard LED e.g Capslock, Numlock etc...
    if (report_id == REPORT_ID_KEYBOARD) {
      // bufsize should be (at least) 1
      if (bufsize < 1)
        return;

      uint8_t const kbd_leds = buffer[0];
    }
  }
}

static enum hid_mode_t current_hid_mode = HID_MODE_KEYBOARD_MOUSE;

static bool keyboard_pressed = false;
static bool keyboard_pending = false;
static uint8_t keyboard_key_codes[6] = {0};
static bool mouse_pending = false;
static usb_device_mouse_t mouse_state = {0};
static bool gamepad_pending = false;
static usb_device_gamepad_t gamepad_state = {0};

static void send_keyboard_or_mouse_report(void) {
  // skip if hid is not ready yet
  if (!tud_hid_ready()) {
    return;
  }

  // avoid sending multiple zero reports
  static bool send_empty_keyboard = false;

  if(keyboard_pending) {
    if (keyboard_pressed) {
      tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, keyboard_key_codes);
      send_empty_keyboard = true;
    } else {
      // send empty key report if previously has key pressed
      if (send_empty_keyboard) {
        tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
      }
      send_empty_keyboard = false;
    }
    keyboard_pending = false;
  } else if(mouse_pending) {
    tud_hid_mouse_report(REPORT_ID_MOUSE, mouse_state.buttons, mouse_state.x, mouse_state.y, mouse_state.vertical, mouse_state.horizontal);
    mouse_pending = false;
  }
}

static void send_gamepad_report(void) {
  // skip if hid is not ready yet
  if (!tud_hid_ready()) {
    return;
  }

  if(gamepad_pending) {
    tud_hid_gamepad_report(REPORT_ID_GAMEPAD, gamepad_state.s1_x, gamepad_state.s1_y, gamepad_state.s2_x, 0, gamepad_state.s2_y, 0, gamepad_state.hat, gamepad_state.buttons);
    gamepad_pending = false;
  }
}

static void send_hid_report(void) {
  // skip if hid is not ready yet
  if (!tud_hid_ready()) {
    return;
  }

  switch (current_hid_mode) {
    case HID_MODE_KEYBOARD_MOUSE:
      send_keyboard_or_mouse_report();
      break;
    case HID_MODE_GAMEPAD:
      send_gamepad_report();
      break;
  }
}

void hid_task(void) {

  // Remote wakeup
  if (tud_suspended() && (keyboard_pressed || 0 != mouse_state.buttons || 0 != gamepad_state.buttons))
  {
    // Wake up host if we are in suspend mode
    // and REMOTE_WAKEUP feature is enabled by host
    tud_remote_wakeup();
  }
  else
  {
    // send a report
    send_hid_report();
  }
}

inline void usbDevice_init(void) {
  tusb_init();
}

inline void usbDevice_update(void) {
  tud_task();
  hid_task();
}

void usbDevice_update_keyboard(const bool keys_pressed, const uint8_t* key_codes) {
  keyboard_pressed = keys_pressed;
  if (keys_pressed && key_codes != NULL) {
    for (size_t i = 0; i < 6; i++) {
      keyboard_key_codes[i] = key_codes[i];
    }
  }
  keyboard_pending = true;
}

void usbDevice_update_mouse(const usb_device_mouse_t* mouse) {
  if (mouse != NULL) {
    memcpy(&mouse_state, mouse, sizeof(usb_device_mouse_t));
  }
  mouse_pending = true;
}

void usbDevice_update_gamepad(const usb_device_gamepad_t* gamepad) {
  if (gamepad != NULL) {
    memcpy(&gamepad_state, gamepad, sizeof(usb_device_gamepad_t));
  }
  gamepad_pending = true;
}

void usbDevice_set_hid_mode(enum hid_mode_t mode) {
  current_hid_mode = mode;
}