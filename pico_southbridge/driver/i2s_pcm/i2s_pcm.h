#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "wave_table.h"

// -------------------- Configuration --------------------
#define SAMPLES_PER_BUFFER  256
#define NUM_CHANNELS        16    // MAX 64

// Choose output sample rate here (must match audio_format.sample_freq)
#define AUDIO_FS_HZ         24000 // MAX 96000

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  // Phase accumulator
  uint32_t pos;
  uint32_t base_step;
  uint32_t step;

  // Base volume (Q8: 0..256 means 0.0..1.0)
  // This is the peak level when a note is triggered.
  int32_t vol_q8;

  int32_t vol_l_q8;
  int32_t vol_r_q8;

  // Volume envelope level (Q8). Applied volume is vol_env_q8.
  // For now: simple linear decay envelope.
  uint32_t vol_env_tick_us;       // how often to update env (e.g., 5000)
  uint32_t vol_env_next_us;       // next update time (time_us_32())
  int32_t vol_env_decay_step_q8;  // amount to subtract each tick (>=1)

  // Pitch envelope offset.
  // Applied step is base_step * 2^(pit_env_semitones / 128).
  // A note starts at base pitch and moves toward pit_env_target_semitones.
  // Negative tick enables vibrato, alternating between target and -target.
  int32_t pit_env_target_semitones;
  int32_t pit_env_current_target_semitones;
  int32_t pit_env_semitones;
  bool pit_env_vibrate;
  uint32_t pit_env_tick_us;       // how often to update env (e.g., 5000)
  uint32_t pit_env_next_us;       // next update time (time_us_32())
  int32_t pit_env_step;           // amount to move toward target each tick (>=1)

  // Current waveform table
  wave_t wave;
  const int16_t *table;
} voice_t;

extern voice_t g_voices[NUM_CHANNELS];

void audio_loop(void);
void audio_init(int data_pin, int clock_pin_base, int mute_pin);

void set_voice_waveform(int voice_idx, wave_t w);
void set_voice_freq(int voice_idx, float freq);
void set_voice_volume_q8(int voice_idx, int32_t vol_q8);
void set_voice_lr_volume_q8(int voice_idx, int32_t vol_l_q8, int32_t vol_r_q8);
void set_voice_vol_env(int voice_idx, uint32_t tick_us, int32_t decay_step_q8);
void set_voice_pitch_env(int voice_idx, int32_t tick_us, int32_t target_semitones, int32_t step);

void voice_note_on(int voice_idx, float freq, int32_t peak_vol_q8);

void set_master_volume(uint8_t vol);
void set_mute(bool mute);

#ifdef __cplusplus
}
#endif