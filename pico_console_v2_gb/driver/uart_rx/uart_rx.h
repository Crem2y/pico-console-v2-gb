#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "hardware/pio.h"

typedef struct {
  PIO pio;
  uint sm;
  uint offset;
  uint pin;
} pio_uart_rx_t;

#ifdef __cplusplus
extern "C" {
#endif

bool pio_uart_rx_init(pio_uart_rx_t *rx, PIO pio, uint pin, uint serial_baud);

int pio_uart_rx_read(pio_uart_rx_t *rx, uint8_t* data, size_t buf_size);
int pio_uart_rx_readable(pio_uart_rx_t *rx);

#ifdef __cplusplus
}
#endif