#include "audio_system.hpp"

static int16_t temp_wave_table[WAVE_TABLE_LEN];

audioSystem::audioSystem(void) {

}

void audioSystem::init(void) {

}

void audioSystem::recv_bridge_note_on(const uint8_t* payload, uint8_t payload_size) {
  if(payload_size < 6) return;

  float freq;
  memcpy(&freq, &payload[1], sizeof(float));
  voice_note_on(payload[0], freq, payload[5]);
}

void audioSystem::recv_bridge_set_freq(const uint8_t* payload, uint8_t payload_size) {
  if(payload_size < 5) return;

  float freq;
  memcpy(&freq, &payload[1], sizeof(float));
  set_voice_freq(payload[0], freq);
}

void audioSystem::recv_bridge_set_vol(const uint8_t* payload, uint8_t payload_size) {
  if(payload_size < 2) return;

  set_voice_volume_q8(payload[0], payload[1]);
}

void audioSystem::recv_bridge_set_wave(const uint8_t* payload, uint8_t payload_size) {
  if(payload_size < 2) return;

  set_voice_waveform(payload[0], (wave_t)payload[1]);
}

void audioSystem::recv_bridge_set_mix(const uint8_t* payload, uint8_t payload_size) {
  if(payload_size < 3) return;

  set_voice_lr_volume_q8(payload[0], payload[1], payload[2]);
}

void audioSystem::recv_bridge_set_vol_env(const uint8_t* payload, uint8_t payload_size) {
  if(payload_size < 6) return;

  uint32_t tick_us;
  memcpy(&tick_us, &payload[1], sizeof(uint32_t));
  set_voice_vol_env(payload[0], tick_us, payload[5]);
}

void audioSystem::recv_bridge_set_pitch_env(const uint8_t* payload, uint8_t payload_size) {
  if(payload_size < 7) return;

  int32_t tick_us;
  memcpy(&tick_us, &payload[1], sizeof(int32_t));
  set_voice_pitch_env(payload[0], tick_us, (int8_t)payload[5], payload[6]);
}

void audioSystem::recv_bridge_wave_data_32s(const uint8_t* payload, uint8_t payload_size) {
  if(payload_size < 16) return;

    for (size_t i = 0; i < WAVE_TABLE_LEN; i++) {
    const size_t src_index = (i * 32) / WAVE_TABLE_LEN;

    const uint8_t packed = payload[src_index >> 1];
    const uint8_t sample_4bit =
        (src_index & 1)
            ? (packed & 0x0F)
            : (packed >> 4);

    temp_wave_table[i] =
        (int16_t)(((int32_t)sample_4bit * 65535 / 15) - 32768);
  }

  set_custom_wave(WAVE_CUSTOM_0, temp_wave_table);
}

void audioSystem::recv_bridge_set_master(const uint8_t* payload, uint8_t payload_size) {
  if(payload_size < 1) return;

  set_master_volume(payload[0]);
}