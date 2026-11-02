/* SPDX-License-Identifier: GPL-3.0-or-later
 * Fixed-point port/adaptation of Tom Erbe / SoundHack +delay~.
 * BSD original source and terms accompany this prototype.
 */
#include "delay_dsp.h"
#include "tables.h"
/* Tight six-second history plus interpolation guard, without power-of-two
 * rounding. Inputs to wrap are strictly inside (-SAMPLES,2*SAMPLES). */
static unsigned wrap(int i)
{if(i<0)i+=(int)DL_SAMPLES;else if(i>=(int)DL_SAMPLES)i-=(int)DL_SAMPLES;return (unsigned)i;}
static int clamp(int x,int a,int b) {return x<a?a:x>b?b:x;}
static int mul15(int x,int y) {return (x*y)>>15;}
/* a is nonnegative Q8 delay, b signed Q15. Exact signed floor without muldi3. */
static int delay_product(int a,int b)
{return (a>>15)*b+(((a&32767)*b)>>15);}
static int lerp31(int a,int b,unsigned phase)
{
 int d=b-a,negative=d<0;
 unsigned u=(unsigned)(negative?-d:d),f=phase>>1;
 unsigned h=(unsigned short)u*(unsigned short)(f>>16);
 unsigned l=(unsigned)(unsigned short)u*(unsigned short)f;
 unsigned low=(h<<16)+l,high=(h>>16)+(low<l);
 int q=(int)((high<<1)|(low>>31));
 return a+(negative?-q-((low&0x7fffffffu)!=0):q);
}
/* Exact 32xbounded-20-bit products for Q28 feedback filtering.
 * Coefficients are < 2^30 and sample sums < 2^20; cross terms fit int32.
 * Separate words preserve the fractional carries of the original 64-bit MAC. */
struct filter_word {unsigned lo;int hi;};
static inline __attribute__((always_inline)) struct filter_word filter_product(int c,int x)
{
 short ch=(short)(c>>16),xh=(short)(x>>16);
 unsigned short cl=(unsigned short)c,xl=(unsigned short)x;
 unsigned base=(unsigned)cl*xl;
 int cross=ch*(int)xl+xh*(int)cl;
 unsigned lo=base+((unsigned)cross<<16);
 struct filter_word p={lo,ch*xh+(cross>>16)+(lo<base)};
 return p;
}
static inline __attribute__((always_inline)) void filter_sub(struct filter_word *a,struct filter_word b)
{
 unsigned previous=a->lo;a->lo-=b.lo;
 a->hi-=b.hi+(previous<b.lo);
}
static inline __attribute__((always_inline)) int filter_sum(int c0,int c1,int c2,int input,int x1,int x2,int y1,int y2)
{
 struct filter_word a=filter_product(c0,input+2*x1+x2);
 filter_sub(&a,filter_product(c1,y1));filter_sub(&a,filter_product(c2,y2));
 return a.hi*16+(int)(a.lo>>28);
}
#ifdef DL_FILTER_TEST
int dl_test_filter(int c0,int c1,int c2,int input,int x1,int x2,int y1,int y2)
{return filter_sum(c0,c1,c2,input,x1,x2,y1,y2);}
#endif
int dl_time(int p) {return DL_TIME[clamp(p,0,32767)>>7];}
int dl_freq(int p) {return DL_FREQ[clamp(p,0,32767)>>8];}
int dl_rate_milli(int p) {return (int)(((long long)DL_RATE[clamp(p,0,32767)>>8]*44100000+(1ll<<31))>>32);}
void dl_init(struct dl_state *s)
{
 unsigned char *p=(unsigned char *)s;
 for(unsigned i=0;i<sizeof *s;++i)p[i]=0;
 s->seed=0x13579bdfu;
}
static int random_value(struct dl_state *s)
{
 s->seed^=s->seed<<13;s->seed^=s->seed>>17;s->seed^=s->seed<<5;
 return (int)(s->seed>>16)-32768;
}
static inline __attribute__((always_inline)) int wave(unsigned phase,int shape,int a,int b)
{
 int q=(int)(phase>>16);
 switch(shape) {
 case 0:return DL_SINE[phase>>22];
 case 1:return q<32768?q*2-32767:98303-q*2;
 case 2:return q<32768?32767:-32767;
 case 3:return q-32768;
 case 4:return 32767-q;
 default:return lerp31(a,b,phase);
 }
}
int dl_wave(unsigned phase,int shape,int a,int b){return wave(phase,shape,a,b);}
static int softclip(int x)
{
 int neg=x<0;if(neg)x=-x;
 x=clamp(x,0,131071);
 unsigned i=(unsigned)x>>9,f=(unsigned)x&511;
 int out=DL_CLIP[i]+((DL_CLIP[i+1]-DL_CLIP[i])*(int)f>>9);
 return neg?-out:out;
}
static int history(const short *ring,unsigned i,unsigned write,unsigned filled)
{
 unsigned age=wrap((int)write-(int)i);
 return age && age<=filled?ring[i]:0;
}
static int read_tap(const short *ring,unsigned write,unsigned lag,unsigned filled)
{
 int position=(int)(write<<8)-(int)lag;
 if(position<0)position+=(int)(DL_SAMPLES<<8);
 unsigned pos=(unsigned)position;
 unsigned i=pos>>8,f=pos&255;
 if(!f)return history(ring,i,write,filled);
 const short *w=DL_CUBIC+f*4;
 if(filled>=(lag>>8)+3u) {
  int acc=ring[wrap((int)i-1)]*w[0]+ring[i]*w[1]+ring[wrap((int)i+1)]*w[2]+ring[wrap((int)i+2)]*w[3];
  return acc>>14;
 }
 int acc=history(ring,wrap((int)i-1),write,filled)*w[0]+history(ring,i,write,filled)*w[1]+history(ring,wrap((int)i+1),write,filled)*w[2]+history(ring,wrap((int)i+2),write,filled)*w[3];
 return acc>>14;
}
/* Settled, unmodulated bypass: direct integer taps, no per-sample control,
 * waveform, interpolation or filter decisions. Preserve all audible and
 * history state exactly, including DC history for later high feedback. */
static void settled(struct dl_state *s,short *ring,short *left,short *right,unsigned lag,int fb,int mix,unsigned inc)
{
 unsigned write=s->write;
 int dc=s->dc_out,dc_in=s->dc_in,last=s->x1,previous=s->x2;
 for(int i=0;i<64;++i) {
  int wet=ring[wrap((int)write-(int)lag)];
  int dry=left[i],dr=right?right[i]:dry;
  int pre=(right?(dry+dr)/4:dry/2)+mul15(wet,fb);
  dc=clamp(pre-dc_in+dc-(dc*94)/32768,-262143,262143);dc_in=pre;
  ring[write]=(short)softclip(fb>32767?dc:pre);
  if(++write==DL_SAMPLES)write=0;
  previous=last;last=pre;
  wet=clamp(wet*2,-32768,32767);
  if(mix==16384) {
   left[i]=(short)((dry+wet)>>1);
   if(right)right[i]=(short)((dr+wet)>>1);
  } else {
   left[i]=(short)(mix==32767?wet:clamp(dry+mul15(wet-dry,mix),-32768,32767));
   if(right)right[i]=(short)(mix==32767?wet:clamp(dr+mul15(wet-dr,mix),-32768,32767));
  }
 }
 unsigned old=s->phase;s->phase+=inc*64u;
 if(s->phase<old){s->random_from=s->random_to;s->random_to=random_value(s);}
 s->write=write;s->filled=s->filled>DL_SAMPLES-64u?DL_SAMPLES:s->filled+64u;
 s->x1=s->y1=last;s->x2=s->y2=previous;s->dc_in=dc_in;s->dc_out=dc;
}
/* Modulated counterpart: controls settled and feedback filter bypassed.
 * Cubic readout, phase and PRNG remain sample-exact. */
static void modulated(struct dl_state *s,short *ring,short *left,short *right,int fb,int mix,int depth,unsigned inc,int shape,unsigned offset)
{
 unsigned write=s->write,phase=s->phase;
 int dc=s->dc_out,dc_in=s->dc_in,last=s->x1,previous=s->x2;
 for(int i=0;i<64;++i) {
  unsigned old=phase;phase+=inc;
  if(phase<old){s->random_from=s->random_to;s->random_to=random_value(s);}
  int lfo=wave(phase+offset,shape,s->random_from,s->random_to);
  int lag=s->time_q8+delay_product(s->time_q8,mul15(lfo,depth));
  lag=clamp(lag,4*256,(DL_SAMPLES-4u)*256);
  int wet=read_tap(ring,write,(unsigned)lag,s->filled);
  int dry=left[i],dr=right?right[i]:dry;
  int pre=(right?(dry+dr)/4:dry/2)+mul15(wet,fb);
  dc=clamp(pre-dc_in+dc-(dc*94)/32768,-262143,262143);dc_in=pre;
  ring[write]=(short)softclip(fb>32767?dc:pre);
  if(++write==DL_SAMPLES)write=0;
  previous=last;last=pre;
  wet=clamp(wet*2,-32768,32767);
  if(mix==16384) {
   left[i]=(short)((dry+wet)>>1);if(right)right[i]=(short)((dr+wet)>>1);
  } else {
   left[i]=(short)(mix==32767?wet:clamp(dry+mul15(wet-dry,mix),-32768,32767));
   if(right)right[i]=(short)(mix==32767?wet:clamp(dr+mul15(wet-dr,mix),-32768,32767));
  }
 }
 s->phase=phase;s->write=write;s->filled=s->filled>DL_SAMPLES-64u?DL_SAMPLES:s->filled+64u;
 s->x1=s->y1=last;s->x2=s->y2=previous;s->dc_in=dc_in;s->dc_out=dc;
}
void dl_render(struct dl_state *s,short *ring,short *left,short *right,int n,const short par[8],int resonance,int freeze)
{
 if(n!=64)return;
 int time=dl_time(par[0])*256;
 int fb=clamp(par[1],0,32767)*2,mix=clamp(par[3],0,32767),depth=clamp(par[5],0,32767);
 unsigned inc=(unsigned)DL_RATE[clamp(par[4],0,32767)>>8];
 int shape=clamp(par[6],0,5),tone=clamp(par[2],0,32767)>>8;
 const int *c=DL_FILTER+(tone*64+clamp(resonance,0,63))*3;
 unsigned phase_offset=((unsigned)(unsigned short)par[7]<<17)-0x80000000u;
 if(!s->ready) {
  s->time_q8=time;s->feedback=fb;s->mix=mix;s->depth=depth;
  s->random_from=random_value(s);s->random_to=random_value(s);s->ready=1;
 }
 if(freeze && !s->freeze_length)s->freeze_length=(unsigned)clamp(s->time_q8>>8,4,DL_SAMPLES-4);
 if(!freeze)s->freeze_length=0;
 if(!freeze && tone==127 && !depth && !s->depth && s->time_q8==time &&
    s->feedback==fb && s->mix==mix && s->filled>=(unsigned)(time>>8)) {
  settled(s,ring,left,right,(unsigned)(time>>8),fb,mix,inc);
  return;
 }
 if(!freeze && tone==127 && depth && s->depth==depth && s->time_q8==time &&
    s->feedback==fb && s->mix==mix && s->filled>=(unsigned)(2*(time>>8)+4)) {
  modulated(s,ring,left,right,fb,mix,depth,inc,shape,phase_offset);
  return;
 }
 int tf=s->feedback,tm=s->mix,td=s->depth;
 int x1=s->x1,x2=s->x2,y1=s->y1,y2=s->y2;
 for(int i=0;i<n;++i) {
  /* 100 ms delay slew, like the original; finite snap avoids integer stalls. */
  int diff=time-s->time_q8;
  if(diff) {
   int step=diff/4410;
   if(!step)step=diff>0?1:-1;
   s->time_q8+=step;
  }
  int feedback=tf+(fb-tf)*(i+1)/64;
  int blend=tm+(mix-tm)*(i+1)/64;
  int modulation=td+(depth-td)*(i+1)/64;
  unsigned old=s->phase;s->phase+=inc;
  if(s->phase<old){s->random_from=s->random_to;s->random_to=random_value(s);}
  int lag=s->time_q8;
  if(modulation) {
   int lfo=wave(s->phase+phase_offset,shape,s->random_from,s->random_to);
   lag+=delay_product(s->time_q8,mul15(lfo,modulation));
  }
  lag=clamp(lag,4*256,(DL_SAMPLES-4u)*256);
  int wet;
  if(freeze)wet=s->filled>=s->freeze_length?ring[wrap((int)s->write-(int)s->freeze_length)]:0;
  else wet=read_tap(ring,s->write,(unsigned)lag,s->filled);
  int dry=left[i],dry_r=right?right[i]:dry;
  if(freeze)ring[s->write]=(short)wet;
  else {
   int input=right?(dry+dry_r)/4:dry/2;
   int pre=input+mul15(wet,feedback);
   int out=pre;
   if(tone<127) {
    out=clamp(filter_sum(c[0],c[1],c[2],pre,x1,x2,y1,y2),-131071,131071);
   }
   x2=x1;x1=pre;y2=y1;y1=out;
   int dc=clamp(out-s->dc_in+s->dc_out-(s->dc_out*94)/32768,-262143,262143);
   s->dc_in=out;s->dc_out=dc;
   ring[s->write]=(short)softclip(feedback>32767?dc:out);
  }
  if(++s->write==DL_SAMPLES)s->write=0;
  if(s->filled<DL_SAMPLES)++s->filled;
  wet=clamp(wet*2,-32768,32767);
  left[i]=(short)(blend==32767?wet:clamp(dry+mul15(wet-dry,blend),-32768,32767));
  if(right)right[i]=(short)(blend==32767?wet:clamp(dry_r+mul15(wet-dry_r,blend),-32768,32767));
 }
 s->x1=x1;s->x2=x2;s->y1=y1;s->y2=y2;
 s->feedback=fb;s->mix=mix;s->depth=depth;
}
