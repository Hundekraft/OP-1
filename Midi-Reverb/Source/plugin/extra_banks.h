/* GPL-3.0-only. Native programs and modulation from thement/midiverb_emulator
 * at b92e133c5c7db8d7a2fcbbffe4eed41945a3da2b. */
#pragma once
#include <stdint.h>

int16_t extra_render(void *state, int16_t input, int16_t *ring, unsigned ptr);
void extra_reset(void *state, unsigned bank, unsigned program);
enum { EXTRA_STATE_WORDS = 32 };
