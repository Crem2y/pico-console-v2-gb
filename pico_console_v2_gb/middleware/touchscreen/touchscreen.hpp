#include "xpt2046.hpp"

#define TS_TOUCH_THRESHOLD 100
#define TS_RELEASE_THRESHOLD 100

typedef struct _touch_point_t {
  int32_t x;
  int32_t y;
} touch_point_t;

class touchscreen {
  public:
    touchscreen(xpt2046* touch_controller);

    void init(void);
    void update(void);

    void set_calibration(uint16_t x_min, uint16_t x_max, uint16_t y_min, uint16_t y_max);
    void set_screen_size(uint16_t width, uint16_t height);
    void set_screen_config(bool swap_xy, bool invert_x, bool invert_y);
    void set_rotation(uint8_t rotation);

    bool is_touched(void) {
      return touch_state;
    }
    touch_point_t get_touch_point(void);
    touch_point_t get_last_touched_point(void) {
      return last_touched_point;
    }
    touch_point_t get_last_released_point(void) {
      return last_released_point;
    }

  private:
    xpt2046* _touch_controller;
    uint16_t _x_min, _x_max, _y_min, _y_max;
    uint16_t _screen_width, _screen_height;
    uint8_t _rotation;
    bool _swap_xy;
    bool _invert_x, _invert_y;

    bool touch_state;
    touch_point_t touch_point;
    touch_point_t last_touched_point;
    touch_point_t last_released_point;
};