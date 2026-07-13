#include "led_status.hpp"

ledStatusReal::ledStatusReal(int led_1, int led_2, int led_3, int led_4) {
  led_pin[0] = led_1;
  led_pin[1] = led_2;
  led_pin[2] = led_3;
  led_pin[3] = led_4;
}

void ledStatusReal::init(void) {
  for(int i=0; i<LED_NUM; i++) {
    slice_num[i] = pwm_gpio_to_slice_num(led_pin[i]);
    led_pwm_ch[i] = pwm_gpio_to_channel(led_pin[i]);

    gpio_set_function(led_pin[i], GPIO_FUNC_PWM);
    const uint32_t clk_hz = SYS_CLK_KHZ * 1000;
    const float div = (float)clk_hz / ((float)LED_PWM_FREQ_HZ * (LED_PWM_TOP + 1));

    pwm_set_chan_level(slice_num[i], led_pwm_ch[i], 0);
    pwm_set_clkdiv(slice_num[i], div);
    pwm_set_wrap(slice_num[i], LED_PWM_TOP);
    pwm_set_enabled(slice_num[i], true);
  }
}

void ledStatusReal::set_bright(uint32_t num, uint32_t bright) {
  led_bright[num-1] = bright;
  pwm_set_chan_level(slice_num[num-1], led_pwm_ch[num-1], bright);
}

void ledStatusReal::set_bright_all(uint32_t* bright_arr) {
  for(int i=0; i<LED_NUM; i++) {
    led_bright[i] = bright_arr[i];
    pwm_set_chan_level(slice_num[i], led_pwm_ch[i], bright_arr[i]);
  }
}

uint32_t ledStatusReal::get_bright(uint32_t num) {
  return led_bright[num-1];
}