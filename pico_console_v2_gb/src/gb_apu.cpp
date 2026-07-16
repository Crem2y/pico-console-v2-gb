#include <math.h>
#include <stdint.h>
#include <string.h>

#include "gb_apu.hpp"
#include "audio_system.hpp"

extern audioSystem Audio;

#define APU_ADDR_BASE 0xFF10
#define APU_ADDR_END  0xFF3F
#define APU_MMIO_SIZE (APU_ADDR_END - APU_ADDR_BASE + 1)

enum {
  GB_APU_CH1 = 0,
  GB_APU_CH2 = 1,
  GB_APU_CH3 = 2,
  GB_APU_CH4 = 3,
  GB_APU_CH_COUNT = 4,
};

static uint8_t apu_mmio[APU_MMIO_SIZE] = {0};

struct gb_voice_state_t {
  float freq;
  uint8_t volume;
  bool active;
  bool dac_enabled;
};

static gb_voice_state_t voice_state[GB_APU_CH_COUNT] = {};
static bool wave_ram_dirty = false;

/* -------------------------------------------------------------------------- */
/* Register access helpers                                                    */
/* -------------------------------------------------------------------------- */

static inline bool audio_addr_valid(uint16_t addr) {
  return addr >= APU_ADDR_BASE && addr <= APU_ADDR_END;
}

static inline uint8_t &audio_reg(uint16_t addr) {
  return apu_mmio[addr - APU_ADDR_BASE];
}

static inline uint8_t audio_reg_read(uint16_t addr) {
  return apu_mmio[addr - APU_ADDR_BASE];
}

static inline bool gb_master_enabled(void) {
  return (audio_reg_read(0xFF26) & 0x80) != 0;
}

/* -------------------------------------------------------------------------- */
/* Common conversion helpers                                                  */
/* -------------------------------------------------------------------------- */

static float gb_pulse_frequency(uint16_t raw_freq) {
  raw_freq &= 0x07FF;
  return 131072.0f / (float)(2048U - raw_freq);
}

static float gb_wave_frequency(uint16_t raw_freq) {
  raw_freq &= 0x07FF;
  return 65536.0f / (float)(2048U - raw_freq);
}

static float gb_noise_frequency(uint8_t nr43) {
  static constexpr float divisor_table[8] = {
    0.5f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f,
  };

  const uint8_t divisor_code = nr43 & 0x07;
  const uint8_t clock_shift = (nr43 >> 4) & 0x0F;

  return 262144.0f /
         divisor_table[divisor_code] /
         (float)(1UL << clock_shift);
}

static uint8_t gb_volume_to_u8(uint8_t volume) {
  return (uint8_t)((uint16_t)(volume & 0x0F) * 255U / 15U);
}

static uint8_t gb_initial_volume(uint8_t envelope_reg) {
  return gb_volume_to_u8(envelope_reg >> 4);
}

static bool gb_dac_enabled(uint8_t envelope_reg) {
  return (envelope_reg & 0xF8) != 0;
}

/* -------------------------------------------------------------------------- */
/* Voice control                                                              */
/* -------------------------------------------------------------------------- */

static void gb_voice_stop(uint8_t ch) {
  Audio.stop_note(ch);

  voice_state[ch].volume = 0;
  voice_state[ch].active = false;
}

static void gb_stop_all_voices(void) {
  for (uint8_t ch = 0; ch < GB_APU_CH_COUNT; ++ch) {
    gb_voice_stop(ch);
  }
}

static void gb_trigger_voice(uint8_t ch, float freq, uint8_t volume) {
  gb_voice_state_t &state = voice_state[ch];

  state.freq = freq;
  state.volume = volume;
  state.active = state.dac_enabled;

  Audio.play_wave(ch, freq, gb_master_enabled() && state.active ? volume : 0);
}

/* -------------------------------------------------------------------------- */
/* Waveform conversion                                                        */
/* -------------------------------------------------------------------------- */

static void gb_set_pulse_duty(uint8_t ch, uint8_t duty) {
  switch (duty & 0x03) {
    case 0:
      Audio.set_wave(ch, WAVE_SQUARE_12);
      break;
    case 1:
      Audio.set_wave(ch, WAVE_SQUARE_25);
      break;
    case 2:
      Audio.set_wave(ch, WAVE_SQUARE_50);
      break;
    case 3:
      Audio.set_wave(ch, WAVE_SQUARE_75);
      break;
  }
}

/* -------------------------------------------------------------------------- */
/* Length / envelope conversion                                               */
/* -------------------------------------------------------------------------- */

static uint32_t gb_length_64_us(uint8_t length_value, bool enabled) {
  if (!enabled) {
    return 0;
  }

  const uint32_t length_ticks =
      64U - (uint32_t)(length_value & 0x3F);

  return length_ticks * 1000000UL / 256UL;
}

static uint32_t gb_length_256_us(uint8_t length_value, bool enabled) {
  if (!enabled) {
    return 0;
  }

  const uint32_t length_ticks =
      256U - (uint32_t)length_value;

  return length_ticks * 1000000UL / 256UL;
}

/*
 * 현재 API는 감소 envelope만 표현할 수 있습니다.
 * 또한 volume envelope와 length counter를 동시에 예약할 수 없으므로,
 * length가 켜져 있으면 채널 종료를 우선합니다.
 */
static void gb_set_volume_envelope(
    uint8_t ch,
    uint8_t envelope_reg,
    uint32_t length_us) {

  const uint8_t initial_volume = gb_initial_volume(envelope_reg);

  if (length_us != 0) {
    Audio.set_vol_env(ch, length_us, initial_volume);
    return;
  }

  const uint8_t period = envelope_reg & 0x07;
  const bool increase = (envelope_reg & 0x08) != 0;

  if (period == 0 || increase) {
    Audio.set_vol_env(ch, 0, 0);
    return;
  }

  const uint32_t tick_us =
      (uint32_t)period * 1000000UL / 64UL;

  Audio.set_vol_env(ch, tick_us, 17);
}

/* -------------------------------------------------------------------------- */
/* CH1 sweep conversion                                                       */
/* -------------------------------------------------------------------------- */

static void gb_set_ch1_sweep(uint16_t raw_freq) {
  const uint8_t nr10 = audio_reg_read(0xFF10);

  uint8_t period = (nr10 >> 4) & 0x07;
  const bool decrease = (nr10 & 0x08) != 0;
  const uint8_t shift = nr10 & 0x07;

  if (shift == 0) {
    Audio.set_pitch_env(GB_APU_CH1, 0, 0, 0);
    return;
  }

  if (period == 0) {
    period = 8;
  }

  const uint16_t delta = raw_freq >> shift;
  const int32_t next_raw =
      decrease
          ? (int32_t)raw_freq - (int32_t)delta
          : (int32_t)raw_freq + (int32_t)delta;

  if (next_raw < 0 || next_raw > 2047) {
    gb_voice_stop(GB_APU_CH1);
    return;
  }

  const float current_freq = gb_pulse_frequency(raw_freq);
  const float next_freq = gb_pulse_frequency((uint16_t)next_raw);
  const float semitones_f = 12.0f * log2f(next_freq / current_freq);

  int32_t target_semitones = (int32_t)lroundf(semitones_f);

  if (target_semitones == 0) {
    target_semitones = semitones_f >= 0.0f ? 1 : -1;
  }

  if (target_semitones > 127) {
    target_semitones = 127;
  } else if (target_semitones < -128) {
    target_semitones = -128;
  }

  const int32_t tick_us = (int32_t)((uint32_t)period * 1000000UL / 128UL);

  Audio.set_pitch_env(GB_APU_CH1, tick_us, (int8_t)target_semitones, 1);
}

/* -------------------------------------------------------------------------- */
/* Mixer                                                                      */
/* -------------------------------------------------------------------------- */

static void gb_update_channel_mix(uint8_t ch) {
  const uint8_t nr50 = audio_reg_read(0xFF24);
  const uint8_t nr51 = audio_reg_read(0xFF25);

  /*
   * NR50의 0~7은 실제로 1/8~8/8 단계이므로 +1로 변환합니다.
   * set_mix(255, 255)는 추가 감쇠가 없는 상태입니다.
   */
  const uint8_t left_level = (uint8_t)(((nr50 >> 4) & 0x07) + 1U);
  const uint8_t right_level = (uint8_t)((nr50 & 0x07) + 1U);

  uint8_t mix_l = (uint8_t)((uint16_t)left_level * 255U / 8U);
  uint8_t mix_r = (uint8_t)((uint16_t)right_level * 255U / 8U);

  if ((nr51 & (uint8_t)(1U << (ch + 4U))) == 0) {
    mix_l = 0;
  }

  if ((nr51 & (uint8_t)(1U << ch)) == 0) {
    mix_r = 0;
  }

  Audio.set_mix(ch, mix_l, mix_r);
}

static void gb_update_all_mix(void) {
  for (uint8_t ch = 0; ch < GB_APU_CH_COUNT; ++ch) {
    gb_update_channel_mix(ch);
  }
}

/* -------------------------------------------------------------------------- */
/* Channel trigger functions                                                  */
/* -------------------------------------------------------------------------- */

static void gb_trigger_ch1(void) {
  const uint8_t nr11 = audio_reg_read(0xFF11);
  const uint8_t nr12 = audio_reg_read(0xFF12);
  const uint8_t nr13 = audio_reg_read(0xFF13);
  const uint8_t nr14 = audio_reg_read(0xFF14);

  const uint16_t raw_freq =
      (uint16_t)nr13 |
      ((uint16_t)(nr14 & 0x07) << 8);

  gb_voice_state_t &state = voice_state[GB_APU_CH1];
  state.dac_enabled = gb_dac_enabled(nr12);
  state.freq = gb_pulse_frequency(raw_freq);

  if (!state.dac_enabled) {
    gb_voice_stop(GB_APU_CH1);
    return;
  }

  gb_set_pulse_duty(GB_APU_CH1, nr11 >> 6);

  const uint8_t volume = gb_initial_volume(nr12);
  gb_trigger_voice(GB_APU_CH1, state.freq, volume);

  const uint32_t length_us =
      gb_length_64_us(nr11, (nr14 & 0x40) != 0);

  gb_set_volume_envelope(GB_APU_CH1, nr12, length_us);
  gb_set_ch1_sweep(raw_freq);
}

static void gb_trigger_ch2(void) {
  const uint8_t nr21 = audio_reg_read(0xFF16);
  const uint8_t nr22 = audio_reg_read(0xFF17);
  const uint8_t nr23 = audio_reg_read(0xFF18);
  const uint8_t nr24 = audio_reg_read(0xFF19);

  const uint16_t raw_freq =
      (uint16_t)nr23 |
      ((uint16_t)(nr24 & 0x07) << 8);

  gb_voice_state_t &state = voice_state[GB_APU_CH2];
  state.dac_enabled = gb_dac_enabled(nr22);
  state.freq = gb_pulse_frequency(raw_freq);

  if (!state.dac_enabled) {
    gb_voice_stop(GB_APU_CH2);
    return;
  }

  gb_set_pulse_duty(GB_APU_CH2, nr21 >> 6);

  const uint8_t volume = gb_initial_volume(nr22);
  gb_trigger_voice(GB_APU_CH2, state.freq, volume);

  const uint32_t length_us =
      gb_length_64_us(nr21, (nr24 & 0x40) != 0);

  gb_set_volume_envelope(GB_APU_CH2, nr22, length_us);
  Audio.set_pitch_env(GB_APU_CH2, 0, 0, 0);
}

static void gb_trigger_ch3(void) {
  const uint8_t nr30 = audio_reg_read(0xFF1A);
  const uint8_t nr31 = audio_reg_read(0xFF1B);
  const uint8_t nr32 = audio_reg_read(0xFF1C);
  const uint8_t nr33 = audio_reg_read(0xFF1D);
  const uint8_t nr34 = audio_reg_read(0xFF1E);

  const uint16_t raw_freq =
      (uint16_t)nr33 |
      ((uint16_t)(nr34 & 0x07) << 8);

  static constexpr uint8_t output_level[4] = {
    0, 255, 127, 63,
  };

  gb_voice_state_t &state = voice_state[GB_APU_CH3];
  state.dac_enabled = (nr30 & 0x80) != 0;
  state.freq = gb_wave_frequency(raw_freq);

  if (!state.dac_enabled) {
    gb_voice_stop(GB_APU_CH3);
    return;
  }

  const uint8_t volume = output_level[(nr32 >> 5) & 0x03];
  gb_trigger_voice(GB_APU_CH3, state.freq, volume);

  const uint32_t length_us =
      gb_length_256_us(nr31, (nr34 & 0x40) != 0);

  if (length_us != 0 && volume != 0) {
    Audio.set_vol_env(GB_APU_CH3, length_us, volume);
  } else {
    Audio.set_vol_env(GB_APU_CH3, 0, 0);
  }

  Audio.set_pitch_env(GB_APU_CH3, 0, 0, 0);
}

static void gb_trigger_ch4(void) {
  const uint8_t nr41 = audio_reg_read(0xFF20);
  const uint8_t nr42 = audio_reg_read(0xFF21);
  const uint8_t nr43 = audio_reg_read(0xFF22);
  const uint8_t nr44 = audio_reg_read(0xFF23);

  gb_voice_state_t &state = voice_state[GB_APU_CH4];
  state.dac_enabled = gb_dac_enabled(nr42);
  state.freq = gb_noise_frequency(nr43);

  if (!state.dac_enabled) {
    gb_voice_stop(GB_APU_CH4);
    return;
  }

  if(nr43 & 0x08) {
    Audio.set_wave(GB_APU_CH4, WAVE_NOISE_7);
  } else {
    Audio.set_wave(GB_APU_CH4, WAVE_NOISE_15);
  }

  const uint8_t volume = gb_initial_volume(nr42);
  gb_trigger_voice(GB_APU_CH4, state.freq, volume);

  const uint32_t length_us =
      gb_length_64_us(nr41, (nr44 & 0x40) != 0);

  gb_set_volume_envelope(GB_APU_CH4, nr42, length_us);
  Audio.set_pitch_env(GB_APU_CH4, 0, 0, 0);
}

/* -------------------------------------------------------------------------- */
/* Live register updates                                                      */
/* -------------------------------------------------------------------------- */

static void gb_update_ch1_frequency(void) {
  gb_voice_state_t &state = voice_state[GB_APU_CH1];
  if (!state.active) {
    return;
  }

  const uint16_t raw_freq =
      (uint16_t)audio_reg_read(0xFF13) |
      ((uint16_t)(audio_reg_read(0xFF14) & 0x07) << 8);

  state.freq = gb_pulse_frequency(raw_freq);
  Audio.set_freq(GB_APU_CH1, state.freq);
}

static void gb_update_ch2_frequency(void) {
  gb_voice_state_t &state = voice_state[GB_APU_CH2];
  if (!state.active) {
    return;
  }

  const uint16_t raw_freq =
      (uint16_t)audio_reg_read(0xFF18) |
      ((uint16_t)(audio_reg_read(0xFF19) & 0x07) << 8);

  state.freq = gb_pulse_frequency(raw_freq);
  Audio.set_freq(GB_APU_CH2, state.freq);
}

static void gb_update_ch3_frequency(void) {
  gb_voice_state_t &state = voice_state[GB_APU_CH3];
  if (!state.active) {
    return;
  }

  const uint16_t raw_freq =
      (uint16_t)audio_reg_read(0xFF1D) |
      ((uint16_t)(audio_reg_read(0xFF1E) & 0x07) << 8);

  state.freq = gb_wave_frequency(raw_freq);
  Audio.set_freq(GB_APU_CH3, state.freq);
}

static void gb_update_ch3_volume(void) {
  static constexpr uint8_t output_level[4] = {
    0, 255, 127, 63,
  };

  gb_voice_state_t &state = voice_state[GB_APU_CH3];
  state.volume = output_level[(audio_reg_read(0xFF1C) >> 5) & 0x03];

  if (state.active && state.dac_enabled && gb_master_enabled()) {
    Audio.set_vol(GB_APU_CH3, state.volume);
  } else {
    Audio.set_vol(GB_APU_CH3, 0);
  }
}

static void gb_update_ch4_noise(void) {
  gb_voice_state_t &state = voice_state[GB_APU_CH4];
  const uint8_t nr43 = audio_reg_read(0xFF22);

  if(nr43 & 0x08) {
    Audio.set_wave(GB_APU_CH4, WAVE_NOISE_7);
  } else {
    Audio.set_wave(GB_APU_CH4, WAVE_NOISE_15);
  }

  if (!state.active) {
    return;
  }

  state.freq = gb_noise_frequency(nr43);
  Audio.set_freq(GB_APU_CH4, state.freq);
}

/* -------------------------------------------------------------------------- */
/* Master control                                                             */
/* -------------------------------------------------------------------------- */

static void gb_update_master_enable(void) {
  if (!gb_master_enabled()) {
    gb_stop_all_voices();
  }
}

/* -------------------------------------------------------------------------- */
/* Public interface                                                           */
/* -------------------------------------------------------------------------- */

void audio_init(uint8_t default_volume = 16) {
  memset(apu_mmio, 0, sizeof(apu_mmio));
  memset(voice_state, 0, sizeof(voice_state));
  wave_ram_dirty = true;

  Audio.set_wave(GB_APU_CH1, WAVE_SQUARE_12);
  Audio.set_wave(GB_APU_CH2, WAVE_SQUARE_12);
  Audio.set_wave(GB_APU_CH3, WAVE_CUSTOM_0);
  Audio.set_wave(GB_APU_CH4, WAVE_NOISE_15);

  uint8_t temp[16] = {0,};
  Audio.set_wave_data_32s(WAVE_CUSTOM_0, temp);

  for (uint8_t ch = 0; ch < GB_APU_CH_COUNT; ++ch) {
    Audio.set_vol(ch, 0);
    Audio.set_mix(ch, 255, 255);
  }

  Audio.set_master_config(default_volume);
}

void audio_write(const uint16_t addr, const uint8_t val) {
  if (!audio_addr_valid(addr)) {
    return;
  }

  /* NR52는 bit 7만 저장합니다. */
  if (addr == 0xFF26) {
    audio_reg(addr) = val & 0x80;
  } else {
    audio_reg(addr) = val;
  }

  switch (addr) {
    /* CH1 */
    case 0xFF11:
      gb_set_pulse_duty(GB_APU_CH1, val >> 6);
      break;

    case 0xFF12:
      voice_state[GB_APU_CH1].dac_enabled = gb_dac_enabled(val);
      if (!voice_state[GB_APU_CH1].dac_enabled) {
        gb_voice_stop(GB_APU_CH1);
      }
      break;

    case 0xFF13:
      gb_update_ch1_frequency();
      break;

    case 0xFF14:
      if (val & 0x80) {
        gb_trigger_ch1();
      } else {
        gb_update_ch1_frequency();
      }
      break;

    /* CH2 */
    case 0xFF16:
      gb_set_pulse_duty(GB_APU_CH2, val >> 6);
      break;

    case 0xFF17:
      voice_state[GB_APU_CH2].dac_enabled = gb_dac_enabled(val);
      if (!voice_state[GB_APU_CH2].dac_enabled) {
        gb_voice_stop(GB_APU_CH2);
      }
      break;

    case 0xFF18:
      gb_update_ch2_frequency();
      break;

    case 0xFF19:
      if (val & 0x80) {
        gb_trigger_ch2();
      } else {
        gb_update_ch2_frequency();
      }
      break;

    /* CH3 */
    case 0xFF1A:
      voice_state[GB_APU_CH3].dac_enabled = (val & 0x80) != 0;
      if (!voice_state[GB_APU_CH3].dac_enabled) {
        gb_voice_stop(GB_APU_CH3);
      }
      break;

    case 0xFF1C:
      gb_update_ch3_volume();
      break;

    case 0xFF1D:
      gb_update_ch3_frequency();
      break;

    case 0xFF1E:
      if (val & 0x80) {
        if (wave_ram_dirty) {
          Audio.set_wave_data_32s(WAVE_CUSTOM_0, &apu_mmio[0xFF30 - APU_ADDR_BASE]);
          wave_ram_dirty = false;
        }
        gb_trigger_ch3();
      } else {
        gb_update_ch3_frequency();
      }
      break;

    /* CH4 */
    case 0xFF21:
      voice_state[GB_APU_CH4].dac_enabled = gb_dac_enabled(val);
      if (!voice_state[GB_APU_CH4].dac_enabled) {
        gb_voice_stop(GB_APU_CH4);
      }
      break;

    case 0xFF22:
      gb_update_ch4_noise();
      break;

    case 0xFF23:
      if (val & 0x80) {
        gb_trigger_ch4();
      }
      break;

    /* Mixer */
    case 0xFF24:
    case 0xFF25:
      gb_update_all_mix();
      break;

    case 0xFF26:
      gb_update_master_enable();
      break;

    /* CH3 Wave RAM */
    case 0xFF30:
    case 0xFF31:
    case 0xFF32:
    case 0xFF33:
    case 0xFF34:
    case 0xFF35:
    case 0xFF36:
    case 0xFF37:
    case 0xFF38:
    case 0xFF39:
    case 0xFF3A:
    case 0xFF3B:
    case 0xFF3C:
    case 0xFF3D:
    case 0xFF3E:
    case 0xFF3F:
      wave_ram_dirty = true;
      break;

    default:
      break;
  }
}

uint8_t audio_read(const uint16_t addr) {
  if (!audio_addr_valid(addr)) {
    return 0xFF;
  }

  return audio_reg_read(addr);
}