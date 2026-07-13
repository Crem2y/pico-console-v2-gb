#pragma once

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>

#include "common.h"
#include "ili9488_40_hstx.hpp"
#include "psram_apsxx04.h"

#if ENABLE_PSRAM
  #define USE_FRAME_BUFFER 0
#endif
#define USE_TEXT_BUFFER 1 // Use a buffer for text output. Print speed is improved by approximately 20%, but transparent backgrounds cannot be used in this mode.
#define GRAPHIC_PRINTF_BUFFER_SIZE 256

#define ENABLE_ADAFRUIT_GFX_COMPATIBLE 1 // Enables Adafruit GFX compatible functions.
#if ENABLE_ADAFRUIT_GFX_COMPATIBLE
//// Adafruit GFX compatible layer ////
#define drawPixel     draw_pixel
#define drawLine      draw_line
#define drawRect      draw_rect
#define fillRect      fill_rect
#define fillScreen    fill_screen
#define drawTriangle  draw_triangle
#define fillTriangle  fill_triangle
#define drawCircle    draw_circle
#define fillCircle    fill_circle
#define setCursor     set_cursor
#define setTextColor  set_text_color
#define setTextSize   set_text_size
#define setRotation   set_rotation
#endif

#define GRAPHIC_BLOCK_SIZE 16
#define GRAPHIC_BLOCK_COUNT_H (ILI9488_TFTWIDTH / GRAPHIC_BLOCK_SIZE)
#define GRAPHIC_BLOCK_COUNT_V (ILI9488_TFTHEIGHT / GRAPHIC_BLOCK_SIZE)

enum GRAPHIC_COLOR16 {
  GC_BLACK   = 0x0000,
  GC_BLUE    = 0x001F,
  GC_RED     = 0xF800,
  GC_GREEN   = 0x07E0,
  GC_CYAN    = 0x07FF,
  GC_MAGENTA = 0xF81F,
  GC_YELLOW  = 0xFFE0,
  GC_WHITE   = 0xFFFF
};

typedef uint16_t g_color_t;
typedef uint16_t unicode_bmp_t;

typedef struct _g_pos_t {
  int16_t x;
  int16_t y;
} g_pos_t;

typedef struct _g_rect_t {
  int16_t x;
  int16_t y;
  int16_t w;
  int16_t h;
} g_rect_t;

enum g_font {
  G_FONT_5X8,
  G_FONT_16,
};

enum LCD_COLOR {
  LCD_BLACK = 0x0000,
  LCD_BLUE = 0x001F,
  LCD_RED = 0xF800,
  LCD_GREEN = 0x07E0,
  LCD_CYAN = 0x07FF,
  LCD_MAGENTA = 0xF81F,
  LCD_YELLOW = 0xFFE0,
  LCD_WHITE = 0xFFFF
};

class graphicSystem {
  public:
    graphicSystem(ili9488_40* display);

    void init(void);

    void update_full(void);
    void update_dirty(void);

    void draw_pixel(int16_t x, int16_t y, g_color_t color);
    void draw_pixel(g_pos_t pos, g_color_t color);
    void draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, g_color_t color);
    void draw_line(g_pos_t pos0, g_pos_t pos1, g_color_t color);

    void draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, g_color_t color);
    void draw_rect(g_rect_t rect, g_color_t color);
    void fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, g_color_t color);
    void fill_rect(g_rect_t rect, g_color_t color);
    void fill_screen(g_color_t color);

    void draw_picture(g_rect_t rect, const g_color_t *bitmap);
    void draw_picture(int16_t x, int16_t y, int16_t w, int16_t h, const g_color_t *picture);

    void draw_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, g_color_t color);
    void draw_triangle(g_pos_t pos0, g_pos_t pos1, g_pos_t pos2, g_color_t color);
    void fill_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, g_color_t color);
    void fill_triangle(g_pos_t pos0, g_pos_t pos1, g_pos_t pos2, g_color_t color);

    void draw_circle(int16_t x0, int16_t y0, int16_t r, g_color_t color);
    void draw_circle(g_pos_t pos, int16_t r, g_color_t color);
    void fill_circle(int16_t x0, int16_t y0, int16_t r, g_color_t color);
    void fill_circle(g_pos_t pos, int16_t r, g_color_t color);

    void set_text_color(g_color_t c);
    void set_text_color(g_color_t c, g_color_t bg);

    void set_cursor(int16_t x, int16_t y);
    void set_cursor(g_pos_t pos);

//// text print
    void set_text_size(uint8_t s);
    void set_font(enum g_font font);
    void print(const char *s);
    int printf(const char *format, ...);
    int vprintf(const char *format, va_list args);

    void print_5x8(const char *s);
    size_t write_5x8(unicode_bmp_t C);
    void draw_char_5x8(int16_t x, int16_t y, unsigned char c);

    void print_16(const char *s);
    size_t write_16(unicode_bmp_t C);
    void draw_char_16_eng(int16_t x, int16_t y, const unicode_bmp_t C);
    void draw_char_16_kor(int16_t x, int16_t y, const unicode_bmp_t C);

//// display configuration
    void set_rotation(uint8_t r) { rotation = (r % 4); _display->set_rotation(r); }
    uint8_t get_rotation(void) { return rotation; }
    void set_bright(uint32_t bright) { _display->set_bright(bright); }
    uint32_t get_bright(void) { return _display->get_bright(); }

#if ENABLE_ADAFRUIT_GFX_COMPATIBLE
//// Adafruit GFX compatible layer ////
    void begin(void) { init(); }
#endif

  private:
    ili9488_40* _display;

    int16_t _width, _height;
    g_pos_t cursor;

    g_color_t text_color, text_bg_color;
    uint8_t text_size;
    enum g_font text_font;
    bool wrap;

    uint8_t rotation;
};