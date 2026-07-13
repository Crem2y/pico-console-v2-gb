#include "xpt2046.hpp"

static uint8_t xpt2046_tx_buf[4] = {0,};
static uint8_t xpt2046_rx_buf[4];

xpt2046::xpt2046(spi_inst_t* spi, int pin_tx, int pin_rx, int pin_sck, int pin_cs, int pin_irq) {
  _spi = spi;
  _pin_tx = pin_tx;
  _pin_rx = pin_rx;
  _pin_sck = pin_sck;

  _pin_cs = pin_cs;
  _pin_irq = pin_irq;
}

void xpt2046::init(void) {
  gpio_init(_pin_cs);
  gpio_set_dir(_pin_cs, GPIO_OUT);
  gpio_put(_pin_cs, 1);

  gpio_init(_pin_irq);
  gpio_set_dir(_pin_irq, GPIO_IN);
  gpio_pull_up(_pin_irq);

  spi_init(_spi, XPT2046_SPI_SPEED);          //Initialize SPI with 1MHz
  gpio_set_function(_pin_rx, GPIO_FUNC_SPI);
  gpio_set_function(_pin_sck, GPIO_FUNC_SPI);
  gpio_set_function(_pin_tx, GPIO_FUNC_SPI);
}

uint16_t xpt2046::get_x(void) {
  xpt2046_tx_buf[0] = XPT2046_ADDR_X;
  gpio_put(_pin_cs, 0);
  spi_write_blocking(_spi, xpt2046_tx_buf, 1);
  sleep_us(1);
  spi_read_blocking(_spi, 0, xpt2046_rx_buf, 2);
  gpio_put(_pin_cs, 1);
  return ((xpt2046_rx_buf[0] << 8) | xpt2046_rx_buf[1]) >> 3;
}

uint16_t xpt2046::get_y(void) {
  xpt2046_tx_buf[0] = XPT2046_ADDR_Y;
  gpio_put(_pin_cs, 0);
  spi_write_blocking(_spi, xpt2046_tx_buf, 1);
  sleep_us(1);
  spi_read_blocking(_spi, 0, xpt2046_rx_buf, 2);
  gpio_put(_pin_cs, 1);
  return ((xpt2046_rx_buf[0] << 8) | xpt2046_rx_buf[1]) >> 3;
}

uint16_t xpt2046::get_z1(void) {
  xpt2046_tx_buf[0] = XPT2046_ADDR_Z1;
  gpio_put(_pin_cs, 0);
  spi_write_blocking(_spi, xpt2046_tx_buf, 1);
  sleep_us(1);
  spi_read_blocking(_spi, 0, xpt2046_rx_buf, 2);
  gpio_put(_pin_cs, 1);
  return ((xpt2046_rx_buf[0] << 8) | xpt2046_rx_buf[1]) >> 3;
}

uint16_t xpt2046::get_z2(void) {
  xpt2046_tx_buf[0] = XPT2046_ADDR_Z2;
  gpio_put(_pin_cs, 0);
  spi_write_blocking(_spi, xpt2046_tx_buf, 1);
  sleep_us(1);
  spi_read_blocking(_spi, 0, xpt2046_rx_buf, 2);
  gpio_put(_pin_cs, 1);
  return ((xpt2046_rx_buf[0] << 8) | xpt2046_rx_buf[1]) >> 3;
}

uint16_t xpt2046::get_temp(void) {
  xpt2046_tx_buf[0] = XPT2046_ADDR_TEMP;
  gpio_put(_pin_cs, 0);
  spi_write_blocking(_spi, xpt2046_tx_buf, 1);
  sleep_us(1);
  spi_read_blocking(_spi, 0, xpt2046_rx_buf, 2);
  gpio_put(_pin_cs, 1);
  return ((xpt2046_rx_buf[0] << 8) | xpt2046_rx_buf[1]) >> 3;
}

uint16_t xpt2046::get_vbat(void) {
  xpt2046_tx_buf[0] = XPT2046_ADDR_VBAT;
  gpio_put(_pin_cs, 0);
  spi_write_blocking(_spi, xpt2046_tx_buf, 1);
  sleep_us(1);
  spi_read_blocking(_spi, 0, xpt2046_rx_buf, 2);
  gpio_put(_pin_cs, 1);
  return ((xpt2046_rx_buf[0] << 8) | xpt2046_rx_buf[1]) >> 3;
}

uint16_t xpt2046::get_aux(void) {
  xpt2046_tx_buf[0] = XPT2046_ADDR_AUX;
  gpio_put(_pin_cs, 0);
  spi_write_blocking(_spi, xpt2046_tx_buf, 1);
  sleep_us(1);
  spi_read_blocking(_spi, 0, xpt2046_rx_buf, 2);
  gpio_put(_pin_cs, 1);
  return ((xpt2046_rx_buf[0] << 8) | xpt2046_rx_buf[1]) >> 3;
}

void xpt2046::get_touch_data(void) {
  touch_data.x = get_x();
  touch_data.y = get_y();
  touch_data.z1 = get_z1();
  touch_data.z2 = get_z2();
}