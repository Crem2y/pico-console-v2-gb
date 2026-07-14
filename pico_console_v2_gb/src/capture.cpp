#include <stdint.h>
#include <string.h>
#include "ff.h"

#include "capture.hpp"

#pragma pack(push, 1)

typedef struct _bmp_file_header_t{
    uint16_t signature;       // "BM" = 0x4D42
    uint32_t file_size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t pixel_offset;
} bmp_file_header_t;

typedef struct _bmp_info_header_t{
    uint32_t header_size;
    int32_t  width;
    int32_t  height;
    uint16_t planes;
    uint16_t bits_per_pixel;
    uint32_t compression;
    uint32_t image_size;
    int32_t  x_pixels_per_m;
    int32_t  y_pixels_per_m;
    uint32_t colors_used;
    uint32_t important_colors;
} bmp_info_header_t;

#pragma pack(pop)

FRESULT save_rgb565_bmp(
    const char *path,
    const uint16_t *capture_buffer,
    uint32_t width,
    uint32_t height
) {
    FIL file;
    FRESULT fr;
    UINT written;

    /*
     * BMP의 각 행은 4바이트 경계로 정렬됩니다.
     * RGB888은 픽셀당 3바이트입니다.
     */
    uint32_t raw_row_size = width * 3;
    uint32_t row_size = (raw_row_size + 3) & ~3U;
    uint32_t padding_size = row_size - raw_row_size;
    uint32_t image_size = row_size * height;

    bmp_file_header_t file_header = {
        .signature    = 0x4D42,
        .file_size    = sizeof(bmp_file_header_t) +
                        sizeof(bmp_info_header_t) +
                        image_size,
        .reserved1    = 0,
        .reserved2    = 0,
        .pixel_offset = sizeof(bmp_file_header_t) +
                        sizeof(bmp_info_header_t),
    };

    bmp_info_header_t info_header = {
        .header_size       = sizeof(bmp_info_header_t),
        .width             = (int32_t)width,
        .height            = (int32_t)height,
        .planes            = 1,
        .bits_per_pixel    = 24,
        .compression       = 0,  // BI_RGB
        .image_size        = image_size,
        .x_pixels_per_m    = 2835, // 약 72 DPI
        .y_pixels_per_m    = 2835,
        .colors_used       = 0,
        .important_colors  = 0,
    };

    fr = f_open(&file, path, FA_CREATE_ALWAYS | FA_WRITE);
    if (fr != FR_OK) {
        return fr;
    }

    fr = f_write(
        &file,
        &file_header,
        sizeof(file_header),
        &written
    );
    if (fr != FR_OK || written != sizeof(file_header)) {
        f_close(&file);
        return (fr != FR_OK) ? fr : FR_DISK_ERR;
    }

    fr = f_write(
        &file,
        &info_header,
        sizeof(info_header),
        &written
    );
    if (fr != FR_OK || written != sizeof(info_header)) {
        f_close(&file);
        return (fr != FR_OK) ? fr : FR_DISK_ERR;
    }

    uint8_t padding[3] = {0, 0, 0};

    /*
     * LCD_WIDTH가 컴파일 타임 상수라면 정적 버퍼로 둘 수 있습니다.
     * 480픽셀 기준 1440바이트입니다.
     */
    //static uint8_t row_buffer[LCD_WIDTH * 3];
    static uint8_t row_buffer[160 * 3];

    for (int32_t y = (int32_t)height - 1; y >= 0; y--) {
        const uint16_t *src = &capture_buffer[(uint32_t)y * width];

        for (uint32_t x = 0; x < width; x++) {
            uint16_t rgb565 = src[x];

            uint8_t r5 = (rgb565 >> 11) & 0x1F;
            uint8_t g6 = (rgb565 >> 5)  & 0x3F;
            uint8_t b5 = rgb565         & 0x1F;

            /*
             * 단순 시프트보다 비트 복제를 사용하면
             * 0~255 범위를 더 정확히 채울 수 있습니다.
             */
            uint8_t r8 = (r5 << 3) | (r5 >> 2);
            uint8_t g8 = (g6 << 2) | (g6 >> 4);
            uint8_t b8 = (b5 << 3) | (b5 >> 2);

            /*
             * BMP의 24비트 픽셀 순서는 RGB가 아니라 BGR입니다.
             */
            row_buffer[x * 3 + 0] = b8;
            row_buffer[x * 3 + 1] = g8;
            row_buffer[x * 3 + 2] = r8;
        }

        fr = f_write(&file, row_buffer, raw_row_size, &written);
        if (fr != FR_OK || written != raw_row_size) {
            f_close(&file);
            return (fr != FR_OK) ? fr : FR_DISK_ERR;
        }

        if (padding_size > 0) {
            fr = f_write(&file, padding, padding_size, &written);
            if (fr != FR_OK || written != padding_size) {
                f_close(&file);
                return (fr != FR_OK) ? fr : FR_DISK_ERR;
            }
        }
    }

    fr = f_sync(&file);
    if (fr != FR_OK) {
        f_close(&file);
        return fr;
    }

    return f_close(&file);
}