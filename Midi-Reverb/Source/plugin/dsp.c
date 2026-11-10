/* SPDX-License-Identifier: GPL-3.0-only */
#include "dsp.h"
#include "programs.h"

struct coefficients { short b0, a1, a2; };
#include "tables.h"

unsigned tank_rom_crc(void) { return MV1_ROM_CRC; }

static inline int clamp(int x, int lo, int hi)
{
    return x < lo ? lo : x > hi ? hi : x;
}

static inline int product16(int x, int y)
{
#ifdef __bfin__
    return __builtin_bfin_mult_fr1x32((short)x, (short)y) >> 1;
#else
    return (int)(short)x * (short)y;
#endif
}

/* Three damped combs in parallel, followed by two short allpasses.
 * Delay partitions are powers of two so each address wraps with a mask. */
struct lite_profile {
    unsigned short length[3];
    short gain[3];
    unsigned char damping;
};
#include "lite_profiles.h"

static const struct lite_profile *lite_profile(unsigned bank, unsigned program)
{
    static const struct lite_profile *const profiles[] = {lite_0, lite_1, lite_2};
    const struct lite_profile *profile = &profiles[bank][program];
    return profile->length[0] ? profile : 0;
}

static inline int lite_allpass(int16_t *line, unsigned pointer, unsigned length, int input)
{
    unsigned read = (pointer - length) & 255;
    int delayed = line[read];
    int output = clamp(delayed - input / 2, -32768, 32767);
    line[pointer & 255] = (int16_t)clamp(input + output / 2, -32768, 32767);
    return output;
}

static int lite_tick(struct tank *s, const struct lite_profile *profile, int input)
{
    unsigned pointer = s->pointer;
    int sum = 0;
    for (int i = 0; i < 3; ++i) {
        int16_t *line = s->ring + i * 2048;
        int delayed = line[(pointer - profile->length[i]) & 2047];
        int damp = delayed - ((delayed - s->lite_damp[i]) * profile->damping >> 2);
        s->lite_damp[i] = damp;
        int scaled = product16(damp, profile->gain[i]);
        int decay = (scaled + ((scaled >> 31) & 32767)) >> 15;
        int next = input / 2 + decay;
        line[pointer & 2047] = (int16_t)clamp(next, -32768, 32767);
        sum += delayed;
    }
    int sample = lite_allpass(s->ring + 6144, pointer, 113, sum >> 2);
    sample = lite_allpass(s->ring + 6400, pointer, 173, sample);
    s->pointer = (pointer + 1) & (RING_WORDS - 1);
    return sample;
}

/* Ideal-op-amp transfer of MIDIVerb_RE's MV_DAC_Filter.cir. The
 * bilinear transform at 44.1 kHz is prewarped at its 10.136 kHz
 * -3 dB point. Q14 numerator is 3762 * [1, 3, 3, 1]. */
static inline int reconstruct(struct tank *s, int sample)
{
    int feed = product16(sample, 3762);
    int sum = feed + s->dac_z1;
    int rounded = sum + 8192 - (int)((unsigned)sum >> 31);
    int out = clamp(rounded >> 14, -32768, 32767);
    int triple = feed * 3;
    s->dac_z1 = triple - product16(out, 10421) + s->dac_z2;
    s->dac_z2 = triple - product16(out, 619) + s->dac_z3;
    s->dac_z3 = feed - product16(out, 2675);
    return out;
}

static inline int interpolate(int a, int b, unsigned fraction)
{
    return a + (((b - a) * (int)fraction) >> 11);
}

void tank_reset(struct tank *s)
{
    s->clear_at = 0;
    s->pointer = 0;
    s->phase = 0;
    s->acc = 0;
    s->lite_damp[0] = s->lite_damp[1] = s->lite_damp[2] = 0;
    s->input_old = s->wet_old = s->wet_new = s->feedback = s->z1 = s->z2 = 0;
    s->dac_z1 = s->dac_z2 = s->dac_z3 = 0;
}

void tank_render(struct tank *s, short *buf, const unsigned controls[4], unsigned bank, int reconstruction, int low_cpu)
{
    if (!reconstruction) s->dac_z1 = s->dac_z2 = s->dac_z3 = 0;
    int wet = wet_gain[controls[3]], dry = wet_gain[100 - controls[3]];
    if (s->clear_at < RING_WORDS) {
        for (int i = 0; i < 2048; ++i) s->ring[s->clear_at + i] = 0;
        s->clear_at += 2048;
        if (wet)
            for (int i = 0; i < 64; ++i) buf[i] = (short)(product16(buf[i], dry) >> 15);
        return;
    }
    const struct lite_profile *lite = low_cpu ? lite_profile(bank, controls[0]) : 0;
    const struct coefficients *c = &lowpass[controls[1]];
    int feedback = control_gain[controls[2]];
    int16_t (*tick)(int16_t, int16_t *, unsigned *, int16_t *) = bank ? 0 : programs[controls[0]];
    for (int i = 0; i < 64; ++i) {
        int input = buf[i];
        if (feedback)
            input = clamp(input + ((s->feedback * feedback) >> 12), -65536, 65535);
        int feed = c->b0 * input;
        int filtered = clamp((feed + s->z1 + 2048) >> 12, -65536, 65535);
        s->z1 = 2 * feed - c->a1 * filtered + s->z2;
        s->z2 = feed - c->a2 * filtered;
        /* 6 MHz / 256 relative to the OP-1's 44.1 kHz is 625/1176. */
        unsigned phase = s->phase + 625;
        if (phase >= 1176) {
            phase -= 1176;
            int at_tick = interpolate(filtered, s->input_old, input_fraction[phase]);
            int16_t in = (int16_t)clamp((at_tick >> 4) * 2, -4096, 4094);
            int sample;
            if (lite) sample = lite_tick(s, lite, in);
            else if (tick) sample = tick(in, s->ring, &s->pointer, &s->acc);
            else {
                sample = extra_render(s->extra, in, s->ring, s->pointer);
                s->pointer = (s->pointer + 1) & (RING_WORDS - 1);
            }
            s->wet_old = s->wet_new;
            s->wet_new = clamp(sample, -4096, 4095) * 8;
        }
        s->phase = phase;
        s->input_old = filtered;
        int sample = interpolate(s->wet_old, s->wet_new, output_fraction[phase]);
        s->feedback = sample;
        /* Keep reconstruction outside the feedback loop and off the dry path. */
        int reconstructed = reconstruction ? reconstruct(s, sample) : sample;
        if (wet)
            buf[i] = (short)clamp((product16(buf[i], dry) + product16(reconstructed, wet)) >> 15,
                                  -32768, 32767);
    }
}
