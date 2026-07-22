#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "pico/stdlib.h"
#include "pico/audio_i2s.h"

#include "i2s_pcm.h"
#include "system_time.h"

static int _data_pin;
static int _clock_pin_base;
static int _mute_pin;

// convert freq to step
static inline uint32_t step_from_hz(float f_hz, uint32_t fs_hz) {
    // step = round( (WAVE_TABLE_LEN * 65536 * f) / Fs )
    // Use double for precision; called only on note changes.
    const double num = (double)WAVE_TABLE_LEN * 65536.0 * (double)f_hz;
    double step = num / (double)fs_hz;
    if (step < 0.0) step = 0.0;
    if (step > 4294967295.0) step = 4294967295.0;
    return (uint32_t)(step + 0.5);
}

// -------------------- Simple synth state --------------------

voice_t g_voices[NUM_CHANNELS];
int32_t master_volume;

void set_master_volume(uint8_t vol) {
    master_volume = vol;
}

void set_voice_vol_env(int voice_idx, uint32_t tick_us, int32_t decay_step_q8) {
    if (voice_idx < 0 || voice_idx >= NUM_CHANNELS) return;
    if (tick_us == 0) tick_us = 1;
    g_voices[voice_idx].vol_env_tick_us = tick_us;
    g_voices[voice_idx].vol_env_decay_step_q8 = (decay_step_q8 < 0) ? 0 : decay_step_q8;
    g_voices[voice_idx].vol_env_next_us = get_system_time_us() + g_voices[voice_idx].vol_env_tick_us;
}

static inline void voice_vol_env_init(int voice_idx, uint32_t tick_us, int32_t decay_step_q8) {
    set_voice_vol_env(voice_idx, tick_us, decay_step_q8);
}

static inline void voice_vol_env_note_on(voice_t *v, int32_t peak_vol_q8) {
    if (peak_vol_q8 < 0) peak_vol_q8 = 0;
    if (peak_vol_q8 > 256) peak_vol_q8 = 256;
    v->vol_q8 = peak_vol_q8;
    v->vol_env_next_us = get_system_time_us() + v->vol_env_tick_us;
}

static inline uint32_t pitch_env_tick_interval_us(int32_t tick_us) {
    if (tick_us == 0) return 1;
    if (tick_us == INT32_MIN) return (uint32_t)INT32_MAX + 1u;
    return (tick_us < 0) ? (uint32_t)(-tick_us) : (uint32_t)tick_us;
}

void set_voice_pitch_env(int voice_idx, int32_t tick_us, int32_t target_semitones, int32_t step) {
    if (voice_idx < 0 || voice_idx >= NUM_CHANNELS) return;
    g_voices[voice_idx].pit_env_tick_us = pitch_env_tick_interval_us(tick_us);
    g_voices[voice_idx].pit_env_target_semitones = target_semitones;
    g_voices[voice_idx].pit_env_current_target_semitones = target_semitones;
    g_voices[voice_idx].pit_env_vibrate = tick_us < 0;
    g_voices[voice_idx].pit_env_step = (step <= 0) ? 1 : step;
    g_voices[voice_idx].pit_env_next_us = get_system_time_us() + g_voices[voice_idx].pit_env_tick_us;
}

static inline void voice_pitch_env_init(int voice_idx, int32_t tick_us, int32_t target_semitones, int32_t step) {
    set_voice_pitch_env(voice_idx, tick_us, target_semitones, step);
    g_voices[voice_idx].pit_env_semitones = 0;
}

static inline uint32_t voice_step_with_pitch_env(const voice_t *v) {
    double step = (double)v->base_step;
    if (v->pit_env_semitones != 0) {
        step *= pow(2.0, (double)v->pit_env_semitones / 128.0);
    }
    if (step > 4294967295.0) return UINT32_MAX;
    if (step < 0.0) return 0;
    return (uint32_t)(step + 0.5);
}

static inline void voice_pitch_env_note_on(voice_t *v) {
    v->pit_env_semitones = 0;
    v->pit_env_current_target_semitones = v->pit_env_target_semitones;
    v->pit_env_next_us = get_system_time_us() + v->pit_env_tick_us;
    v->step = voice_step_with_pitch_env(v);
}

static inline bool voice_pitch_env_move_toward(voice_t *v, int32_t target) {
    if (v->pit_env_semitones < target) {
        v->pit_env_semitones += v->pit_env_step;
        if (v->pit_env_semitones >= target) {
            v->pit_env_semitones = target;
            return true;
        }
    } else if (v->pit_env_semitones > target) {
        v->pit_env_semitones -= v->pit_env_step;
        if (v->pit_env_semitones <= target) {
            v->pit_env_semitones = target;
            return true;
        }
    } else {
        return true;
    }
    return false;
}

static inline void voice_env_tick(voice_t *v, uint32_t now_us) {
    // Simple linear decay: vol_env_q8 -= vol_env_decay_step_q8 every vol_env_tick_us
    if (v->vol_q8 > 0 && v->vol_env_tick_us > 0 && v->vol_env_decay_step_q8 > 0) {
        // Catch up if we missed ticks (avoid depending on main loop cadence)
        while ((int32_t)(now_us - v->vol_env_next_us) >= 0) {
            v->vol_q8 -= v->vol_env_decay_step_q8;
            if (v->vol_q8 <= 0) {
                v->vol_q8 = 0;
                break;
            }
            v->vol_env_next_us += v->vol_env_tick_us;
        }
    }

    const bool pit_env_active = v->pit_env_vibrate
        ? (v->pit_env_target_semitones != 0)
        : (v->pit_env_semitones != v->pit_env_target_semitones);

    if (pit_env_active) {
        while ((int32_t)(now_us - v->pit_env_next_us) >= 0) {
            const int32_t target = v->pit_env_vibrate
                ? v->pit_env_current_target_semitones
                : v->pit_env_target_semitones;
            const bool reached = voice_pitch_env_move_toward(v, target);

            if (reached && v->pit_env_vibrate) {
                v->pit_env_current_target_semitones = -target;
            }
            v->pit_env_next_us += v->pit_env_tick_us;

            if (!v->pit_env_vibrate && reached) break;
        }
        v->step = voice_step_with_pitch_env(v);
    }
}

static inline void voice_next_sample_i32(voice_t *v, int32_t* acc_l, int32_t* acc_r) {
    const uint32_t pos_max = 0x10000u * (uint32_t)WAVE_TABLE_LEN;

    const int32_t s = (int32_t)v->table[v->pos >> 16u];

    int32_t y = (s * v->vol_q8) / 255;

    int32_t l = (y * v->vol_l_q8) / 255;
    int32_t r = (y * v->vol_r_q8) / 255;

    v->pos += v->step;
    if (v->pos >= pos_max) v->pos -= pos_max;

    *acc_l += l;
    *acc_r += r;
}

static void render_buffer_mono_mix(int16_t *dst, uint32_t count) {
    uint32_t now_us = get_system_time_us();
    // Update envelopes once per buffer render (cheap). If you need tighter timing,
    // call voice_env_tick() inside the sample loop instead.
    for (int v = 0; v < NUM_CHANNELS; v++) {
        voice_env_tick(&g_voices[v], now_us);
    }

    for (uint32_t i = 0; i < count; i++) {
        int32_t acc_l = 0;
        int32_t acc_r = 0;

        for (int v = 0; v < NUM_CHANNELS; v++) {
            voice_next_sample_i32(&g_voices[v], &acc_l, &acc_r);
        }

        // Simple headroom to reduce clipping when multiple voices stack.
        // For NUM_CHANNELS=4, shifting by 2 approximates /4.
        acc_l >>= 4; // acc /= 16
        acc_l = (acc_l * master_volume) / 256;

        acc_r >>= 4; // acc /= 16
        acc_r = (acc_r * master_volume) / 256;

        // Clip to int16 range
        if (acc_l > 32767) acc_l = 32767;
        if (acc_l < -32768) acc_l = -32768;

        if (acc_r > 32767) acc_r = 32767;
        if (acc_r < -32768) acc_r = -32768;

        // Left channel
        dst[i * 2 + 0] = (int16_t)acc_l;
        // Right channel
        dst[i * 2 + 1] = (int16_t)acc_r;
    }
}

void set_voice_waveform(int voice_idx, wave_t w) {
    if (voice_idx < 0 || voice_idx >= NUM_CHANNELS) return;
    g_voices[voice_idx].table = wave_table_ptr(w);
    g_voices[voice_idx].wave = w;
}

void set_voice_freq(int voice_idx, float freq) {
    if (voice_idx < 0 || voice_idx >= NUM_CHANNELS) return;
    g_voices[voice_idx].base_step = step_from_hz(freq, AUDIO_FS_HZ);
    g_voices[voice_idx].step = voice_step_with_pitch_env(&g_voices[voice_idx]);
}

void set_voice_volume_q8(int voice_idx, int32_t vol_q8) {
    if (voice_idx < 0 || voice_idx >= NUM_CHANNELS) return;

    if (vol_q8 < 0) vol_q8 = 0;
    if (vol_q8 > 255) vol_q8 = 255;

    g_voices[voice_idx].vol_q8 = vol_q8;
}

void set_voice_lr_volume_q8(int voice_idx, int32_t vol_l_q8, int32_t vol_r_q8) {
    if (voice_idx < 0 || voice_idx >= NUM_CHANNELS) return;

    if (vol_l_q8 < 0) vol_l_q8 = 0;
    if (vol_l_q8 > 255) vol_l_q8 = 255;
    if (vol_r_q8 < 0) vol_r_q8 = 0;
    if (vol_r_q8 > 255) vol_r_q8 = 255;

    g_voices[voice_idx].vol_l_q8 = vol_l_q8;
    g_voices[voice_idx].vol_r_q8 = vol_r_q8;
}

void voice_note_on(int voice_idx, float freq, int32_t peak_vol_q8) {
    set_voice_freq(voice_idx, freq);
    voice_vol_env_note_on(&g_voices[voice_idx], peak_vol_q8);
    voice_pitch_env_note_on(&g_voices[voice_idx]);
}

// -------------------- Audio init --------------------
static struct audio_buffer_pool *init_audio(void) {
    static audio_format_t audio_format = {
        .format = AUDIO_BUFFER_FORMAT_PCM_S16,
        .sample_freq = AUDIO_FS_HZ,
        .channel_count = 2,
    };

    static struct audio_buffer_format producer_format = {
        .format = &audio_format,
        .sample_stride = 4,
    };

    struct audio_buffer_pool *producer_pool = audio_new_producer_pool(&producer_format,
                                                                      3,
                                                                      SAMPLES_PER_BUFFER);
    bool ok;

    struct audio_i2s_config config = {
        .data_pin = _data_pin,
        .clock_pin_base = _clock_pin_base,
        .dma_channel = 0,
        .pio_sm = 0,
    };

    const struct audio_format *output_format = audio_i2s_setup(&audio_format, &config);
    if (!output_format) {
        panic("PicoAudio: Unable to open audio device\n");
    }

    ok = audio_i2s_connect(producer_pool);
    assert(ok);
    audio_i2s_set_enabled(true);

    return producer_pool;
}

void set_mute(bool mute) {
    gpio_put(_mute_pin, !mute);
}

void audio_loop(void) {

    struct audio_buffer_pool *ap = init_audio();

    while(true) {
        struct audio_buffer *buffer = take_audio_buffer(ap, true);
        int16_t *samples = (int16_t *)buffer->buffer->bytes;

        render_buffer_mono_mix(samples, buffer->max_sample_count);

        buffer->sample_count = buffer->max_sample_count;
        give_audio_buffer(ap, buffer);
    }
}

/**
 * @brief 
 * 
 * @param data_pin i2s data pin
 * @param clock_pin_base i2s clock pin base (clock_pin_base = bit clock, clock_pin_base+1 = word select)
 * @return struct audio_buffer_pool* 
 */
void audio_init(int data_pin, int clock_pin_base, int mute_pin) {
    _data_pin = data_pin;
    _clock_pin_base = clock_pin_base;
    _mute_pin = mute_pin;

    gpio_init(_mute_pin);
    gpio_set_dir(_mute_pin, GPIO_OUT);
    set_mute(true);

    make_wave_tables();
    memset(g_voices, 0, sizeof(g_voices));
    set_master_volume(127);

    for (int i = 0; i < NUM_CHANNELS; i++) {
        set_voice_waveform(i, WAVE_SQUARE_50);
        set_voice_volume_q8(i, 8);
        set_voice_lr_volume_q8(i, 255, 255);
        voice_vol_env_init(i, 25000, 1);
        voice_pitch_env_init(i, 25000, 0, 0);
    }


}