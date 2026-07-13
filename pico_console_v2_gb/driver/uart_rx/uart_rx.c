#include "uart_rx.h"

#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "uart_rx.pio.h"

bool pio_uart_rx_init(pio_uart_rx_t *rx, PIO pio, uint pin, uint serial_baud) {
  if (rx == NULL) return false;

  int sm = pio_claim_unused_sm(pio, false);
  if (sm < 0) return false;

  uint offset = pio_add_program(pio, &uart_rx_program);

  rx->pio = pio;
  rx->sm = (uint)sm;
  rx->offset = offset;
  rx->pin = pin;

  uart_rx_program_init(pio, sm, offset, pin, serial_baud);

  return true;
}

int pio_uart_rx_read(pio_uart_rx_t *rx, uint8_t* data, size_t buf_size) {
  if(!uart_rx_program_readable(rx->pio, rx->sm)) return 0;

  size_t count = 0;
  while(uart_rx_program_readable(rx->pio, rx->sm)) {
    data[count] = uart_rx_program_getc(rx->pio, rx->sm);
    count++;
    if(count >= buf_size) break;
  }

  return count;
}

int pio_uart_rx_readable(pio_uart_rx_t *rx) {
  return uart_rx_program_readable(rx->pio, rx->sm);
}