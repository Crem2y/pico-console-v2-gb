#pragma once

#include "pico/stdlib.h"
#include "hardware/spi.h"

#define XPT2046_SPI_SPEED (1*1000*1000)
#define XPT2046_MAX_X 4095
#define XPT2046_MAX_Y 4095

#define XPT2046_CMD_START   0x80

#define XPT2046_ADDR_X      0xD0 // default to 12-bit mode
#define XPT2046_ADDR_Y      0x90 // default to 12-bit mode
#define XPT2046_ADDR_Z1     0xB0 // default to 12-bit mode
#define XPT2046_ADDR_Z2     0xC0 // default to 12-bit mode

#define XPT2046_ADDR_TEMP0  0x80 // internal temperature sensor
#define XPT2046_ADDR_TEMP   0xF0 // internal temperature sensor
#define XPT2046_ADDR_VBAT   0xA0 // internal battery voltage sensor
#define XPT2046_ADDR_AUX    0xE0 // auxiliary channel

#define XPT2046_MODE_12BIT  0x00 // default 12-bit mode
#define XPT2046_MODE_8BIT   0x08 // 8-bit mode

#define XPT2046_REF_SING    0x04 // single-ended mode
#define XPT2046_REF_DIFF    0x00 // differential mode

#define XPT2046_PD_LOWPOWER 0x00 // Power down between conversions, reference off, ADC off
#define XPT2046_PD_ADC_ONLY 0x01 // reference off, ADC on
#define XPT2046_PD_REF_ONLY 0x02 // reference on, ADC off
#define XPT2046_PD_ADC_REF  0x03 // always powered, reference on, ADC on

typedef struct _touch_data_t {
  uint16_t x;
  uint16_t y;
  uint16_t z1;
  uint16_t z2;
} touch_data_t;

class xpt2046 {
  public:
    xpt2046(spi_inst_t* spi, int pin_tx, int pin_rx, int pin_sck, int pin_cs, int pin_irq);
    void init(void);

    uint16_t get_x(void);
    uint16_t get_y(void);
    uint16_t get_z1(void);
    uint16_t get_z2(void);
    uint16_t get_temp(void);
    uint16_t get_vbat(void);
    uint16_t get_aux(void);

    void get_touch_data(void);

    touch_data_t touch_data;
    uint16_t temp;
    uint16_t vbat;
    uint16_t aux;

  private:
    spi_inst_t* _spi;
    int _pin_tx, _pin_rx, _pin_sck;
    int _pin_cs, _pin_irq;
};