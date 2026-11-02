/* SPDX-License-Identifier: GPL-3.0-or-later
 * Opus adapter derived from opus.bitcrush and ESC_CODE's prior ports.
 */
#include "opus_plugin.h"
#include "native_screen.h"
#include "op1fw.h"
#include "pitch_dsp.h"
#include "pitch_params.h"
#include "pitch_art.h"
#include "instrument_only.h"
static int clamp(int x,int a,int b){return x<a?a:x>b?b:x;}
static struct opl_self *self;
static unsigned short extra_words[11];
static volatile unsigned extras[OPUS_FX_OWNERS];
static int edit[OPUS_FX_OWNERS][2],acc[OPUS_FX_OWNERS][2],taken[2];
static unsigned frame,view_frame;
static int view_owner=-1;
static short art[4];static int art_cents;
static char pitch_text[24],mix_text[24],oct_text[24],loop_text[24];
static int owner_now(void){return clamp(opus_fx_owner(),0,16);}
static int audio_owner(int stereo){if(stereo)return 16;int i=clamp(opus_instrument_now(),0,1);return i*8+clamp(opus_sound_slot(i),0,7);}
static int octave(unsigned v){return clamp((int)(v&7),0,6)-3;}
static int depth(unsigned v){return 10+clamp((int)(v>>3),0,90);}
static void need(int stereo,struct opus_need *n){if(stereo){n->state=0;n->bulk=0;n->fast=0;return;}n->state=sizeof(struct pd_state);n->bulk=sizeof(struct pd_buffer);n->fast=0;}
static void init(struct opus_fx *fx){if(!fx->stereo)pd_init(fx->state);}
static short *buffer(struct opus_fx *fx)
{
 struct pd_buffer *b=fx->bulk;
 if(b->owner!=fx->state){pd_init(fx->state);b->owner=fx->state;}
 return b->ring;
}
static void mono(struct opus_fx *fx,short *buf,const short *p){unsigned v=extras[audio_owner(0)];pd_render(fx->state,buffer(fx),buf,0,p,octave(v),depth(v));}
static void stereo(struct opus_fx *fx,short *l,short *r,const short *p)
{(void)fx;(void)l;(void)r;(void)p; /* Master is dry, no state or buffers. */}

static int focused(void){return view_owner==owner_now() && frame-view_frame<=1u && opus_fx_page_shown() && !opus_fx_list_shown() && !opus_fn_held();}
static void setting_changed(const char *name,int value)
{
 if(!name || name[0]!='e' || name[1]<'0' || name[1]>'1' || name[2]<'0'||name[2]>'9')return;
 int i=(name[1]-'0')*10+name[2]-'0';if(i>10)return;
 extra_words[i]=(unsigned short)value;
 for(int o=0;o<17;++o)extras[o]=pd_extra_get(extra_words,o);
}
static void store(int o,unsigned value)
{
 pd_extra_put(extra_words,o,value);extras[o]=value;
 unsigned first=(unsigned)o*10/16,last=((unsigned)o*10+9)/16;
 for(unsigned i=first;i<=last;++i){char name[4]={'e',(char)('0'+i/10),(char)('0'+i%10),0};opus_set_setting(self,name,(short)extra_words[i]);}
}
static int key(int code,int pressed)
{
 int which=code==0x3d?0:code==0x3e?1:-1;
 if(which>=0 && !pressed && taken[which]){taken[which]=0;return 1;}
 if(which>=0 && pressed && focused() && !fw_shift_held()) {
  int o=owner_now();edit[o][which]=!edit[o][which];acc[o][which]=0;taken[which]=1;return 1;
 }
 if(pressed && ((code>=1&&code<=9)||code==0x15||code==0x2c||(code>=0x2d&&code<=0x2f)||code==0x10||(code>=0x11&&code<=0x14)))view_owner=-1;
 return 0;
}
static int knob(int index,short *delta)
{
 int which=(index&3)-2,o=owner_now();
 if(which<0 || !focused() || !edit[o][which])return 0;
 acc[o][which]+=*delta>>10;int d=acc[o][which]/4;acc[o][which]-=d*4;
 if(d) {
  unsigned v=extras[o];int oct=octave(v),dep=depth(v);
  if(which==0)oct=clamp(oct+d,-3,3);else dep=clamp(dep+d,10,100);
  store(o,(unsigned)(oct+3)|((unsigned)(dep-10)<<3));
 }
 return 1;
}
static void begin(void){++frame;pd_art_begin_frame();}
static void under(unsigned short *buf)
{
 (void)buf;if(!focused())return;
 pd_art_draw(art,art_cents);
 native_page_text_right(self,3,311,29,16,OPUS_COLOR_TEXT,pitch_text);
 native_page_text_right(self,4,311,48,10,OPUS_COLOR_E3,oct_text);
 native_page_text_right(self,5,311,98,10,OPUS_COLOR_E4,"mix");
 native_page_text_right(self,2,311,118,16,OPUS_COLOR_TEXT,mix_text);
 native_page_text_right(self,1,311,136,8,OPUS_COLOR_E4,loop_text);
}
static void view(const short *p,struct opus_fx_view *v)
{
 static const char *const labels[4]={"time","feedback","pitchshift","mix"};
 static char time_text[24],feedback[24];int o=owner_now();view_owner=o;view_frame=frame;
 unsigned ex=extras[o];int oct=octave(ex),dep=depth(ex);
 for(int i=0;i<4;++i)art[i]=p[i];
 art_cents=pd_cents(pd_pitch_index(p[2]))+oct*1200;
 fw_snprintf(time_text,24,"%i ms",(pd_time(p[0])*10+220)/441);
 fw_snprintf(feedback,24,"%i%%",clamp(p[1],0,32767)*200/32767);
 fw_snprintf(pitch_text,24,"%s",pd_pitch_label(pd_pitch_index(p[2])));
 fw_snprintf(mix_text,24,"%i%%",clamp(p[3],0,32767)*100/32767);
 fw_snprintf(oct_text,24,edit[o][0]?"> oct %+i":"oct %+i",oct);
 fw_snprintf(loop_text,24,edit[o][1]?"> loop %i%%":"loop %i%%",dep);
 for(int i=0;i<4;++i){v->label[i]=i==3?"":labels[i];v->value[i]=i==0?time_text:i==1?feedback:"";}
 for(int i=0;i<3;++i)v->wave[i]=0;
 v->waves=0;v->caption=0;
}
static const struct opus_effect PITCH={
 .size=sizeof(struct opus_effect),.name="pitchdelay",.defaults={27008,8192,0,16384,0,0,0,0},
 .limits={{0,32767,8192},{0,32767,8192},{0,PD_PITCH_MAX,PD_PITCH_STEP*8},{0,32767,8192},{0,0,0},{0,0,0},{0,0,0},{0,0,0}},
 .need=need,.init=init,.mono=mono,.stereo=stereo,.view=view
};
int opl_init(struct opl_self *s)
{
 self=s;
 for(int i=0;i<11;++i){char name[4]={'e',(char)('0'+i/10),(char)('0'+i%10),0};extra_words[i]=(unsigned short)opus_setting(s,name);}
 for(int o=0;o<17;++o)extras[o]=pd_extra_get(extra_words,o);
 opus_on_setting(s,setting_changed);
 int result=opus_effect_register(s,&PITCH);
 if(result<0) {
  opus_log(s,result==-1?"effect registration: type limit or invalid descriptor":
   result==-2?"effect registration: insufficient instance memory":
   result==-3?"effect registration: effect list full":
   "effect registration: effect service unavailable");
  return result;
 }
 opus_on_key(s,key);opus_on_knob(s,knob);opus_on_frame_begin(s,begin);opus_on_frame_under(s,under);
 return 0;
}
