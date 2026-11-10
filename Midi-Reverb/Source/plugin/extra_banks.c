/* SPDX-License-Identifier: GPL-3.0-only */
#include "extra_banks.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "decompiled-midifex.h"
#include "decompiled-midiverb2.h"
#pragma GCC diagnostic pop

#define ARRAY_SIZE(a) (sizeof (a) / sizeof *(a))
#include "upstream_lfo.h"

typedef void (*effect_fn)(int16_t, int16_t *, int16_t *, int16_t *, int, uint32_t, uint32_t);

struct __attribute__((may_alias)) extra_state {
    effect_fn run;
    Lfo lfo1, lfo2;
    LfoPatch *patch;
    uint32_t values[2];
    unsigned counter;
};
_Static_assert(sizeof(struct extra_state) <= EXTRA_STATE_WORDS * sizeof(unsigned), "extra state");

static void update(struct extra_state *state)
{
    state->values[0] = state->lfo1.update(&state->lfo1) | ((uint32_t)state->patch->top1 << 16);
    state->values[1] = state->lfo2.update(&state->lfo2) | ((uint32_t)state->patch->top2 << 16);
}

void extra_reset(void *raw, unsigned bank, unsigned program)
{
    struct extra_state *state = raw;
    state->patch = 0;
    state->counter = state->values[0] = state->values[1] = 0;
    state->run = bank == 1 ? midifex_effects[program] : midiverb2_effects[program];
    if (bank == 2 && init_lfo_for_program((int)program, &state->lfo1, &state->lfo2, &state->patch))
        update(state);
}

int16_t extra_render(void *raw, int16_t input, int16_t *ring, unsigned ptr)
{
    struct extra_state *state = raw;
    int16_t left = 0, right = 0;
    state->run(input, &left, &right, ring, (int)ptr, state->values[0], state->values[1]);
    if (state->patch && ++state->counter == 8) {
        state->counter = 0;
        update(state);
    }
    return left;
}
