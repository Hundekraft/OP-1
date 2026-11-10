#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "delay_dsp.h"
static unsigned seed=12345;
static unsigned rnd(void){seed=seed*1664525u+1013904223u;return seed;}
int main(void)
{
 dl_set_smooth(1);
 struct dl_state s;short *ring=malloc(DL_SAMPLES*sizeof *ring);assert(ring);
 short p[8]={0,0,32767,32767,0,0,0,16384},l[64],r[64];
 /* Poisoned shared memory must never leak before valid history. */
 for(unsigned i=0;i<DL_SAMPLES;++i)ring[i]=12345;
 dl_init(&s);memset(l,0,sizeof l);dl_render(&s,ring,l,0,64,p,0,0);
 for(int i=0;i<64;++i)assert(l[i]==0);
 /* A fully wet impulse arrives at exactly the selected integer delay. */
 dl_init(&s);int delay=dl_time(0);assert(delay==44);
 memset(l,0,sizeof l);l[0]=16000;dl_render(&s,ring,l,0,64,p,0,0);
 for(int i=0;i<delay;++i)assert(l[i]==0);
 assert(l[delay]>14000 && l[delay]<16000);
 /* Zero mix preserves both dry channels exactly, including full scale. */
 p[3]=0;dl_render(&s,ring,l,r,64,p,63,0);
 for(int k=0;k<100;++k){short a[64],b[64];for(int i=0;i<64;++i)a[i]=l[i]=(short)rnd(),b[i]=r[i]=(short)rnd();dl_render(&s,ring,l,r,64,p,63,0);assert(!memcmp(a,l,sizeof l));assert(!memcmp(b,r,sizeof r));}
 /* Freeze recirculates an unchanged loop, independent of input/filter/FB. */
 dl_init(&s);s.ready=1;s.time_q8=100*256;s.filled=100;s.mix=32767;
 for(int i=0;i<100;++i)ring[DL_SAMPLES-100u+(unsigned)i]=(short)(i*10-500);
 p[3]=32767;p[1]=32767;
 for(int k=0;k<500;++k){for(int i=0;i<64;++i)l[i]=32767;dl_render(&s,ring,l,0,64,p,63,1);for(int i=0;i<64;++i)assert(l[i]==2*(((k*64+i)%100)*10-500));}
 /* All shapes, phase endpoints, max feedback/resonance, ring wrap, changes. */
 for(int k=0;k<40000;++k){
  if(k%13==0){for(int j=0;j<8;++j)p[j]=(short)(rnd()&32767);p[6]=(short)(rnd()%6);}
  for(int i=0;i<64;++i)l[i]=(short)rnd(),r[i]=(short)rnd();
  dl_render(&s,ring,l,k&1?r:0,64,p,(int)(rnd()%64),k%97<3);
  assert(s.write<DL_SAMPLES && s.filled<=DL_SAMPLES);
 }
 assert(dl_wave(0,0,0,0)==0);assert(dl_wave(0x40000000u,0,0,0)>32760);
 assert(dl_wave(0x80000000u,0,0,0)==0);assert(dl_wave(0xc0000000u,0,0,0)<-32760);
 assert(dl_time(32767)==44100);assert(dl_freq(32767)==20000);
 free(ring);puts("DSP: impulse, dry stereo, exact freeze, history isolation and 2.56M stress samples passed");
}
