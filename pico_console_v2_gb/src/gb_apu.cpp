#include "gb_apu.hpp"
#include "audio_system.hpp"

#include <cmath>
#include <cstdint>
#include <cstring>

extern audioSystem Audio;

#define APU_ADDR_BASE 0xFF10
#define APU_ADDR_END  0xFF3F
#define APU_MMIO_SIZE (APU_ADDR_END - APU_ADDR_BASE + 1)

enum {
  GB_APU_CH1 = 0,
  GB_APU_CH2 = 1,
  GB_APU_CH3 = 2,
  GB_APU_CH4 = 3,
};

static uint8_t apu_mmio[APU_MMIO_SIZE] = {0};


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


/* -------------------------------------------------------------------------- */
/* Common conversion helpers                                                  */
/* -------------------------------------------------------------------------- */

/*
 * CH1 / CH2:
 *
 *   frequency = 131072 / (2048 - x)
 */
static float gb_pulse_frequency(uint16_t raw_freq) {
  raw_freq &= 0x07FF;

  return 131072.0f /
         static_cast<float>(2048U - raw_freq);
}

/*
 * CH3:
 *
 *   frequency = 65536 / (2048 - x)
 */
static float gb_wave_frequency(uint16_t raw_freq) {
  raw_freq &= 0x07FF;

  return 65536.0f /
         static_cast<float>(2048U - raw_freq);
}

/*
 * CH4 NR43:
 *
 *   frequency = 262144 / divisor / 2^shift
 *
 * divisor code 0은 0.5로 처리합니다.
 */
static float gb_noise_frequency(uint8_t nr43) {
  static constexpr float divisor_table[8] = {
    0.5f,
    1.0f,
    2.0f,
    3.0f,
    4.0f,
    5.0f,
    6.0f,
    7.0f,
  };

  const uint8_t divisor_code = nr43 & 0x07;
  const uint8_t clock_shift = (nr43 >> 4) & 0x0F;

  return 262144.0f /
         divisor_table[divisor_code] /
         static_cast<float>(1UL << clock_shift);
}

/*
 * GB volume 0~15를 Q8 0~255로 변환합니다.
 */
static int32_t gb_volume_to_q8(uint8_t volume) {
  return static_cast<int32_t>(volume & 0x0F) * 255 / 15;
}

static int32_t gb_initial_volume_q8(uint8_t envelope_reg) {
  return gb_volume_to_q8(envelope_reg >> 4);
}

static bool gb_dac_enabled(uint8_t envelope_reg) {
  /*
   * CH1/CH2/CH4 DAC는 envelope register 상위 5비트가 모두 0이면 꺼집니다.
   */
  return (envelope_reg & 0xF8) != 0;
}

static void gb_voice_stop(int voice_idx, float freq) {
  if (freq <= 0.0f) {
    freq = 440.0f;
  }

  Audio.play_wave(voice_idx, freq, 0);
  Audio.set_env(voice_idx, 0, 0);
  Audio.set_pitch_env(voice_idx, 0, 0, 0);
}


/* -------------------------------------------------------------------------- */
/* Waveform conversion                                                        */
/* -------------------------------------------------------------------------- */

static void gb_set_pulse_duty(int voice_idx, uint8_t duty) {
  switch (duty & 0x03) {
    case 0:
      Audio.set_wave(voice_idx, WAVE_SQUARE_12);
      break;

    case 1:
      Audio.set_wave(voice_idx, WAVE_SQUARE_25);
      break;

    case 2:
      Audio.set_wave(voice_idx, WAVE_SQUARE_50);
      break;

    case 3:
      Audio.set_wave(voice_idx, WAVE_SQUARE_75);
      break;
  }
}


/* -------------------------------------------------------------------------- */
/* Envelope conversion                                                        */
/* -------------------------------------------------------------------------- */

static void gb_set_volume_envelope(
    int voice_idx,
    uint8_t envelope_reg,
    uint32_t length_us) {

  const uint8_t period = envelope_reg & 0x07;
  const bool increase = (envelope_reg & 0x08) != 0;

  /*
   * GB volume envelope는 64 Hz 기준입니다.
   *
   * volume 한 단계:
   *   0~15 중 1단계
   *   Q8 환산 시 약 17
   */
  if (period != 0) {
    const uint32_t tick_us =
        static_cast<uint32_t>(period) * 1000000UL / 64UL;

    /*
     * Audio 엔진이 현재 볼륨에서 decay_step_q8을 빼는 구조라고 가정합니다.
     *
     * 증가: 음수
     * 감소: 양수
     */
    const int32_t step_q8 = increase ? -17 : 17;

    Audio.set_env(
        voice_idx,
        tick_us,
        step_q8);

    return;
  }

  /*
   * 자동 volume envelope가 없고 Length Counter가 켜졌다면
   * length_us 후 현재 볼륨을 한 번에 0으로 내립니다.
   */
  if (length_us != 0) {
    const int32_t initial_volume =
        gb_initial_volume_q8(envelope_reg);

    Audio.set_env(
        voice_idx,
        length_us,
        initial_volume);

    return;
  }

  Audio.set_env(voice_idx, 0, 0);
}


/* -------------------------------------------------------------------------- */
/* Length conversion                                                          */
/* -------------------------------------------------------------------------- */

static uint32_t gb_length_64_us(
    uint8_t length_value,
    bool enabled) {

  if (!enabled) {
    return 0;
  }

  const uint32_t length_ticks =
      64U - static_cast<uint32_t>(length_value & 0x3F);

  /*
   * Length Counter는 256 Hz 기준입니다.
   */
  return length_ticks * 1000000UL / 256UL;
}

static uint32_t gb_length_256_us(
    uint8_t length_value,
    bool enabled) {

  if (!enabled) {
    return 0;
  }

  const uint32_t length_ticks =
      256U - static_cast<uint32_t>(length_value);

  return length_ticks * 1000000UL / 256UL;
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
    Audio.set_pitch_env(
        GB_APU_CH1,
        0,
        0,
        0);

    return;
  }

  /*
   * GB에서는 sweep period 0을 내부적으로 8처럼 취급합니다.
   */
  if (period == 0) {
    period = 8;
  }

  const int32_t tick_us =
      static_cast<int32_t>(
          static_cast<uint32_t>(period) *
          1000000UL /
          128UL);

  /*
   * 실제 GB sweep:
   *
   *   delta = raw_freq >> shift
   *   next  = raw_freq +/- delta
   *
   * Audio API는 semitone 기반이므로 첫 sweep 결과를 반음으로 변환합니다.
   */
  const uint16_t delta = raw_freq >> shift;

  const int32_t next_raw =
      decrease
          ? static_cast<int32_t>(raw_freq) -
                static_cast<int32_t>(delta)
          : static_cast<int32_t>(raw_freq) +
                static_cast<int32_t>(delta);

  if (next_raw < 0 || next_raw > 2047) {
    Audio.set_pitch_env(
        GB_APU_CH1,
        0,
        0,
        0);

    return;
  }

  const float current_freq =
      gb_pulse_frequency(raw_freq);

  const float next_freq =
      gb_pulse_frequency(
          static_cast<uint16_t>(next_raw));

  const float semitones =
      12.0f * std::log2(next_freq / current_freq);

  int32_t semitone_step =
      static_cast<int32_t>(std::lround(semitones));

  if (semitone_step == 0) {
    semitone_step = semitones > 0.0f ? 1 : -1;
  }

  /*
   * 반복 sweep를 완전히 재현할 수 없으므로
   * 우선 한 단계 sweep를 목표값으로 지정합니다.
   */
  Audio.set_pitch_env(
      GB_APU_CH1,
      tick_us,
      semitone_step,
      semitone_step > 0 ? 1 : -1);
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
      static_cast<uint16_t>(nr13) |
      (static_cast<uint16_t>(nr14 & 0x07) << 8);

  const float freq = gb_pulse_frequency(raw_freq);

  if (!gb_dac_enabled(nr12)) {
    gb_voice_stop(GB_APU_CH1, freq);
    return;
  }

  gb_set_pulse_duty(
      GB_APU_CH1,
      nr11 >> 6);

  const int32_t volume_q8 =
      gb_initial_volume_q8(nr12);

  Audio.play_wave(
      GB_APU_CH1,
      freq,
      volume_q8);

  const uint32_t length_us =
      gb_length_64_us(
          nr11,
          (nr14 & 0x40) != 0);

  gb_set_volume_envelope(
      GB_APU_CH1,
      nr12,
      length_us);

  gb_set_ch1_sweep(raw_freq);
}

static void gb_trigger_ch2(void) {
  const uint8_t nr21 = audio_reg_read(0xFF16);
  const uint8_t nr22 = audio_reg_read(0xFF17);
  const uint8_t nr23 = audio_reg_read(0xFF18);
  const uint8_t nr24 = audio_reg_read(0xFF19);

  const uint16_t raw_freq =
      static_cast<uint16_t>(nr23) |
      (static_cast<uint16_t>(nr24 & 0x07) << 8);

  const float freq = gb_pulse_frequency(raw_freq);

  if (!gb_dac_enabled(nr22)) {
    gb_voice_stop(GB_APU_CH2, freq);
    return;
  }

  gb_set_pulse_duty(
      GB_APU_CH2,
      nr21 >> 6);

  const int32_t volume_q8 =
      gb_initial_volume_q8(nr22);

  Audio.play_wave(
      GB_APU_CH2,
      freq,
      volume_q8);

  const uint32_t length_us =
      gb_length_64_us(
          nr21,
          (nr24 & 0x40) != 0);

  gb_set_volume_envelope(
      GB_APU_CH2,
      nr22,
      length_us);

  Audio.set_pitch_env(
      GB_APU_CH2,
      0,
      0,
      0);
}

static void gb_trigger_ch3(void) {
  const uint8_t nr30 = audio_reg_read(0xFF1A);
  const uint8_t nr31 = audio_reg_read(0xFF1B);
  const uint8_t nr32 = audio_reg_read(0xFF1C);
  const uint8_t nr33 = audio_reg_read(0xFF1D);
  const uint8_t nr34 = audio_reg_read(0xFF1E);

  const uint16_t raw_freq =
      static_cast<uint16_t>(nr33) |
      (static_cast<uint16_t>(nr34 & 0x07) << 8);

  const float freq = gb_wave_frequency(raw_freq);

  /*
   * NR30 bit 7: CH3 DAC enable
   */
  if ((nr30 & 0x80) == 0) {
    gb_voice_stop(GB_APU_CH3, freq);
    return;
  }

  /*
   * NR32:
   *
   *   00: mute
   *   01: 100%
   *   10: 50%
   *   11: 25%
   */
  static constexpr int32_t output_level_q8[4] = {
      0,
      255,
      127,
      63,
  };

  const uint8_t output_level =
      (nr32 >> 5) & 0x03;

  const int32_t volume_q8 =
      output_level_q8[output_level];

  /*
   * 현재 Audio API에는 GB의 32 x 4-bit Wave RAM 파형을
   * 직접 전달할 방법이 없으므로 삼각파로 근사합니다.
   */
  Audio.set_wave(
      GB_APU_CH3,
      WAVE_TRIANGLE);

  Audio.play_wave(
      GB_APU_CH3,
      freq,
      volume_q8);

  const uint32_t length_us =
      gb_length_256_us(
          nr31,
          (nr34 & 0x40) != 0);

  if (length_us != 0 && volume_q8 != 0) {
    Audio.set_env(
        GB_APU_CH3,
        length_us,
        volume_q8);
  } else {
    Audio.set_env(
        GB_APU_CH3,
        0,
        0);
  }

  Audio.set_pitch_env(
      GB_APU_CH3,
      0,
      0,
      0);
}

static void gb_trigger_ch4(void) {
  const uint8_t nr41 = audio_reg_read(0xFF20);
  const uint8_t nr42 = audio_reg_read(0xFF21);
  const uint8_t nr43 = audio_reg_read(0xFF22);
  const uint8_t nr44 = audio_reg_read(0xFF23);

  const float freq =
      gb_noise_frequency(nr43);

  if (!gb_dac_enabled(nr42)) {
    gb_voice_stop(GB_APU_CH4, freq);
    return;
  }

  /*
   * 현재 파형 API에는 GB 7-bit / 15-bit LFSR 구분이 없으므로
   * 공통 WAVE_NOISE를 사용합니다.
   */
  Audio.set_wave(
      GB_APU_CH4,
      WAVE_NOISE);

  const int32_t volume_q8 =
      gb_initial_volume_q8(nr42);

  Audio.play_wave(
      GB_APU_CH4,
      freq,
      volume_q8);

  const uint32_t length_us =
      gb_length_64_us(
          nr41,
          (nr44 & 0x40) != 0);

  gb_set_volume_envelope(
      GB_APU_CH4,
      nr42,
      length_us);

  Audio.set_pitch_env(
      GB_APU_CH4,
      0,
      0,
      0);
}


/* -------------------------------------------------------------------------- */
/* Master control                                                             */
/* -------------------------------------------------------------------------- */

static void gb_update_master_volume(void) {
  const uint8_t nr50 = audio_reg_read(0xFF24);

  /*
   * 스테레오를 사용하지 않으므로 좌우 볼륨을 평균냅니다.
   *
   * NR50:
   *   bits 6:4: left volume  0~7
   *   bits 2:0: right volume 0~7
   */
  const uint8_t left_volume =
      (nr50 >> 4) & 0x07;

  const uint8_t right_volume =
      nr50 & 0x07;

  const uint16_t mono_volume =
      static_cast<uint16_t>(left_volume) +
      static_cast<uint16_t>(right_volume);

  const uint8_t master_volume =
      static_cast<uint8_t>(
          mono_volume * 255U / 14U);

  Audio.set_master_config(master_volume/8);
}

static void gb_update_master_enable(void) {
  const bool enabled =
      (audio_reg_read(0xFF26) & 0x80) != 0;

  Audio.set_enable(enabled);

  if (!enabled) {
    gb_voice_stop(GB_APU_CH1, 440.0f);
    gb_voice_stop(GB_APU_CH2, 440.0f);
    gb_voice_stop(GB_APU_CH3, 440.0f);
    gb_voice_stop(GB_APU_CH4, 440.0f);
  }
}


/* -------------------------------------------------------------------------- */
/* Public interface                                                           */
/* -------------------------------------------------------------------------- */

void audio_init(void) {
  std::memset(
      apu_mmio,
      0,
      sizeof(apu_mmio));

  Audio.set_wave(
      GB_APU_CH1,
      WAVE_SQUARE_12);

  Audio.set_wave(
      GB_APU_CH2,
      WAVE_SQUARE_12);

  Audio.set_wave(
      GB_APU_CH3,
      WAVE_TRIANGLE);

  Audio.set_wave(
      GB_APU_CH4,
      WAVE_NOISE);

  Audio.set_master_config(16);
  Audio.set_enable(false);
}

void audio_write(
    const uint16_t addr,
    const uint8_t val) {

  if (!audio_addr_valid(addr)) {
    return;
  }

  /*
   * MMIO mirror에 먼저 기록해야 trigger 함수에서 최신 값을 읽을 수 있습니다.
   */
  audio_reg(addr) = val;

  switch (addr) {
    /*
     * CH1
     */
    case 0xFF11: {
      gb_set_pulse_duty(
          GB_APU_CH1,
          val >> 6);
      break;
    }

    case 0xFF12: {
      if (!gb_dac_enabled(val)) {
        gb_voice_stop(
            GB_APU_CH1,
            440.0f);
      }
      break;
    }

    case 0xFF14: {
      if (val & 0x80) {
        gb_trigger_ch1();
      }
      break;
    }

    /*
     * CH2
     */
    case 0xFF16: {
      gb_set_pulse_duty(
          GB_APU_CH2,
          val >> 6);
      break;
    }

    case 0xFF17: {
      if (!gb_dac_enabled(val)) {
        gb_voice_stop(
            GB_APU_CH2,
            440.0f);
      }
      break;
    }

    case 0xFF19: {
      if (val & 0x80) {
        gb_trigger_ch2();
      }
      break;
    }

    /*
     * CH3
     */
    case 0xFF1A: {
      if ((val & 0x80) == 0) {
        gb_voice_stop(
            GB_APU_CH3,
            440.0f);
      }
      break;
    }

    case 0xFF1E: {
      if (val & 0x80) {
        gb_trigger_ch3();
      }
      break;
    }

    /*
     * CH4
     */
    case 0xFF21: {
      if (!gb_dac_enabled(val)) {
        gb_voice_stop(
            GB_APU_CH4,
            440.0f);
      }
      break;
    }

    case 0xFF23: {
      if (val & 0x80) {
        gb_trigger_ch4();
      }
      break;
    }

    /*
     * Mixer / master
     */
    case 0xFF24: {
      gb_update_master_volume();
      break;
    }

    case 0xFF25: {
      /*
       * NR51 stereo routing은 모노 구현이므로 무시합니다.
       */
      break;
    }

    case 0xFF26: {
      gb_update_master_enable();
      break;
    }

    /*
     * FF30~FF3F Wave RAM은 현재 Audio API로 전달할 수 없으므로
     * MMIO mirror에만 저장합니다.
     */
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