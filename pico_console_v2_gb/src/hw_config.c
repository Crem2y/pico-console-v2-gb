/* hw_config.c
Copyright 2021 Carl John Kugler III

Licensed under the Apache License, Version 2.0 (the License); you may not use
this file except in compliance with the License. You may obtain a copy of the
License at

   http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software distributed
under the License is distributed on an AS IS BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied. See the License for the
specific language governing permissions and limitations under the License.
*/

/*

This file should be tailored to match the hardware design.

See 
  https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/tree/main#customizing-for-the-hardware-configuration

There should be one element of the spi[] array for each RP2040 hardware SPI used.

There should be one element of the spi_ifs[] array for each SPI interface object.
* Each element of spi_ifs[] must point to an spi_t instance with member "spi".

There should be one element of the sdio_ifs[] array for each SDIO interface object.

There should be one element of the sd_cards[] array for each SD card slot.
* Each element of sd_cards[] must point to its interface with spi_if_p or sdio_if_p.
*/

/* Hardware configuration for Pico SD Card Development Board
See https://oshwlab.com/carlk3/rp2040-sd-card-dev

See https://docs.google.com/spreadsheets/d/1BrzLWTyifongf_VQCc2IpJqXWtsrjmG7KnIbSBy-CPU/edit?usp=sharing,
tab "Dev Brd", for pin assignments assumed in this configuration file.
*/

#include <assert.h>
//
#include "my_debug.h"
//
#include "hw_config.h"

#include "v2_hw_def.h"

/* SDIO Interfaces */
/*
Pins CLK_gpio, D1_gpio, D2_gpio, and D3_gpio are at offsets from pin D0_gpio.
The offsets are determined by sd_driver\SDIO\rp2040_sdio.pio.
    CLK_gpio = (D0_gpio + SDIO_CLK_PIN_D0_OFFSET) % 32;
    As of this writing, SDIO_CLK_PIN_D0_OFFSET is 30,
        which is -2 in mod32 arithmetic, so:
    CLK_gpio = D0_gpio -2.
    D1_gpio = D0_gpio + 1;
    D2_gpio = D0_gpio + 2;
    D3_gpio = D0_gpio + 3;
*/
static sd_sdio_if_t sdio_ifs[] = {
    {   // sdio_ifs[0]
        .CMD_gpio = PIN_SDIO_CMD,
        .D0_gpio = PIN_SDIO_D0,
        /* GPIO reset drive strength is 4 mA.
         * When set_drive_strength is true,
         * the drive strength is set to 2 mA unless otherwise specified. */
        .set_drive_strength = true,
        .CLK_gpio_drive_strength = GPIO_DRIVE_STRENGTH_12MA,
        .CMD_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
        .D0_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
        .D1_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
        .D2_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
        .D3_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
        .SDIO_PIO = pio0,
        .DMA_IRQ_num = DMA_IRQ_1,
        //.baud_rate = 150 * 1000 * 1000 / 8  // 18750000 Hz
        //.baud_rate = 150 * 1000 * 1000 / 7  // 21428571 Hz
        .baud_rate = 150 * 1000 * 1000 / 6  // 25000000 Hz
        //.baud_rate = 150 * 1000 * 1000 / 5  // 30000000 Hz
        //.baud_rate = 150 * 1000 * 1000 / 4  // 37500000 Hz
    }
};

/* Hardware Configuration of the SD Card "objects"
    These correspond to SD card sockets
*/
static sd_card_t sd_cards[] = {  // One for each SD card
    {   // sd_cards[0]: Socket sd0
        .type = SD_IF_SDIO,
        .sdio_if_p = &sdio_ifs[0],  // Pointer to the SPI interface driving this card
        // SD Card detect:
        .use_card_detect = true,
        .card_detect_gpio = PIN_SD_DET,  
        .card_detected_true = 0, // What the GPIO read returns when a card is
                                 // present.
        .card_detect_use_pull = true,
        .card_detect_pull_hi = true
    }
};

/* ********************************************************************** */

size_t sd_get_num() { return count_of(sd_cards); }

/**
 * @brief Get a pointer to an SD card object by its number.
 *
 * @param[in] num The number of the SD card to get.
 *
 * @return A pointer to the SD card object, or @c NULL if the number is invalid.
 */
sd_card_t *sd_get_by_num(size_t num) {
    assert(num < sd_get_num());
    if (num < sd_get_num()) {
        return &sd_cards[num];
    } else {
        return NULL;
    }
}

/* [] END OF FILE */