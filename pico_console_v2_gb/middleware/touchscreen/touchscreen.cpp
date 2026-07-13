#include "touchscreen.hpp"

static inline int32_t median3(int32_t a, int32_t b, int32_t c) {
  if (a > b) {
    int32_t t = a;
    a = b;
    b = t;
  }

  if (b > c) {
    int32_t t = b;
    b = c;
    c = t;
  }

  if (a > b) {
    b = a;
  }

  return b;
}

touchscreen::touchscreen(xpt2046* touch_controller) {
  _touch_controller = touch_controller;
}

void touchscreen::init(void) {
  _touch_controller->init();

  _x_min = 0;
  _x_max = XPT2046_MAX_X;
  _y_min = 0;
  _y_max = XPT2046_MAX_Y;

  _screen_width = 100;
  _screen_height = 100;
  _rotation = 0;
  _swap_xy = false;
  _invert_x = false;
  _invert_y = false;

  touch_state = false;
  touch_point = {.x = -1, .y = -1};
  last_touched_point = {.x = -1, .y = -1};
  last_released_point = {.x = -1, .y = -1};
}

void touchscreen::update(void) {
  // get touch point data
  touch_data_t raw_touch_data[3];
  for(int i=0; i<3; i++) {
    _touch_controller->get_touch_data();
    raw_touch_data[i] = _touch_controller->touch_data;
  }

  int32_t raw_x = median3(raw_touch_data[0].x, raw_touch_data[1].x, raw_touch_data[2].x);
  int32_t raw_y = median3(raw_touch_data[0].y, raw_touch_data[1].y, raw_touch_data[2].y);
  int32_t raw_z1 = median3(raw_touch_data[0].z1, raw_touch_data[1].z1, raw_touch_data[2].z1);

  if(_invert_x) {
    raw_x = _x_max - raw_x;
  }
  if(_invert_y) {
    raw_y = _y_max - raw_y;
  }
  if(_swap_xy) {
    uint32_t temp = raw_x;
    raw_x = raw_y;
    raw_y = temp;
  }

  static touch_point_t filtered_point;
  if(touch_state) {
    filtered_point.x += (raw_x - filtered_point.x) >> 2;
    filtered_point.y += (raw_y - filtered_point.y) >> 2;
    raw_x = filtered_point.x;
    raw_y = filtered_point.y;
  }

  touch_point.x = ((raw_x - _x_min) * (_screen_width - 1)) / (_x_max - _x_min);
  touch_point.y = ((raw_y - _y_min) * (_screen_height - 1)) / (_y_max - _y_min);

  // set touch state
  if(touch_state) {
    if(raw_z1 < TS_RELEASE_THRESHOLD) { // just released
      touch_state = false;
      last_released_point = touch_point;
      filtered_point.x = -1;
      filtered_point.y = -1;
    }
  } else {
    if(raw_z1 > TS_TOUCH_THRESHOLD) { // just touched
      touch_state = true;
      last_touched_point = touch_point;
      filtered_point.x = raw_x;
      filtered_point.y = raw_y;
    }
  }
}

void touchscreen::set_calibration(uint16_t x_min, uint16_t x_max, uint16_t y_min, uint16_t y_max) {
  _x_min = x_min;
  _x_max = x_max;
  _y_min = y_min;
  _y_max = y_max;
}

void touchscreen::set_screen_size(uint16_t width, uint16_t height) {
  _screen_width = width;
  _screen_height = height;
}

void touchscreen::set_screen_config(bool swap_xy, bool invert_x, bool invert_y) {
  _swap_xy = swap_xy;
  _invert_x = invert_x;
  _invert_y = invert_y;
}

void touchscreen::set_rotation(uint8_t rotation) {
  _rotation = rotation % 4;
  switch(_rotation) {
    case 0:
      _swap_xy = true;
      _invert_x = false;
      _invert_y = false;
      break;
    case 1:
      _swap_xy = false;
      _invert_x = false;
      _invert_y = true;
      break;
    case 2:
      _swap_xy = true;
      _invert_x = true;
      _invert_y = true;
      break;
    case 3:
      _swap_xy = false;
      _invert_x = true;
      _invert_y = false;
      break;
  }
}

touch_point_t touchscreen::get_touch_point(void) {
  touch_point_t point;

  if(!touch_state) {
    point = {.x = -1, .y = -1};
  } else {
    point = touch_point;
  }

  return point;
}