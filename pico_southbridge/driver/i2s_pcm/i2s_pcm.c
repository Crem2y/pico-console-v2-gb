#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "pico/stdlib.h"
#include "pico/audio_i2s.h"

#include "i2s_pcm.h"
#include "system_time.h"

static int16_t square_12_wave_table[WAVE_TABLE_LEN];
static int16_t square_25_wave_table[WAVE_TABLE_LEN];
static int16_t square_50_wave_table[WAVE_TABLE_LEN];
static int16_t square_75_wave_table[WAVE_TABLE_LEN];
static int16_t triangle_wave_table[WAVE_TABLE_LEN];
static int16_t sawtooth_wave_table[WAVE_TABLE_LEN];
static int16_t noise_wave_table[WAVE_TABLE_LEN];
static int16_t sine_wave_table[WAVE_TABLE_LEN];
static int16_t custom0_wave_table[WAVE_TABLE_LEN];

static int _data_pin;
static int _clock_pin_base;
static int _mute_pin;

// -------------------- Wave tables --------------------
static uint32_t xorshift32(uint32_t *state)
{
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static void make_wave_tables(void) {
    int16_t noise_temp = -32768;
    uint32_t rng = 0x12345678;

    for (int i = 0; i < WAVE_TABLE_LEN; i++) {
        square_12_wave_table[i] = (i < WAVE_TABLE_LEN / 8) ? 32767 : -32768;
        square_25_wave_table[i] = (i < WAVE_TABLE_LEN / 4) ? 32767 : -32768;
        square_50_wave_table[i] = (i < WAVE_TABLE_LEN / 2) ? 32767 : -32768;
        square_75_wave_table[i] = (i < (WAVE_TABLE_LEN * 3) / 4) ? 32767 : -32768;
        // triangle ranges -32768..32767 without overflow
        if (i < WAVE_TABLE_LEN / 2) {
            triangle_wave_table[i] = (int16_t)(((i * 65535) / (WAVE_TABLE_LEN / 2)) - 32768);
        } else {
            triangle_wave_table[i] = (int16_t)((((WAVE_TABLE_LEN - i) * 65535) / (WAVE_TABLE_LEN / 2)) - 32768);
        }
        sawtooth_wave_table[i] = (int16_t)((( (WAVE_TABLE_LEN - i) * 65535) / WAVE_TABLE_LEN) - 32768);
        noise_wave_table[i] = (xorshift32(&rng) & 1) ? 32767 : -32768;
        sine_wave_table[i] = (int16_t)(32767.0f * cosf((float)i * 2.0f * (float)(M_PI / WAVE_TABLE_LEN)));
        custom0_wave_table[i] = 0;
    }
}

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

void voice_vol_env_set(int voice_idx, uint32_t tick_us, int32_t decay_step_q8) {
    if (voice_idx < 0 || voice_idx >= NUM_CHANNELS) return;
    if (tick_us == 0) tick_us = 1;
    g_voices[voice_idx].vol_env_tick_us = tick_us;
    g_voices[voice_idx].vol_env_decay_step_q8 = (decay_step_q8 < 0) ? 0 : decay_step_q8;
    g_voices[voice_idx].vol_env_next_us = get_system_time_us() + g_voices[voice_idx].vol_env_tick_us;
}

static inline void voice_vol_env_init(int voice_idx, uint32_t tick_us, int32_t decay_step_q8) {
    voice_vol_env_set(voice_idx, tick_us, decay_step_q8);
    g_voices[voice_idx].vol_env_q8 = 0;
}

static inline void voice_vol_env_note_on(voice_t *v, int32_t peak_vol_q8) {
    if (peak_vol_q8 < 0) peak_vol_q8 = 0;
    if (peak_vol_q8 > 256) peak_vol_q8 = 256;
    v->vol_q8 = peak_vol_q8;
    v->vol_env_q8 = peak_vol_q8;
    v->vol_env_next_us = get_system_time_us() + v->vol_env_tick_us;
}

static inline uint32_t pitch_env_tick_interval_us(int32_t tick_us) {
    if (tick_us == 0) return 1;
    if (tick_us == INT32_MIN) return (uint32_t)INT32_MAX + 1u;
    return (tick_us < 0) ? (uint32_t)(-tick_us) : (uint32_t)tick_us;
}

void voice_pitch_env_set(int voice_idx, int32_t tick_us, int32_t target_semitones, int32_t step) {
    if (voice_idx < 0 || voice_idx >= NUM_CHANNELS) return;
    g_voices[voice_idx].pit_env_tick_us = pitch_env_tick_interval_us(tick_us);
    g_voices[voice_idx].pit_env_target_semitones = target_semitones;
    g_voices[voice_idx].pit_env_current_target_semitones = target_semitones;
    g_voices[voice_idx].pit_env_vibrate = tick_us < 0;
    g_voices[voice_idx].pit_env_step = (step <= 0) ? 1 : step;
    g_voices[voice_idx].pit_env_next_us = get_system_time_us() + g_voices[voice_idx].pit_env_tick_us;
}

static inline void voice_pitch_env_init(int voice_idx, int32_t tick_us, int32_t target_semitones, int32_t step) {
    voice_pitch_env_set(voice_idx, tick_us, target_semitones, step);
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
    if (v->vol_env_q8 > 0 && v->vol_env_decay_step_q8 > 0) {
        // Catch up if we missed ticks (avoid depending on main loop cadence)
        while ((int32_t)(now_us - v->vol_env_next_us) >= 0) {
            v->vol_env_q8 -= v->vol_env_decay_step_q8;
            if (v->vol_env_q8 <= 0) {
                v->vol_env_q8 = 0;
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

static inline int32_t voice_next_sample_i32(voice_t *v) {
    const uint32_t pos_max = 0x10000u * (uint32_t)WAVE_TABLE_LEN;

    const int32_t s = (int32_t)v->table[v->pos >> 16u];
    // Apply envelope level (Q8)
    int32_t y = (s * v->vol_env_q8) >> 8;

    v->pos += v->step;
    if (v->pos >= pos_max) v->pos -= pos_max;

    return y;
}

static void render_buffer_mono_mix(int16_t *dst, uint32_t count) {
    uint32_t now_us = get_system_time_us();
    // Update envelopes once per buffer render (cheap). If you need tighter timing,
    // call voice_env_tick() inside the sample loop instead.
    for (int v = 0; v < NUM_CHANNELS; v++) {
        voice_env_tick(&g_voices[v], now_us);
    }
    for (uint32_t i = 0; i < count; i++) {
        int32_t acc = 0;
        // 4 fixed voices mixed into mono
        for (int v = 0; v < NUM_CHANNELS; v++) {
            acc += voice_next_sample_i32(&g_voices[v]);
        }

        // Simple headroom to reduce clipping when multiple voices stack.
        // For NUM_CHANNELS=4, shifting by 2 approximates /4.
        acc >>= 4; // acc /= 16
        acc = (acc * master_volume) / 256;

        // Clip to int16 range
        if (acc > 32767) acc = 32767;
        if (acc < -32768) acc = -32768;
        // Left channel
        dst[i * 2 + 0] = (int16_t)acc;
        // Right channel
        dst[i * 2 + 1] = (int16_t)acc;
    }
}

static const int16_t *wave_table_ptr(wave_t w) {
    switch (w) {
        case WAVE_SQUARE_12:    return square_12_wave_table;
        case WAVE_SQUARE_25:    return square_25_wave_table;
        case WAVE_SQUARE_50:    return square_50_wave_table;
        case WAVE_SQUARE_75:    return square_75_wave_table;
        case WAVE_TRIANGLE:     return triangle_wave_table;
        case WAVE_SAWTOOTH:     return sawtooth_wave_table;
        case WAVE_NOISE:        return noise_wave_table;
        case WAVE_SINE:         return sine_wave_table;
        case WAVE_CUSTOM_0:     return custom0_wave_table;
        default:                return square_50_wave_table;
    }
}

void set_voice_waveform(int voice_idx, wave_t w) {
    if (voice_idx < 0 || voice_idx >= NUM_CHANNELS) return;
    g_voices[voice_idx].table = wave_table_ptr(w);
    g_voices[voice_idx].wave = w;
}

static bool set_voice_note(int voice_idx, float freq) {
    if (voice_idx < 0 || voice_idx >= NUM_CHANNELS) return false;
    g_voices[voice_idx].base_step = step_from_hz(freq, AUDIO_FS_HZ);
    g_voices[voice_idx].step = voice_step_with_pitch_env(&g_voices[voice_idx]);
    return true;
}

static void set_voice_volume_q8(int voice_idx, int32_t vol_q8) {
    if (voice_idx < 0 || voice_idx >= NUM_CHANNELS) return;
    if (vol_q8 < 0) vol_q8 = 0;
    if (vol_q8 > 256) vol_q8 = 256;
    g_voices[voice_idx].vol_q8 = vol_q8;
    // Do not directly change vol_env_q8 here; vol_env_q8 is controlled by note triggers.
}

void voice_note_on(int voice_idx, float freq, int32_t peak_vol_q8) {
    if (!set_voice_note(voice_idx, freq)) return;
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

void set_custom_wave(const int16_t* wave_data) {
    if(wave_data == NULL) return;
    memcpy(custom0_wave_table, wave_data, (sizeof(int16_t) * WAVE_TABLE_LEN));
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
        voice_vol_env_init(i, 25000, 1);
        voice_pitch_env_init(i, 25000, 0, 0);
    }


}