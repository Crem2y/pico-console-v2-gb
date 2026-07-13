#pragma once
// ili9488_40 display lib

/*
  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution.
  Original source: https://github.com/adafruit/Adafruit_ILI9340
*/

/*
  Original source: https://github.com/Interested-In-Spresense/ILI9488
*/

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

#include "hardware/resets.h"
#include "hardware/structs/clocks.h"
#include "hardware/structs/hstx_ctrl.h"
#include "hardware/structs/hstx_fifo.h"

#define LCD_SPI_FAST (50*1000*1000)

/////////////////////////////////////////////////////////////////////

#define ILI9488_TFTWIDTH   320      ///< ILI9488 max TFT width
#define ILI9488_TFTHEIGHT  480      ///< ILI9488 max TFT height

#define ILI9488_NOP        0x00     ///< No-op register
#define ILI9488_SWRESET    0x01     ///< Software reset register
#define ILI9488_RDDID      0x04     ///< Read display identification information
#define ILI9488_RDDST      0x09     ///< Read Display Status

#define ILI9488_SLPIN      0x10     ///< Enter Sleep Mode
#define ILI9488_SLPOUT     0x11     ///< Sleep Out
#define ILI9488_PTLON      0x12     ///< Partial Mode ON
#define ILI9488_NORON      0x13     ///< Normal Display Mode ON

#define ILI9488_RDMODE     0x0A     ///< Read Display Power Mode
#define ILI9488_RDMADCTL   0x0B     ///< Read Display MADCTL
#define ILI9488_RDPIXFMT   0x0C     ///< Read Display Pixel Format
#define ILI9488_RDIMGFMT   0x0D     ///< Read Display Image Format
#define ILI9488_RDSELFDIAG 0x0F     ///< Read Display Self-Diagnostic Result

#define ILI9488_INVOFF     0x20     ///< Display Inversion OFF
#define ILI9488_INVON      0x21     ///< Display Inversion ON
#define ILI9488_GAMMASET   0x26     ///< Gamma Set
#define ILI9488_DISPOFF    0x28     ///< Display OFF
#define ILI9488_DISPON     0x29     ///< Display ON

#define ILI9488_CASET      0x2A     ///< Column Address Set
#define ILI9488_PASET      0x2B     ///< Page Address Set
#define ILI9488_RAMWR      0x2C     ///< Memory Write
#define ILI9488_RAMRD      0x2E     ///< Memory Read

#define ILI9488_PTLAR      0x30     ///< Partial Area
#define ILI9488_VSCRDEF    0x33     ///< Vertical Scrolling Definition
#define ILI9488_MADCTL     0x36     ///< Memory Access Control
#define ILI9488_VSCRSADD   0x37     ///< Vertical Scrolling Start Address
#define ILI9488_PIXFMT     0x3A     ///< COLMOD: Pixel Format Set

#define ILI9488_FRMCTR1    0xB1     ///< Frame Rate Control (In Normal Mode/Full Colors)
#define ILI9488_FRMCTR2    0xB2     ///< Frame Rate Control (In Idle Mode/8 colors)
#define ILI9488_FRMCTR3    0xB3     ///< Frame Rate control (In Partial Mode/Full Colors)
#define ILI9488_INVCTR     0xB4     ///< Display Inversion Control
#define ILI9488_DFUNCTR    0xB6     ///< Display Function Control

#define ILI9488_PWCTR1     0xC0     ///< Power Control 1
#define ILI9488_PWCTR2     0xC1     ///< Power Control 2
#define ILI9488_PWCTR3     0xC2     ///< Power Control 3
#define ILI9488_PWCTR4     0xC3     ///< Power Control 4
#define ILI9488_PWCTR5     0xC4     ///< Power Control 5
#define ILI9488_VMCTR1     0xC5     ///< VCOM Control 1
#define ILI9488_VMCTR2     0xC7     ///< VCOM Control 2

#define ILI9488_RDID1      0xDA     ///< Read ID 1
#define ILI9488_RDID2      0xDB     ///< Read ID 2
#define ILI9488_RDID3      0xDC     ///< Read ID 3
#define ILI9488_RDID4      0xDD     ///< Read ID 4

#define ILI9488_GMCTRP1    0xE0     ///< Positive Gamma Correction
#define ILI9488_GMCTRN1    0xE1     ///< Negative Gamma Correction
//#define ILI9488_PWCTR6     0xFC

#define ILI9488_MADCTL_MY  0x80  ///< Bottom to top
#define ILI9488_MADCTL_MX  0x40  ///< Right to left
#define ILI9488_MADCTL_MV  0x20  ///< Reverse Mode
#define ILI9488_MADCTL_ML  0x10  ///< LCD refresh Bottom to top
#define ILI9488_MADCTL_RGB 0x00  ///< Red-Green-Blue pixel order
#define ILI9488_MADCTL_BGR 0x08  ///< Blue-Green-Red pixel order
#define ILI9488_MADCTL_MH  0x04  ///< LCD refresh right to left

// Color definitions
#define ILI9488_BLACK       0x0000  ///<   0,   0,   0
#define ILI9488_NAVY        0x000F  ///<   0,   0, 123
#define ILI9488_DARKGREEN   0x03E0  ///<   0, 125,   0
#define ILI9488_DARKCYAN    0x03EF  ///<   0, 125, 123
#define ILI9488_MAROON      0x7800  ///< 123,   0,   0
#define ILI9488_PURPLE      0x780F  ///< 123,   0, 123
#define ILI9488_OLIVE       0x7BE0  ///< 123, 125,   0
#define ILI9488_LIGHTGREY   0xC618  ///< 198, 195, 198
#define ILI9488_DARKGREY    0x7BEF  ///< 123, 125, 123
#define ILI9488_BLUE        0x001F  ///<   0,   0, 255
#define ILI9488_GREEN       0x07E0  ///<   0, 255,   0
#define ILI9488_CYAN        0x07FF  ///<   0, 255, 255
#define ILI9488_RED         0xF800  ///< 255,   0,   0
#define ILI9488_MAGENTA     0xF81F  ///< 255,   0, 255
#define ILI9488_YELLOW      0xFFE0  ///< 255, 255,   0
#define ILI9488_WHITE       0xFFFF  ///< 255, 255, 255
#define ILI9488_ORANGE      0xFD20  ///< 255, 165,   0
#define ILI9488_GREENYELLOW 0xAFE5  ///< 173, 255,  41
#define ILI9488_PINK        0xFC18  ///< 255, 130, 198

#define LCD_BACKLIGHT_MAX 1000

class ili9488_40 {
  public:
    int16_t WIDTH, HEIGHT;   // This is the 'raw' display w/h - never changes
    int16_t _width, _height;

    ili9488_40(int pin_tx, int pin_sck, int pin_cs, int pin_dc, int pin_reset, int pin_led);

    void init(void);
    void begin(void);

    void draw_pixel(int16_t x, int16_t y, uint16_t color);
    void draw_fast_line_v(int16_t x, int16_t y, int16_t h, uint16_t color);
    void draw_fast_line_h(int16_t x, int16_t y, int16_t w, uint16_t color);
    void fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void fill_screen(uint16_t color);
    void draw_picture(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *picture);

    uint16_t convert_color(uint8_t r, uint8_t g, uint8_t b);

  // display setting
    void set_rotation(uint8_t r);
    void invert_display(bool i);

    void set_bright(uint32_t bright);
    uint32_t get_bright(void);

  private:
    uint8_t  tabcolor;
    int _pin_tx, _pin_sck, _pin_cs, _pin_dc;
    int _pin_reset, _pin_led;
    uint32_t _bright;
    int slice_num, led_pwm_ch;

    void write_command(uint8_t cmd);
    void write_data(uint8_t d);

    void set_addr_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
    void push_color(uint16_t color);
};