#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "delay_dsp.h"
void ref_init(void *);void ref_render(void *,short *,short *,short *,int,const short *,int,int);
int dl_test_smooth_wave(struct dl_state *,unsigned,unsigned,int,int *);
static unsigned rng=1234567;
static unsigned rnd(void){rng=rng*1664525u+1013904223u;return rng;}
static void controls(void)
{
 const unsigned inc=973915u;int random_jump=0;
 for(int shape=2;shape<=5;++shape)for(int off=0;off<4;++off){
  struct dl_state s;dl_init(&s);s.random_from=12345;s.random_to=-9876;
  unsigned phase=(unsigned)off*0x40000000u;int gain=32768,previous_gain=32768,previous=0,seen=0,wraps=0;
  for(int i=0;i<16000;++i){unsigned old=phase;phase+=inc;int v=dl_test_smooth_wave(&s,phase,inc,shape,&gain);
   if(phase<old || (shape==2 && ((phase^old)&0x80000000u))){++wraps;if(seen){if(shape==5){int jump=abs(v-previous);if(jump>random_jump)random_jump=jump;assert(jump<64);}else {assert(gain==0);assert(previous_gain<=512);}}}
   if(shape==5)assert(gain==32768);
   assert(gain>=0&&gain<=32768);previous=v;previous_gain=gain;seen=1;
  }
  assert(wraps>=3);
 }
 printf("Smooth control: square/ramp resets muted; random wrap step <=%d at four phase offsets\n",random_jump);
}
static void legacy(void)
{
 struct dl_state s;unsigned char ref[1024];short *a=calloc(DL_SAMPLES,2),*b=calloc(DL_SAMPLES,2);assert(a&&b);
 dl_set_smooth(0);dl_init(&s);memset(ref,0,sizeof ref);ref_init(ref);
 short par[8]={21760,8192,32767,16384,14208,0,0,16384};
 for(int k=0;k<24000;++k){
  if(k%97==0){for(int j=0;j<8;++j)par[j]=(short)(rnd()&32767);par[6]=(short)(rnd()%6);}
  short l[64],r[64],rl[64],rr[64];for(int i=0;i<64;++i){l[i]=rl[i]=(short)rnd();r[i]=rr[i]=(short)rnd();}
  int res=(int)(rnd()%64),freeze=k%197<3;
  dl_render(&s,a,l,k&1?r:0,64,par,res,freeze);ref_render(ref,b,rl,k&1?rr:0,64,par,res,freeze);
  assert(!memcmp(l,rl,sizeof l));if(k&1)assert(!memcmp(r,rr,sizeof r));
 }
 free(a);free(b);puts("Smooth Off: 1.536M samples bit-identical to Delayed 0.3.4");
}
int main(void){controls();legacy();return 0;}
