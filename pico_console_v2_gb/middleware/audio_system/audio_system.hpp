#pragma once

#include <stdint.h>
#include "system_time.h"

extern const float sound_freq_table[9][12];

typedef struct _music_note_t { //placeholder
  uint8_t channel;
  uint8_t octave;
  uint8_t note;
  uint8_t volume;
} music_note_t;

typedef struct _music_table_t { //placeholder
  uint32_t len;
  time_ms_t note_duration_ms;
  music_note_t* notes;
} music_table_t;

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
} wave_t;

class audioSystem {
  public:
    audioSystem(void);

    void init(void);
    void update(void);

    void play_music(music_table_t* music_table, bool loop); //placeholder
    void stop_music(void);

    void set_enable(bool enable);
  
    void play_note(uint8_t ch, uint8_t octave, uint8_t note, uint8_t volume) {
      send_bridge_note_on(ch, sound_freq_table[octave][note], volume);
    }
    void play_wave(uint8_t ch, float freq, uint8_t volume) {
      send_bridge_note_on(ch, freq, volume);
    }
    void stop_note(uint8_t ch) {
      send_bridge_set_vol(ch, 0);
    }

    void set_freq(uint8_t ch, float freq) {
      send_bridge_set_freq(ch, freq);
    }
    void set_vol(uint8_t ch, uint8_t volume) {
      send_bridge_set_vol(ch, volume);
    }
    void set_wave(uint8_t ch, wave_t w) {
      send_bridge_set_wave(ch, w);
    }
    void set_mix(uint8_t ch, uint8_t volume_l, uint8_t volume_r) {
      send_bridge_set_mix(ch, volume_l, volume_r);
    }
    void set_vol_env(uint8_t ch, uint32_t tick_us, uint8_t step) {
      send_bridge_set_vol_env(ch, tick_us, step);
    }
    void set_pitch_env(uint8_t ch, int32_t tick_us, int8_t target_semitones, uint8_t step) {
      send_bridge_set_pitch_env(ch, tick_us, target_semitones, step);
    }
    void set_wave_data_32s(wave_t w, const uint8_t* wav_data) {
      send_bridge_wave_data_32s(wav_data);
    }
    void set_master_config(uint8_t volume) {
      send_bridge_set_master(volume);
    }

    void send_bridge_note_on(uint8_t ch, float freq, uint8_t volume);
    void send_bridge_note_off(uint8_t ch);
    
    void send_bridge_set_freq(uint8_t ch, float freq);
    void send_bridge_set_vol(uint8_t ch, uint8_t volume);
    void send_bridge_set_wave(uint8_t ch, wave_t w);
    void send_bridge_set_mix(uint8_t ch, uint8_t volume_l, uint8_t volume_r);
    void send_bridge_set_vol_env(uint8_t ch, uint32_t tick_us, uint8_t step);
    void send_bridge_set_pitch_env(uint8_t ch, int32_t tick_us, int8_t target_semitones, uint8_t step);
    void send_bridge_wave_data_32s(const uint8_t* wav_data);
    void send_bridge_set_master(uint8_t volume);

  private:
    time_ms_t current_time_ms;
    time_ms_t prev_note_time_ms;

    music_note_t note_now;

    music_note_t* current_note;
    uint32_t current_note_len;
    uint32_t current_note_index;
    time_ms_t current_note_duration_ms;
    bool current_table_looping;
};