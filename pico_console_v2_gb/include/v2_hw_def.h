#pragma once

// hardware info
#define HW_INFO_VERSION 0x0100 // V1.0.0

// software info
#define SW_INFO_VERSION 0x1000 // V1.0.00

// build date
#define DATE_YY ((__DATE__ [9] - '0') * 10 + (__DATE__ [10] - '0'))
#define DATE_MM (__DATE__ [2] == 'n' ? (__DATE__ [1] == 'a' ? 1 : 6) \
: __DATE__ [2] == 'b' ? 2 \
: __DATE__ [2] == 'r' ? (__DATE__ [0] == 'M' ? 3 : 4) \
: __DATE__ [2] == 'y' ? 5 \
: __DATE__ [2] == 'l' ? 7 \
: __DATE__ [2] == 'g' ? 8 \
: __DATE__ [2] == 'p' ? 9 \
: __DATE__ [2] == 't' ? 10 \
: __DATE__ [2] == 'v' ? 11 : 12)
#define DATE_DD ((__DATE__ [4] == ' ' ? 0 : __DATE__ [4] - '0') * 10 \
+ (__DATE__ [5] - '0'))
#define TIME_HH ((__TIME__ [0] - '0') * 10 + (__TIME__ [1] - '0'))
#define TIME_MM ((__TIME__ [3] - '0') * 10 + (__TIME__ [4] - '0'))
#define TIME_SS ((__TIME__ [6] - '0') * 10 + (__TIME__ [7] - '0'))

// pin definitions
#define PIN_LOG_TX      0
#define PIN_LOG_RX      1
    #define HW_LOG_CH       uart0
    #define HW_LOG_BAUD     115200
#define PIN_SDIO_CLK    2
#define PIN_SDIO_CMD    3
#define PIN_SDIO_D0     4
#define PIN_SDIO_D1     5
#define PIN_SDIO_D2     6
#define PIN_SDIO_D3     7
#define PIN_PSRAM_CS    8
#define PIN_SD_DET      9
#define PIN_BUILTIN_LED 10
// reserved 11
#define PIN_BRIDGE_TX   12
#define PIN_BRIDGE_RX   13
//    #define HW_BRIDGE_CH    uart0
    #define HW_BRIDGE_PIO   pio1
    #define HW_BRIDGE_BAUD  921600
#define PIN_DP_CS       14
#define PIN_DP_DC       15
#define PIN_DP_SCK      16
#define PIN_DP_MOSI     17
#define PIN_DP_RST      18
#define PIN_DP_BL       19
#define PIN_TOUCH_MISO  20
#define PIN_TOUCH_CS    21
#define PIN_TOUCH_SCK   22
#define PIN_TOUCH_MOSI  23
    #define HW_TOUCH_CH     spi0
#define PIN_TOUCH_IRQ   24
#define PIN_SB_RESET    25
#define PIN_LED_WL_1    26
#define PIN_LED_WL_2    27
#define PIN_LED_WL_3    28
#define PIN_LED_WL_4    29

#define PIN_RF_BRIDGE_TX 26
#define PIN_RF_BRIDGE_RX 27
    #define HW_RF_BRIDGE_PIO   pio2
    #define HW_RF_BRIDGE_BAUD  921600
// reserved 28-29