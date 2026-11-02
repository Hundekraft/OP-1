/* SPDX-License-Identifier: GPL-3.0-or-later
 * Opus adapter, derived from the opus.bitcrush example and ESC_CODE Bubbler.
 */
#include "opus_plugin.h"
#include "native_screen.h"
#include "op1fw.h"
#include "delay_dsp.h"
#include "delay_art.h"
#include "instrument_only.h"
static int clamp(int x,int a,int b){return x<a?a:x>b?b:x;}
static struct opl_self *self;
static volatile int resonance[OPUS_FX_OWNERS],frozen[OPUS_FX_OWNERS];
static int res_edit[OPUS_FX_OWNERS],push_taken[2],knob_acc;
static volatile unsigned frame,view_frame;
static volatile int view_owner=-1;
static short art_par[4];
static char res_text[24],filter_text[24];
static int owner_now(void){return clamp(opus_fx_owner(),0,OPUS_FX_OWNERS-1);}
static int audio_owner(int stereo)
{
 if(stereo)return 16;
 int instrument=clamp(opus_instrument_now(),0,1);
 return instrument*8+clamp(opus_sound_slot(instrument),0,7);
}
static void need(int stereo,struct opus_need *n)
{if(stereo){n->state=0;n->bulk=0;n->fast=0;return;}n->state=sizeof(struct dl_state);n->bulk=sizeof(struct dl_buffer);n->fast=0;}
static void init(struct opus_fx *fx){if(!fx->stereo)dl_init(fx->state);}
static short *buffer(struct opus_fx *fx)
{
 struct dl_buffer *b=fx->bulk;
 /* Synth and drum share bulk in Opus. Invalidate history at every handover,
  * without clearing a megabyte in the audio interrupt. */
 if(b->owner!=fx->state){dl_init(fx->state);b->owner=fx->state;}
 return b->ring;
}
static void mono(struct opus_fx *fx,short *buf,const short *p)
{int o=audio_owner(0);dl_render(fx->state,buffer(fx),buf,0,64,p,resonance[o],frozen[o]);}
static void stereo(struct opus_fx *fx,short *l,short *r,const short *p)
{(void)fx;(void)l;(void)r;(void)p; /* Master is dry, no state or buffers. */}

static int focused(void)
{return view_owner==owner_now() && frame-view_frame<=1u && opus_fx_page_shown() && !opus_fx_list_shown();}
static void settings(const char *name,int value)
{
 if(name && name[0]=='r' && name[1]>='0' && name[1]<='8') {
  int pair=name[1]-'0';
  resonance[pair*2]=value&63;
  if(pair*2+1<OPUS_FX_OWNERS)resonance[pair*2+1]=(value>>6)&63;
 }
}
static void save_res(int owner)
{
 int pair=owner/2;
 char name[3]={'r',(char)('0'+pair),0};
 int value=resonance[pair*2];
 if(pair*2+1<OPUS_FX_OWNERS)value|=resonance[pair*2+1]<<6;
 opus_set_setting(self,name,value);
}
static int key(int code,int pressed)
{
 int push=code==0x3d?0:code==0x3e?1:-1;
 if(push>=0 && !pressed && push_taken[push]){push_taken[push]=0;return 1;}
 if(push>=0 && pressed && focused() && !opus_fn_held() && !fw_shift_held()) {
  int o=owner_now();
  if(push==0){res_edit[o]=!res_edit[o];knob_acc=0;}
  else frozen[o]=!frozen[o];
  push_taken[push]=1;return 1;
 }
 if(pressed && ((code>=1&&code<=9)||code==0x15||code==0x2c||(code>=0x2d&&code<=0x2f)||code==0x10||(code>=0x11&&code<=0x14)))view_owner=-1;
 return 0;
}
static int knob(int index,short *delta)
{
 int o=owner_now();
 if(opus_fn_held() || (index&3)!=2 || !focused() || fw_shift_held() || !res_edit[o])return 0;
 knob_acc+=*delta>>10;
 int detents=knob_acc/4;knob_acc-=detents*4;
 if(detents){resonance[o]=clamp(resonance[o]+detents*2,0,63);save_res(o);}
 return 1;
}
static int shift_knob(short *p,int encoder,int detents)
{
 if(encoder<0||encoder>3||opus_fn_held())return 0;
 int d=clamp(detents,-64,64),k=encoder+4;
 if(encoder==2)p[k]=(short)clamp(p[k]+d,0,5);
 else p[k]=(short)clamp(p[k]+d*512,0,32767);
 return 1;
}
static void begin(void){++frame;dl_art_begin_frame();}
static void under(unsigned short *buf)
{
 (void)buf;if(!focused())return;
 int shift=fw_shift_held();
 dl_art_draw(self,art_par,(unsigned)shift | (frozen[owner_now()]?2u:0u));
 if(!shift) {
  native_page_text_right(self,3,311,29,16,OPUS_COLOR_TEXT,filter_text);
  native_page_text_right(self,1,311,48,10,OPUS_COLOR_E3,res_text);
  native_page_text_right(self,2,311,78,10,OPUS_COLOR_TEXT,frozen[owner_now()]?"freeze":"");
 }
}
static void view(const short *p,struct opus_fx_view *v)
{
 static const char *const labels[2][4]={{"time","feedback","filter freq","mix"},{"lfo freq","lfo depth","lfo shape","lfo phase"}};
 static const char *const shapes[6]={"sine","tri","square","up","down","rand"};
 static char text[4][24];
 int o=owner_now(),shift=fw_shift_held();view_frame=frame;view_owner=o;
 for(int i=0;i<4;++i)art_par[i]=p[i];
 if(!shift) {
  int samples=dl_time(p[0]),hz=dl_freq(p[2]);
  fw_snprintf(text[0],24,"%i ms",(samples*10+220)/441);
  fw_snprintf(text[1],24,"%i%%",clamp(p[1],0,32767)*200/32767);
  if(hz>=1000)fw_snprintf(text[2],24,"%i.%i khz",hz/1000,hz%1000/100);
  else fw_snprintf(text[2],24,"%i hz",hz);
  fw_snprintf(text[3],24,"%i%%",clamp(p[3],0,32767)*100/32767);
  fw_snprintf(filter_text,24,"%s",text[2]);
  fw_snprintf(res_text,24,res_edit[o]?"> reso %i%%":"reso %i%%",resonance[o]*100/63);
 } else {
  int rate=(dl_rate_milli(p[4])+5)/10;
  fw_snprintf(text[0],24,"%i.%02i hz",rate/100,rate%100);
  fw_snprintf(text[1],24,"%i%%",clamp(p[5],0,32767)*100/32767);
  fw_snprintf(text[2],24,"%s",shapes[clamp(p[6],0,5)]);
  fw_snprintf(text[3],24,"%i deg",clamp(p[7],0,32767)*360/32767-180);
 }
 for(int i=0;i<4;++i){v->label[i]=labels[!!shift][i];v->value[i]=!shift && i==2?"":text[i];}
 for(int i=0;i<3;++i)v->wave[i]=0;
 v->waves=0;v->caption=0;
}
static const struct opus_effect DELAY={
 .size=sizeof(struct opus_effect),.name="delayed",
 .defaults={21760,8192,32767,16384,14208,0,0,16384},
 .limits={{0,32767,8192},{0,32767,8192},{0,32767,8192},{0,32767,8192},
 {0,32767,0},{0,32767,0},{0,5,0},{0,32767,0}},
 .need=need,.init=init,.mono=mono,.stereo=stereo,.view=view,.shift_knob=shift_knob
};
int opl_init(struct opl_self *s)
{
 self=s;
 for(int i=0;i<9;++i){char n[3]={'r',(char)('0'+i),0};settings(n,opus_setting(s,n));}
 opus_on_setting(s,settings);
 if(opus_effect_register(s,&DELAY)<0)return -1;
 opus_on_key(s,key);opus_on_knob(s,knob);
 opus_on_frame_begin(s,begin);opus_on_frame_under(s,under);
 return 0;
}
