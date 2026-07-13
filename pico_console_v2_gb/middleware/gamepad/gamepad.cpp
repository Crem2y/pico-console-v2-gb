#include "gamepad.hpp"
#include "bridge_protocol.hpp"

extern bridgeProtocol Bridge;

gamepad::gamepad(void) {

}

void gamepad::init(void) {
  for(int i=0; i<GP_BTN_NUM; i++) {
    btn_state[i] = 0;
    btn_last_pressed_ms[i] = 0;
    btn_last_released_ms[i] = 0;
  }

  for(int i=0; i<4; i++) {
    joystick_data[i] = 0;
    joystick_raw[i] = 2048;
  }
}

void gamepad::update(void) {
  current_time_ms = get_system_time_ms();

  for(int i=0; i<BTN_S1_UP; i++) {
    uint32_t is_pressed = (key_data & (0x00000001 << i)) ? 1 : 0;
    if(is_pressed && !btn_state[i]) { // button just pressed
      btn_state[i] = 1;
      btn_last_pressed_ms[i] = current_time_ms;
    } else if(!is_pressed && btn_state[i]) { // button just released
      btn_state[i] = 0;
      btn_last_released_ms[i] = current_time_ms;
    }
  }

  // joystick -> btn data update
  // S1 joystick
  if(joystick_data[JOY1_X] > GP_JOYSTICK_TO_BTN_THRESHOLD) {
    if(!btn_state[BTN_S1_RIGHT]) {
      btn_state[BTN_S1_RIGHT] = 1;
      btn_last_pressed_ms[BTN_S1_RIGHT] = current_time_ms;
    }
  } else if(joystick_data[JOY1_X] < -GP_JOYSTICK_TO_BTN_THRESHOLD) {
    if(!btn_state[BTN_S1_LEFT]) {
      btn_state[BTN_S1_LEFT] = 1;
      btn_last_pressed_ms[BTN_S1_LEFT] = current_time_ms;
    }
  } else {
    if(btn_state[BTN_S1_RIGHT]) {
      btn_state[BTN_S1_RIGHT] = 0;
      btn_last_released_ms[BTN_S1_RIGHT] = current_time_ms;
    }
    if(btn_state[BTN_S1_LEFT]) {
      btn_state[BTN_S1_LEFT] = 0;
      btn_last_released_ms[BTN_S1_LEFT] = current_time_ms;
    }
  }

  if(joystick_data[JOY1_Y] > GP_JOYSTICK_TO_BTN_THRESHOLD) {
    if(!btn_state[BTN_S1_DOWN]) {
      btn_state[BTN_S1_DOWN] = 1;
      btn_last_pressed_ms[BTN_S1_DOWN] = current_time_ms;
    }
  } else if(joystick_data[JOY1_Y] < -GP_JOYSTICK_TO_BTN_THRESHOLD) {
    if(!btn_state[BTN_S1_UP]) {
      btn_state[BTN_S1_UP] = 1;
      btn_last_pressed_ms[BTN_S1_UP] = current_time_ms;
    }
  } else {
    if(btn_state[BTN_S1_DOWN]) {
      btn_state[BTN_S1_DOWN] = 0;
      btn_last_released_ms[BTN_S1_DOWN] = current_time_ms;
    }
    if(btn_state[BTN_S1_UP]) {
      btn_state[BTN_S1_UP] = 0;
      btn_last_released_ms[BTN_S1_UP] = current_time_ms;
    }
  }

  // S2 joystick
  if(joystick_data[JOY2_X] > GP_JOYSTICK_TO_BTN_THRESHOLD)
  {
    if(!btn_state[BTN_S2_RIGHT]) {
      btn_state[BTN_S2_RIGHT] = 1;
      btn_last_pressed_ms[BTN_S2_RIGHT] = current_time_ms;
    }
  } else if(joystick_data[JOY2_X] < -GP_JOYSTICK_TO_BTN_THRESHOLD) {
    if(!btn_state[BTN_S2_LEFT]) {
      btn_state[BTN_S2_LEFT] = 1;
      btn_last_pressed_ms[BTN_S2_LEFT] = current_time_ms;
    }
  } else {
    if(btn_state[BTN_S2_RIGHT]) {
      btn_state[BTN_S2_RIGHT] = 0;
      btn_last_released_ms[BTN_S2_RIGHT] = current_time_ms;
    }
    if(btn_state[BTN_S2_LEFT]) {
      btn_state[BTN_S2_LEFT] = 0;
      btn_last_released_ms[BTN_S2_LEFT] = current_time_ms;
    }
  }

  if(joystick_data[JOY2_Y] > GP_JOYSTICK_TO_BTN_THRESHOLD) {
    if(!btn_state[BTN_S2_DOWN]) {
      btn_state[BTN_S2_DOWN] = 1;
      btn_last_pressed_ms[BTN_S2_DOWN] = current_time_ms;
    }
  } else if(joystick_data[JOY2_Y] < -GP_JOYSTICK_TO_BTN_THRESHOLD) {
    if(!btn_state[BTN_S2_UP]) {
      btn_state[BTN_S2_UP] = 1;
      btn_last_pressed_ms[BTN_S2_UP] = current_time_ms;
    }
  } else {
    if(btn_state[BTN_S2_DOWN]) {
      btn_state[BTN_S2_DOWN] = 0;
      btn_last_released_ms[BTN_S2_DOWN] = current_time_ms;
    }
    if(btn_state[BTN_S2_UP]) {
      btn_state[BTN_S2_UP] = 0;
      btn_last_released_ms[BTN_S2_UP] = current_time_ms;
    }
  }
}

int gamepad::is_btn_pressed(enum btn_code btn) {
  return btn_state[btn];
}

int gamepad::is_btn_released(enum btn_code btn) {
  return !btn_state[btn];
}

time_ms_t gamepad::get_btn_pressed_duration(enum btn_code btn) {
  if(btn_state[btn]) {
    return system_time_elapsed_ms(current_time_ms, btn_last_pressed_ms[btn]);
  } else {
    return 0;
  }
}

time_ms_t gamepad::get_btn_released_duration(enum btn_code btn) {
  if(!btn_state[btn]) {
    return system_time_elapsed_ms(current_time_ms, btn_last_released_ms[btn]);
  } else {
    return 0;
  }
}

int8_t gamepad::get_joystick_data(enum joy_channel joy_ch) {
  if(joy_ch < 0 || joy_ch >= GP_JOYSTICK_CH_NUM) return 0;

  if(joystick_data[joy_ch] > GP_JOYSTICK_MAX) joystick_data[joy_ch] = GP_JOYSTICK_MAX;
  if(joystick_data[joy_ch] < GP_JOYSTICK_MIN) joystick_data[joy_ch] = GP_JOYSTICK_MIN;

  return joystick_data[joy_ch];
}

uint16_t gamepad::get_joystick_raw(enum joy_channel joy_ch) {
  if(joy_ch < 0 || joy_ch >= GP_JOYSTICK_CH_NUM) return 0;

  return joystick_raw[joy_ch];
}

void gamepad::send_bridge_enable(bool enable, bool raw_data_enable) {
  int payload_size = 1;
  uint8_t payload_buf[PAYLOAD_MAX_SIZE];

  payload_buf[0] = (enable ? 1 : 0) | (raw_data_enable ? 2 : 0);

  Bridge.send(CMD_GAMEPAD_ENABLE, payload_size, payload_buf);
}

void gamepad::recv_bridge_data(const uint8_t* payload, uint8_t payload_size) {
  if(payload_size < 7) return;

  // little endian
  key_data = payload[0] | (payload[1] << 8) | (payload[2] << 16);

  for(int i=0; i<GP_JOYSTICK_CH_NUM; i++) {
    joystick_data[i] = payload[3+i];
  }
}

void gamepad::recv_bridge_raw_data(const uint8_t* payload, uint8_t payload_size) {
  if(payload_size < 8) return;

  // little endian
  for(int i=0; i<GP_JOYSTICK_CH_NUM; i++) {
    joystick_raw[i] = payload[2*i] | (payload[2*i+1] << 8);
  }
}