#pragma once

#include <stdint.h>
#include "uart_log.h"

// sd lib (carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico)
#include "f_util.h"
#include "hw_config.h"
#include "sd_card.h"
#include "diskio.h" /* Declarations of disk functions */

#ifndef SD_CARD_AUTO_MOUNT
#define SD_CARD_AUTO_MOUNT 1
#endif

enum sd_status {
  SD_NO_CARD = 0,
  SD_NOT_MOUNTED,
  SD_MOUNTING,
  SD_MOUNTED,
  SD_CARD_ERR,
};

typedef int (*printer_wrapper_t)(const char* format, ...);

class sdCard {
  public:
    sdCard(void);

    void init(void);
    void update(void);
    void update_status(void);

    void mount(void);

    void print_info(printer_wrapper_t printer);

    enum sd_status get_status(void) { return status; }
    bool is_inserted(void);
    bool is_mounted(void);

  private:
    sd_card_t *sd_card_p;
    enum sd_status status;
#if SD_CARD_AUTO_MOUNT
    bool tried_mount;
#endif
};
