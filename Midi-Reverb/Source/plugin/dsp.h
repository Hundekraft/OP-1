/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
#include "extra_banks.h"

enum { RING_WORDS = 16384, PROGRAM_COUNT = 63 };
struct tank {
    int16_t *ring;
    unsigned clear_at, pointer, phase;
    int16_t acc;
    _Alignas(8) unsigned extra[EXTRA_STATE_WORDS];
    int input_old, wet_old, wet_new, feedback, z1, z2;
    int lite_damp[3];
    int dac_z1, dac_z2, dac_z3;
};
void tank_reset(struct tank *tank);
void tank_render(struct tank *tank, short *buf, const unsigned controls[4], unsigned bank, int reconstruction, int low_cpu);
unsigned tank_rom_crc(void);
