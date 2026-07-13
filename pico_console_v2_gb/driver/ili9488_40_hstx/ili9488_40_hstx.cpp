// ili9488_40 display lib

// Based in part on Adafruit ILI9340 display driver
// https://github.com/adafruit/Adafruit_ILI9340

/*
  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution.
  Original source: https://github.com/adafruit/Adafruit_ILI9340
*/

// Portions of the ILI9488 initialization sequence are derived from:
// https://github.com/Interested-In-Spresense/ILI9488

// hstx original source
// https://github.com/raspberrypi/pico-examples/blob/master/hstx/spi_lcd/hstx_spi_lcd.c

#include "ili9488_40_hstx.hpp"

#define FIRST_HSTX_PIN 12

static inline void hstx_put_word(uint32_t data) {
	while (hstx_fifo_hw->stat & HSTX_FIFO_STAT_FULL_BITS)
		;
	hstx_fifo_hw->fifo = data;
}

static inline void lcd_put_dc_cs_data(bool dc, bool csn, uint8_t data) {
	hstx_put_word(
		(uint32_t)data |
		(csn ? 0x0ff00000u : 0x00000000u) |
		// Note DC gets inverted inside of HSTX:
		(dc  ? 0x00000000u : 0x0003fc00u)
	);
}

static inline void lcd_start_cmd(uint8_t cmd) {
	lcd_put_dc_cs_data(false, true, 0);
	lcd_put_dc_cs_data(false, false, cmd);
}

static inline void lcd_put_data(uint32_t data) {
	lcd_put_dc_cs_data(true, false, data);
}

static const uint8_t ili9488_init_seq[] = {
  3, 0, ILI9488_PWCTR1, 0x17, 0x15,
  2, 0, ILI9488_PWCTR2, 0x41,
  4, 30, ILI9488_VMCTR1, 0x00, 0x12, 0x00,
  2, 0, ILI9488_MADCTL, 0x40,
  2, 0, ILI9488_PIXFMT, 0x66,
  3, 30, ILI9488_FRMCTR1, 0x20, 0x00,
  2, 0, ILI9488_INVCTR, 0x02,
  3, 0, ILI9488_DFUNCTR, 0x02, 0x02,
  5, 30, 0xF7, 0x29, 0x51, 0x2C, 0x02,
  1, 30, ILI9488_SLPOUT,
  1, 30, ILI9488_DISPON,
  0
};

static inline void lcd_write_cmd(const uint8_t *cmd, size_t count) {
  lcd_start_cmd(*cmd++);
  if (count >= 2) {
    for (size_t i = 0; i < count - 1; ++i) {
      lcd_put_data(*cmd++);
    }
  }
}

static inline void lcd_init(const uint8_t *init_seq) {
  const uint8_t *cmd = init_seq;
  while (*cmd) {
    lcd_write_cmd(cmd + 2, *cmd);
    sleep_ms(*(cmd) * 5);
    cmd += *cmd + 2;
  }
}

static inline void lcd_start_pixels(void) {
  uint8_t cmd = ILI9488_RAMWR;
  lcd_write_cmd(&cmd, 1);
}

/////////////////////////////////////////////////////////////////////

// Constructor when using hardware SPI.  Faster, but must use SPI pins
// specific to each board type (e.g. 11,13 for Uno, 51,52 for Mega, etc.)
ili9488_40::ili9488_40(int pin_tx, int pin_sck, int pin_cs, int pin_dc, int pin_reset, int pin_led) {
  // These can be any permutation of HSTX-capable pins:
  _pin_tx = pin_tx;
  _pin_sck = pin_sck;
  _pin_cs = pin_cs;
  _pin_dc = pin_dc;
  // These can be any pin:
  _pin_reset = pin_reset;
  _pin_led = pin_led;
}

void ili9488_40::write_command(uint8_t cmd) {
  lcd_start_cmd(cmd);
}

void ili9488_40::write_data(uint8_t d) {
  lcd_put_data(d);
} 


void ili9488_40::init(void) {
  WIDTH   = ILI9488_TFTHEIGHT;
  HEIGHT  = ILI9488_TFTWIDTH;
  _width  = WIDTH;
  _height = HEIGHT;
}

void ili9488_40::begin(void) {
  gpio_init(_pin_reset);
  gpio_set_dir(_pin_reset, GPIO_OUT);
  gpio_put(_pin_reset, 1);

  reset_block(RESETS_RESET_HSTX_BITS);
  clock_configure(
      clk_hstx,
      0,
      CLOCKS_CLK_HSTX_CTRL_AUXSRC_VALUE_CLK_SYS,
      SYS_CLK_KHZ * 1000,
      LCD_SPI_FAST
  );
  unreset_block_wait(RESETS_RESET_HSTX_BITS);

  hstx_ctrl_hw->bit[_pin_sck - FIRST_HSTX_PIN] =
    HSTX_CTRL_BIT0_CLK_BITS;

  hstx_ctrl_hw->bit[_pin_tx - FIRST_HSTX_PIN] =
    (7u << HSTX_CTRL_BIT0_SEL_P_LSB) |
    (7u << HSTX_CTRL_BIT0_SEL_N_LSB);

  hstx_ctrl_hw->bit[_pin_cs - FIRST_HSTX_PIN] =
    (27u << HSTX_CTRL_BIT0_SEL_P_LSB) |
    (27u << HSTX_CTRL_BIT0_SEL_N_LSB);

  hstx_ctrl_hw->bit[_pin_dc - FIRST_HSTX_PIN] =
    (17u << HSTX_CTRL_BIT0_SEL_P_LSB) |
    (17u << HSTX_CTRL_BIT0_SEL_N_LSB) |
    (HSTX_CTRL_BIT0_INV_BITS);

  // We have packed 8-bit fields, so shift left 1 bit/cycle, 8 times.
  hstx_ctrl_hw->csr =
    HSTX_CTRL_CSR_EN_BITS |
    (31u << HSTX_CTRL_CSR_SHIFT_LSB) |
    (8u << HSTX_CTRL_CSR_N_SHIFTS_LSB) |
    (1u << HSTX_CTRL_CSR_CLKDIV_LSB);

  gpio_set_function(_pin_sck, GPIO_FUNC_HSTX);
  gpio_set_function(_pin_tx,  GPIO_FUNC_HSTX);
  gpio_set_function(_pin_cs,  GPIO_FUNC_HSTX);
  gpio_set_function(_pin_dc,  GPIO_FUNC_HSTX);

  // toggle RST low to reset
  gpio_put(_pin_reset, 0);
  sleep_ms(20);
  gpio_put(_pin_reset, 1);
  sleep_ms(150);

  lcd_init(ili9488_init_seq);

  write_command(ILI9488_INVON);
  set_rotation(3);

  // pwm initialize
  gpio_set_function(_pin_led, GPIO_FUNC_PWM);
  slice_num = pwm_gpio_to_slice_num(_pin_led);
  led_pwm_ch = pwm_gpio_to_channel(_pin_led);

  pwm_set_chan_level(slice_num, led_pwm_ch, 0);
  pwm_set_clkdiv(slice_num, (SYS_CLK_KHZ * 1000)/LCD_BACKLIGHT_MAX);
  pwm_set_wrap(slice_num, 1000);
  pwm_set_enabled(slice_num, true);
}


void ili9488_40::set_addr_window(uint16_t x0, uint16_t y0, uint16_t x1,
  uint16_t y1) {

  write_command(ILI9488_CASET); // Column addr set
  write_data(x0 >> 8);
  write_data(x0 & 0xFF);     // XSTART 
  write_data(x1 >> 8);
  write_data(x1 & 0xFF);     // XEND

  write_command(ILI9488_PASET); // Row addr set
  write_data(y0>>8);
  write_data(y0);     // YSTART
  write_data(y1>>8);
  write_data(y1);     // YEND

  write_command(ILI9488_RAMWR); // write to RAM
}


void ili9488_40::push_color(uint16_t color) {
  uint8_t temp[3];
  temp[0]   = (color & 0x1f) << 3;         // blue
  temp[1] = ((color >> 5) & 0x3f) << 2;  // green
  temp[2] = ((color >> 11) & 0x1f) << 3; // red

  lcd_put_data(temp[0]);
  lcd_put_data(temp[1]);
  lcd_put_data(temp[2]);
}

void ili9488_40::draw_pixel(int16_t x, int16_t y, uint16_t color) {

  if((x < 0) ||(x >= _width) || (y < 0) || (y >= _height)) return;

  set_addr_window(x,y,x+1,y+1);
  push_color(color);
}


void ili9488_40::draw_fast_line_v(int16_t x, int16_t y, int16_t h,
  uint16_t color) {

  // Rudimentary clipping
  if((x >= _width) || (y >= _height)) return;
  if((y+h-1) >= _height) h = _height-y;

  set_addr_window(x, y, x, y+h-1);
  
  while (h--) {
    push_color(color);
  }
}


void ili9488_40::draw_fast_line_h(int16_t x, int16_t y, int16_t w,
  uint16_t color) {

  // Rudimentary clipping
  if((x >= _width) || (y >= _height)) return;
  if((x+w-1) >= _width)  w = _width-x;

  set_addr_window(x, y, x+w-1, y);

  while (w--) {
    push_color(color);
  }
}

void ili9488_40::draw_picture(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *picture) {
  int16_t i, j, color;

  set_addr_window(x, y, x+w-1, y+h-1);

  for(j=0; j<h; j++) {
    for(i=0; i<w; i++) {
      color = (picture[i + (j * w)]);
      push_color(color);
    }
  }
}

void ili9488_40::fill_screen(uint16_t color) {
  fill_rect(0, 0,  _width, _height, color);
}

// fill a rectangle
void ili9488_40::fill_rect(int16_t x, int16_t y, int16_t w, int16_t h,
  uint16_t color) {

  // rudimentary clipping (drawChar w/big text requires this)
  if((x >= _width) || (y >= _height)) return;
  if((x + w - 1) >= _width)  w = _width  - x;
  if((y + h - 1) >= _height) h = _height - y;

  set_addr_window(x, y, x+w-1, y+h-1);

  for(y=h; y>0; y--) {
    for(x=w; x>0; x--) {
      push_color(color);
    }
  }
}


// Pass 8-bit (each) R,G,B, get back 16-bit packed color
uint16_t ili9488_40::convert_color(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}


void ili9488_40::set_rotation(uint8_t m) {

  write_command(ILI9488_MADCTL);
  m = m % 4; // can't be higher than 3s
  switch (m) {
  case 0:
    write_data(ILI9488_MADCTL_MX);
    _width  = ILI9488_TFTWIDTH;
    _height = ILI9488_TFTHEIGHT;
    break;
  case 1:
    write_data(ILI9488_MADCTL_MV);
    _width  = ILI9488_TFTHEIGHT;
    _height = ILI9488_TFTWIDTH;
    break;
  case 2:
    write_data(ILI9488_MADCTL_MY);
    _width  = ILI9488_TFTWIDTH;
    _height = ILI9488_TFTHEIGHT;
  break;
  case 3:
    write_data(ILI9488_MADCTL_MV | ILI9488_MADCTL_MY | ILI9488_MADCTL_MX);
    _width  = ILI9488_TFTHEIGHT;
    _height = ILI9488_TFTWIDTH;
    break;
  }
}

void ili9488_40::invert_display(bool i) {
  write_command(i ? ILI9488_INVON : ILI9488_INVOFF);
}

void ili9488_40::set_bright(uint32_t bright) {
  _bright = bright;
  pwm_set_chan_level(slice_num, led_pwm_ch, bright);
}

uint32_t ili9488_40::get_bright(void) {
  return _bright;
}