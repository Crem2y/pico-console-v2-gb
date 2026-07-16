#include "audio_system.hpp"
#include "bridge_protocol.hpp"

extern bridgeProtocol Bridge;

audioSystem::audioSystem(void) {

}

void audioSystem::init(void) {
  current_note = NULL;
  current_note_index = 0;
}

void audioSystem::update(void) {
  current_time_ms = get_system_time_ms();

  if(current_note == NULL) return;

  if(system_time_elapsed_ms(current_time_ms, prev_note_time_ms) > current_note_duration_ms) {
    prev_note_time_ms = current_time_ms;

    if(current_note_index >= current_note_len) {
      if(current_table_looping) {
        current_note_index = 0;
      } else {
        current_note = NULL;
        current_note_index = 0;
        note_now = (music_note_t){0, 0, 0}; // no sound
        return;
      }
    }

    note_now = current_note[current_note_index];
    current_note_index++;
    play_note(note_now.channel, note_now.octave, note_now.note, note_now.volume);
  }
}

void audioSystem::play_music(music_table_t* music_table, bool loop) {
  if(music_table->notes == NULL || music_table->len == 0) return;

  current_note = music_table->notes;
  current_note_len = music_table->len;
  current_note_index = 0;
  current_note_duration_ms = music_table->note_duration_ms;
  current_table_looping = loop;

  prev_note_time_ms = current_time_ms;
}

void audioSystem::stop_music(void) {
  current_note = NULL;
}

void audioSystem::set_enable(bool enable) {
  if(enable)
    Bridge.send(CMD_AUDIO_ENABLE, 0, NULL);
  else
    Bridge.send(CMD_AUDIO_DISABLE, 0, NULL);
}

void audioSystem::send_bridge_note_on(uint8_t ch, float freq, uint8_t volume) {
  int payload_size = 6;
  uint8_t payload_buf[PAYLOAD_MAX_SIZE];

  payload_buf[0] = ch;
  memcpy(&payload_buf[1], &freq, sizeof(float));
  payload_buf[5] = volume;

  Bridge.send(CMD_AUDIO_NOTE_ON, payload_size, payload_buf);
}

void audioSystem::send_bridge_set_freq(uint8_t ch, float freq) {
  int payload_size = 5;
  uint8_t payload_buf[PAYLOAD_MAX_SIZE];

  payload_buf[0] = ch;
  memcpy(&payload_buf[1], &freq, sizeof(float));

  Bridge.send(CMD_AUDIO_SET_FREQ, payload_size, payload_buf);
}

void audioSystem::send_bridge_set_vol(uint8_t ch, uint8_t volume) {
  int payload_size = 2;
  uint8_t payload_buf[PAYLOAD_MAX_SIZE];

  payload_buf[0] = ch;
  payload_buf[1] = volume;

  Bridge.send(CMD_AUDIO_SET_VOL, payload_size, payload_buf);
}

void audioSystem::send_bridge_set_wave(uint8_t ch, wave_t w) {
  int payload_size = 2;
  uint8_t payload_buf[PAYLOAD_MAX_SIZE];

  payload_buf[0] = ch;
  payload_buf[1] = w;

  Bridge.send(CMD_AUDIO_SET_WAVE, payload_size, payload_buf);
}

void audioSystem::send_bridge_set_mix(uint8_t ch, uint8_t volume_l, uint8_t volume_r) {
  int payload_size = 2;
  uint8_t payload_buf[PAYLOAD_MAX_SIZE];

  payload_buf[0] = ch;
  payload_buf[1] = volume_l;
  payload_buf[2] = volume_r;

  Bridge.send(CMD_AUDIO_SET_MIX, payload_size, payload_buf);
}

void audioSystem::send_bridge_set_vol_env(uint8_t ch, uint32_t tick_us, uint8_t step) {
  int payload_size = 6;
  uint8_t payload_buf[PAYLOAD_MAX_SIZE];

  payload_buf[0] = ch;
  memcpy(&payload_buf[1], &tick_us, sizeof(uint32_t));
  payload_buf[5] = step;

  Bridge.send(CMD_AUDIO_SET_VOL_ENV, payload_size, payload_buf);
}

void audioSystem::send_bridge_set_pitch_env(uint8_t ch, int32_t tick_us, int8_t target_semitones, uint8_t step) {
  int payload_size = 7;
  uint8_t payload_buf[PAYLOAD_MAX_SIZE];

  payload_buf[0] = ch;
  memcpy(&payload_buf[1], &tick_us, sizeof(int32_t));
  payload_buf[5] = (uint8_t)target_semitones;
  payload_buf[6] = step;

  Bridge.send(CMD_AUDIO_SET_PIT_ENV, payload_size, payload_buf);
}

void audioSystem::send_bridge_wave_data_32s(const uint8_t* wav_data) {
  int payload_size = 16;
  uint8_t payload_buf[PAYLOAD_MAX_SIZE];

  memcpy(payload_buf, &wav_data, 16);

  Bridge.send(CMD_AUDIO_WAVE_DATA32, payload_size, payload_buf);
}

void audioSystem::send_bridge_set_master(uint8_t volume) {
  int payload_size = 1;
  uint8_t payload_buf[PAYLOAD_MAX_SIZE];

  payload_buf[0] = volume;

  Bridge.send(CMD_AUDIO_SET_MASTER, payload_size, payload_buf);
}