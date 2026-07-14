#pragma once

#include "common.h"
#include "sd_card.hpp"

FRESULT save_rgb565_bmp(const char *path, const uint16_t *capture_buffer, uint32_t width, uint32_t height);