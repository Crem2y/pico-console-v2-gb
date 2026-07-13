#pragma once

#include <stdio.h>
#include <string.h>

#include "uart_bridge.h"

enum log_level {
  LOG_ERROR = 0,
  LOG_WARN,
  LOG_INFO,
  LOG_DEBUG,
  LOG_TRACE,
};

#define LOG_DEFAULT_LEVEL LOG_INFO
#define LOG_PRINT_WITH_LEVEL 1
#define LOG_PRINTF_BUFFER_SIZE 256

//#define LOG_PRINTF(...) printf(__VA_ARGS__)
#define LOG_PRINTF(...) uartLog_printf(LOG_DEFAULT_LEVEL, __VA_ARGS__)
#define LOGE(...) uartLog_printf(LOG_ERROR, __VA_ARGS__)
#define LOGW(...) uartLog_printf(LOG_WARN,  __VA_ARGS__)
#define LOGI(...) uartLog_printf(LOG_INFO,  __VA_ARGS__)
#define LOGD(...) uartLog_printf(LOG_DEBUG, __VA_ARGS__)
#define LOGT(...) uartLog_printf(LOG_TRACE, __VA_ARGS__)

#ifdef __cplusplus
extern "C" {
#endif

void uartLog_init(uart_inst_t* _uart, int tx_pin, int rx_pin, int baudrate);

void uartLog_set_level(enum log_level level);
void uartLog_print(const char *s);
int uartLog_printf(enum log_level level, const char* format, ...);
int uartLog_vprintf(const char *format, va_list args);

#ifdef __cplusplus
}
#endif