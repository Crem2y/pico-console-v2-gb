#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "wave_table.h"

int16_t square_12_wave_table[WAVE_TABLE_LEN];
int16_t square_25_wave_table[WAVE_TABLE_LEN];
int16_t square_50_wave_table[WAVE_TABLE_LEN];
int16_t square_75_wave_table[WAVE_TABLE_LEN];
int16_t triangle_wave_table[WAVE_TABLE_LEN];
int16_t sawtooth_wave_table[WAVE_TABLE_LEN];
int16_t noise_wave_table[WAVE_TABLE_LEN];
int16_t noise7_wave_table[WAVE_TABLE_LEN];
int16_t noise15_wave_table[WAVE_TABLE_LEN];
int16_t sine_wave_table[WAVE_TABLE_LEN];
int16_t custom0_wave_table[WAVE_TABLE_LEN];


static uint32_t xorshift32(uint32_t *state)
{
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static void make_gb_noise_table(int16_t *table, int table_len, bool width_7bit) {
    uint16_t lfsr = 0x7FFF;

    for (int i = 0; i < table_len; i++) {
        table[i] =
            (lfsr & 0x0001U)
                ? WAVE_MIN_VALUE
                : WAVE_MAX_VALUE;

        const uint16_t feedback =
            (uint16_t)((lfsr ^ (lfsr >> 1)) & 0x0001U);

        lfsr >>= 1;
        lfsr |= (uint16_t)(feedback << 14);

        if (width_7bit) {
            lfsr =
                (uint16_t)(
                    (lfsr & ~(1U << 6)) |
                    (feedback << 6));
        }
    }
}

void make_wave_tables(void) {
    uint32_t rng = 0x12345678;

    for (int i = 0; i < WAVE_TABLE_LEN; i++) {
        square_12_wave_table[i] = (i < WAVE_TABLE_LEN / 8) ? WAVE_MAX_VALUE : WAVE_MIN_VALUE;
        square_25_wave_table[i] = (i < WAVE_TABLE_LEN / 4) ? WAVE_MAX_VALUE : WAVE_MIN_VALUE;
        square_50_wave_table[i] = (i < WAVE_TABLE_LEN / 2) ? WAVE_MAX_VALUE : WAVE_MIN_VALUE;
        square_75_wave_table[i] = (i < (WAVE_TABLE_LEN * 3) / 4) ? WAVE_MAX_VALUE : WAVE_MIN_VALUE;
        // triangle ranges WAVE_MIN_VALUE .. WAVE_MAX_VALUE without overflow
        if (i < WAVE_TABLE_LEN / 2) {
            triangle_wave_table[i] = (int16_t)(((i * (WAVE_MAX_VALUE - WAVE_MIN_VALUE))) / (WAVE_TABLE_LEN / 2)) - (WAVE_MID_VALUE - WAVE_MIN_VALUE);
        } else {
            triangle_wave_table[i] = (int16_t)((((WAVE_TABLE_LEN - i) * (WAVE_MAX_VALUE - WAVE_MIN_VALUE)) / (WAVE_TABLE_LEN / 2)) - (WAVE_MID_VALUE - WAVE_MIN_VALUE));
        }
        sawtooth_wave_table[i] = (int16_t)((( (WAVE_TABLE_LEN - i) * (WAVE_MAX_VALUE - WAVE_MIN_VALUE)) / WAVE_TABLE_LEN) - (WAVE_MID_VALUE - WAVE_MIN_VALUE));
        noise_wave_table[i] = (xorshift32(&rng) & 1) ? WAVE_MAX_VALUE : WAVE_MIN_VALUE;
        sine_wave_table[i] = (int16_t)((float)WAVE_MAX_VALUE * cosf((float)i * 2.0f * (float)(M_PI / WAVE_TABLE_LEN)));

        custom0_wave_table[i] = WAVE_MIN_VALUE;
    }

    make_gb_noise_table(noise15_wave_table, WAVE_TABLE_LEN, false);
    make_gb_noise_table(noise7_wave_table,  WAVE_TABLE_LEN, true);
}

int16_t* wave_table_ptr(wave_t w) {
    switch (w) {
        case WAVE_SQUARE_12:    return square_12_wave_table;
        case WAVE_SQUARE_25:    return square_25_wave_table;
        case WAVE_SQUARE_50:    return square_50_wave_table;
        case WAVE_SQUARE_75:    return square_75_wave_table;
        case WAVE_TRIANGLE:     return triangle_wave_table;
        case WAVE_SAWTOOTH:     return sawtooth_wave_table;
        case WAVE_NOISE:        return noise_wave_table;
        case WAVE_NOISE_7:      return noise7_wave_table;
        case WAVE_NOISE_15:     return noise15_wave_table;
        case WAVE_SINE:         return sine_wave_table;
        case WAVE_CUSTOM_0:     return custom0_wave_table;
        default:                return square_50_wave_table;
    }
}

void set_custom_wave(wave_t w, const int16_t* wave_data) {
    if(wave_data == NULL || w < WAVE_CUSTOM_0 || w >= WAVE_COUNT) return;

    int16_t* table_ptr = wave_table_ptr(w);
    memcpy(table_ptr, wave_data, (sizeof(int16_t) * WAVE_TABLE_LEN));
}