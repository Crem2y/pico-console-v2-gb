#include "uart_tx.h"

#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "uart_tx.pio.h"

bool pio_uart_tx_init(pio_uart_tx_t *tx, PIO pio, uint pin, uint serial_baud) {
  if (tx == NULL) return false;

  int sm = pio_claim_unused_sm(pio, false);
  if (sm < 0) return false;

  uint offset = pio_add_program(pio, &uart_tx_program);

  tx->pio = pio;
  tx->sm = (uint)sm;
  tx->offset = offset;
  tx->pin = pin;

  uart_tx_program_init(pio, sm, offset, pin, serial_baud);

  return true;
}

int pio_uart_tx_write(pio_uart_tx_t *tx, const uint8_t* data, size_t data_size) {
  for(size_t i=0; i<data_size; i++) {
    uart_tx_program_putc(tx->pio, tx->sm, data[i]);
  }
  return data_size;
}

int pio_uart_tx_writeable(pio_uart_tx_t *tx) {
  return 256; //test
}