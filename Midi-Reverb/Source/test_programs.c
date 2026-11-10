/* Compile with MIDIVerb_RE's midiverb.c and the ROM-derived mv_ucode.h. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "midiverb.h"
#include "plugin/programs.h"

extern uint16_t addr;
extern int16_t acc, mem[16384];
extern void (*mv_progs[63])(int16_t, int16_t *, int16_t *);

int main(void)
{
    static mvblk ref;
    static int16_t ring[16384];
    unsigned random = 42;
    for (unsigned p = 0; p < 63; ++p) {
        unsigned ptr = 0;
        int16_t state = 0;
        acc = addr = 0;
        memset(mem, 0, sizeof ring);
        midiverb_Init(&ref);
        midiverb_SetProg(&ref, p);
        memset(ring, 0, sizeof ring);
        for (unsigned i = 0; i < 32768; ++i) {
            random = random * 1664525u + 1013904223u;
            int16_t in[2] = {(int16_t)(random >> 16), (int16_t)random}, out[2];
            midiverb_Proc(&ref, in, out);
            int16_t mono = (int16_t)(((in[0] >> 4) + (in[1] >> 4)) & 0xfffe);
            int16_t left, right;
            mv_progs[p](mono, &left, &right);
            int x = programs[p](mono, ring, &ptr, &state);
            assert(x == left && ptr == addr && state == acc);
            if (x > 4095) x = 4095;
            if (x < -4096) x = -4096;
            assert(x * 8 == out[0]);
            assert(ptr == ref.asum);
        }
        assert(memcmp(ring, mem, sizeof ring) == 0);
    }
    puts("PASS: 63 mono programs match ROM interpreter output and compiled delay state");
}
