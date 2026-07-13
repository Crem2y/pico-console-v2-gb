#pragma once

#include <stdint.h>
#include "system_time.h"

#define GP_BTN_NUM 26 // 18 buttons + 8 directional inputs from joysticks

#define GP_JOYSTICK_NUM 2
#define GP_JOYSTICK_CH_NUM (GP_JOYSTICK_NUM * 2)
#define GP_JOYSTICK_MAX 127
#define GP_JOYSTICK_MIN -127
#define GP_JOYSTICK_TO_BTN_THRESHOLD 50

enum btn_code {
  BTN_UP = 0,
  BTN_DOWN,
  BTN_LEFT,
  BTN_RIGHT,
  BTN_A,
  BTN_B,
  BTN_X,
  BTN_Y,
  BTN_SL,
  BTN_SR,
  BTN_ZL,
  BTN_ZR,
  BTN_START,
  BTN_SELECT,
  BTN_SUB1,
  BTN_SUB2,
  BTN_S1_CENTER,
  BTN_S2_CENTER,

  BTN_S1_UP,
  BTN_S1_DOWN,
  BTN_S1_LEFT,
  BTN_S1_RIGHT,
  BTN_S2_UP,
  BTN_S2_DOWN,
  BTN_S2_LEFT,
  BTN_S2_RIGHT,
  BTN_NONE = 0xff
};

enum joy_channel {
  JOY1_X = 0,
  JOY1_Y,
  JOY2_X,
  JOY2_Y
};

class gamepad {
  public:
    gamepad(void);

    void init(void);
    void update(void);

    void set_enable(bool enable, bool raw_data_enable) {
      send_bridge_enable(enable, raw_data_enable);
    }

    int is_btn_pressed(enum btn_code btn);
    int is_btn_released(enum btn_code btn);
    time_ms_t get_btn_pressed_duration(enum btn_code btn);
    time_ms_t get_btn_released_duration(enum btn_code btn);

    int8_t get_joystick_data(enum joy_channel joy_ch);
    uint16_t get_joystick_raw(enum joy_channel joy_ch);

    void send_bridge_enable(bool enable, bool raw_data_enable);

    void recv_bridge_data(const uint8_t* payload, uint8_t payload_size);
    void recv_bridge_raw_data(const uint8_t* payload, uint8_t payload_size);

  private:
    time_ms_t current_time_ms;
    uint8_t btn_state[GP_BTN_NUM]; // 0 for released, 1 for pressed
    time_ms_t btn_last_pressed_ms[GP_BTN_NUM];
    time_ms_t btn_last_released_ms[GP_BTN_NUM];
    uint32_t key_data;

    int8_t joystick_data[GP_JOYSTICK_NUM*2];
    uint16_t joystick_raw[GP_JOYSTICK_NUM*2];
};