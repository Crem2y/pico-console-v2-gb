#pragma once

#include <stdint.h>
#include <string.h>
#include "i2s_pcm.h"

class audioSystem {
  public:
    audioSystem(void);

    void init(void);

    void set_enable(bool enable) {
      set_mute(!enable);
    }

    void recv_bridge_note_on(const uint8_t* payload, uint8_t payload_size);
    void recv_bridge_set_freq(const uint8_t* payload, uint8_t payload_size);
    void recv_bridge_set_vol(const uint8_t* payload, uint8_t payload_size);
    void recv_bridge_set_wave(const uint8_t* payload, uint8_t payload_size);
    void recv_bridge_set_mix(const uint8_t* payload, uint8_t payload_size);
    void recv_bridge_set_vol_env(const uint8_t* payload, uint8_t payload_size);
    void recv_bridge_set_pitch_env(const uint8_t* payload, uint8_t payload_size);
    void recv_bridge_wave_data_32s(const uint8_t* payload, uint8_t payload_size);
    void recv_bridge_set_master(const uint8_t* payload, uint8_t payload_size);

  private:

};