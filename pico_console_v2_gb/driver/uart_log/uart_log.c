#include "uart_log.h"

static uart_inst_t* _uart;

enum log_level log_lvl;

/*
int _write(int fd, const void *buf, size_t count) {
  const char *cbuf = (const char *)buf;
  for (size_t i = 0; i < count; ++i) {
    uart_putc_raw(_uart, cbuf[i]);
  }
  return count;
}
*/

void uartLog_init(uart_inst_t* uart, int tx_pin, int rx_pin, int baudrate) {
  _uart = uart;
  uart_bridge_init(uart, tx_pin, rx_pin, baudrate);

  uart_bridge_enable_irq();

  uartLog_set_level(LOG_DEFAULT_LEVEL);
}

void uartLog_set_level(enum log_level level) {
  log_lvl = level;
}

void uartLog_print(const char *s) {
  size_t len = 0;
  while (s[len]) {
    len++;
  }
  uart_bridge_write(s, len);
}

int uartLog_printf(enum log_level level, const char *format, ...) {
  if(level > log_lvl) return 0;

#if LOG_PRINT_WITH_LEVEL
  switch(level) {
    case LOG_ERROR:
      uartLog_print("[ERR]");
      break;
    case LOG_WARN:
      uartLog_print("[WARN]");
      break;
    case LOG_INFO:
      uartLog_print("[INFO]");
      break;
    case LOG_DEBUG:
      uartLog_print("[DBG]");
      break;
    case LOG_TRACE:
      uartLog_print("[TRACE]");
      break;
  }
#endif

  va_list args;
  va_start(args, format);

  int result = uartLog_vprintf(format, args);

  return result;
}

int uartLog_vprintf(const char *format, va_list args) {
  char string_buf[LOG_PRINTF_BUFFER_SIZE];

  int len = vsnprintf(
    string_buf,
    sizeof(string_buf),
    format,
    args
  );

  if (len >= 0) {
    uartLog_print(string_buf);
  }

  return len;
}