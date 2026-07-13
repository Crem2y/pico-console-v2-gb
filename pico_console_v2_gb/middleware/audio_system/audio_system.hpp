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
  WAVE_SINE,
} wave_t;

class audioSystem {
  public:
    audioSystem(void);

    void init(void);
    void update(void);

    void play_music(music_table_t* music_table, bool loop); //placeholder
    void stop_music(void);

    void play_note(uint8_t ch, uint8_t octave, uint8_t note, uint8_t volume) {
      send_bridge_note_data(ch, sound_freq_table[octave][note], volume);
    }
    void play_wave(uint8_t ch, float freq, uint8_t volume) {
      send_bridge_note_data(ch, freq, volume);
    }

    void set_enable(bool enable);
    void set_wave(uint8_t ch, wave_t w) {
      send_bridge_set_wave(ch, w);
    }
    void set_env(uint8_t ch, uint32_t tick_us, uint8_t step) {
      send_bridge_set_env(ch, tick_us, step);
    }
    void set_pitch_env(uint8_t ch, int32_t tick_us, int8_t target_semitones, uint8_t step) {
      send_bridge_set_pitch_env(ch, tick_us, target_semitones, step);
    }
    void set_master_config(uint8_t volume) {
      send_bridge_set_master(volume);
    }

    void send_bridge_note_data(uint8_t ch, float freq, uint8_t volume);
    void send_bridge_set_wave(uint8_t ch, wave_t w);
    void send_bridge_set_env(uint8_t ch, uint32_t tick_us, uint8_t step);
    void send_bridge_set_pitch_env(uint8_t ch, int32_t tick_us, int8_t target_semitones, uint8_t step);
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