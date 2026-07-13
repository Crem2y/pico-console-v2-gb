#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "hardware/pio.h"

typedef struct {
  PIO pio;
  uint sm;
  uint offset;
  uint pin;
} pio_uart_tx_t;

#ifdef __cplusplus
extern "C" {
#endif

bool pio_uart_tx_init(pio_uart_tx_t *tx, PIO pio, uint pin, uint serial_baud);

int pio_uart_tx_write(pio_uart_tx_t *tx, const uint8_t* data, size_t data_size);
int pio_uart_tx_writeable(pio_uart_tx_t *tx);

#ifdef __cplusplus
}
#endif