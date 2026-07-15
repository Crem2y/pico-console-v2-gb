#pragma once

#include <stdint.h>
#include <stdbool.h>

#define WAVE_TABLE_LEN 2048
#define WAVE_MAX_VALUE 32767
#define WAVE_MID_VALUE 0
#define WAVE_MIN_VALUE (-32768)

#ifdef __cplusplus
extern "C" {
#endif

extern int16_t square_12_wave_table[WAVE_TABLE_LEN];
extern int16_t square_25_wave_table[WAVE_TABLE_LEN];
extern int16_t square_50_wave_table[WAVE_TABLE_LEN];
extern int16_t square_75_wave_table[WAVE_TABLE_LEN];
extern int16_t triangle_wave_table[WAVE_TABLE_LEN];
extern int16_t sawtooth_wave_table[WAVE_TABLE_LEN];
extern int16_t noise_wave_table[WAVE_TABLE_LEN];
extern int16_t noise7_wave_table[WAVE_TABLE_LEN];
extern int16_t noise15_wave_table[WAVE_TABLE_LEN];
extern int16_t sine_wave_table[WAVE_TABLE_LEN];
extern int16_t custom0_wave_table[WAVE_TABLE_LEN];

typedef enum {
  WAVE_SQUARE_12 = 0,
  WAVE_SQUARE_25,
  WAVE_SQUARE_50,
  WAVE_SQUARE_75,
  WAVE_TRIANGLE,
  WAVE_SAWTOOTH,
  WAVE_NOISE,
  WAVE_NOISE_7,
  WAVE_NOISE_15,
  WAVE_SINE,
  WAVE_CUSTOM_0,
  WAVE_COUNT,
} wave_t;

void make_wave_tables(void);
int16_t* wave_table_ptr(wave_t w);
void set_custom_wave(wave_t w, const int16_t* wave_data);

#ifdef __cplusplus
}
#endif