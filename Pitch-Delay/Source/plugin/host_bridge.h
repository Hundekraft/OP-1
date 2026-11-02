/* SPDX-License-Identifier: GPL-3.0-or-later
 * Full native hosted effect screen, following Nostromo's Opus route.
 * This bridge preserves the existing DSP/parameter codec behind a new screen.
 */
#include "opus_plugin.h"
#include "op1fw.h"
#include "native_screen.h"
#include "native_svg.h"
static const struct opus_effect *esc_effect;
static void (*esc_begin)(void),(*esc_under)(unsigned short *);
static int (*esc_old_key)(int,int),(*esc_old_knob)(int,short *);
static struct opl_self *esc_self;
static short esc_saved[17][8];
static struct opus_fx esc_fx[2];
static int esc_last_owner[2]={-1,-1},esc_mark,esc_buffer_owner=-1;
static char esc_screen_path[256];
/* The effect browser chooses native defaults; stored full-resolution extras
 * must also start fresh. Preset loads and FX bypass are separate operations. */
static volatile int esc_choice[17],esc_reset_dirty[17];
static int esc_new_choice[2];
static void esc_reset_secondary(int owner);
static void esc_store_secondary(int owner);
static int esc_capture_effect(struct opl_self *s,const struct opus_effect *e)
{
 (void)s;esc_effect=e;
 for(int o=0;o<17;++o)for(int i=0;i<8;++i)esc_saved[o][i]=e->defaults[i];
 return 8;
}
static void esc_capture_begin(struct opl_self *s,void (*f)(void)){(void)s;esc_begin=f;}
static void esc_capture_under(struct opl_self *s,void (*f)(unsigned short *)){(void)s;esc_under=f;}
static void esc_capture_key(struct opl_self *s,int (*f)(int,int)){(void)s;esc_old_key=f;}
static inline void esc_capture_knob(struct opl_self *s,int (*f)(int,short *)){(void)s;esc_old_knob=f;}
#define opus_effect_register esc_capture_effect
#define opus_on_frame_begin esc_capture_begin
#define opus_on_frame_under esc_capture_under
#define opus_on_key esc_capture_key
#define opus_on_knob esc_capture_knob
#define opl_init esc_legacy_init
#include "legacy_device.inc"
#undef opl_init
#undef opus_effect_register
#undef opus_on_frame_begin
#undef opus_on_frame_under
#undef opus_on_key
#undef opus_on_knob
#include "screen_file.h"
static int esc_clamp(int v,int a,int b){return v<a?a:v>b?b:v;}
/* The hosted route lives in the stock filter slot. Preserve its physical
 * parameter ranges while keeping the original full-resolution sound record. */
static const short ESC_RANGE[4][2]={{64,14528},{0,24576},{0,22528},{8192,32767}};
static int esc_owner(int pos){return pos==2?16:esc_clamp(pos,0,1)*8+esc_clamp(opus_sound_slot(esc_clamp(pos,0,1)),0,7);}
static short esc_encode(int index,int value)
{return (short)(ESC_RANGE[index][0]+esc_clamp(value,0,32767)*(ESC_RANGE[index][1]-ESC_RANGE[index][0])/32767);}
static void esc_parameters(int owner,const short *native,short out[8])
{
 for(int i=0;i<8;++i)out[i]=esc_saved[owner][i];
 if(!native || native[4]!=(short)esc_mark)return;
 for(int i=0;i<4;++i) {
  int delta=((int)native[i]-esc_encode(i,out[i]))*32767/(ESC_RANGE[i][1]-ESC_RANGE[i][0]);
  out[i]=(short)esc_clamp(out[i]+delta,esc_effect->limits[i][0],esc_effect->limits[i][1]);
 }
}
static void esc_record_loaded(int owner,const struct opus_fx_record *r)
{
 if(owner<0 || owner>16 || !r)return;
 esc_choice[owner]=0;
 if(owner<16)esc_new_choice[owner/8]=0;
 int same=1;
 for(int i=0;i<16;++i){char a=r->type[i],b=esc_effect->name[i];if(a!=b){same=0;break;}if(!a)break;}
 if(!same){for(int i=0;i<8;++i)esc_saved[owner][i]=esc_effect->defaults[i];return;}
 for(int i=0;i<8;++i)if(r->par[i]!=(short)-32768)esc_saved[owner][i]=r->par[i];
}
static void esc_save(int owner)
{
 struct opus_fx_record r;
 for(int i=0;i<16;++i)r.type[i]=0;
 for(int i=0;i<15 && esc_effect->name[i];++i)r.type[i]=esc_effect->name[i];
 r.on=1;for(int i=0;i<8;++i)r.par[i]=esc_saved[owner][i];
 if(!opus_fx_record_put(esc_self,owner,&r))opus_log(esc_self,"could not save hosted sound record");
}
static void esc_flush_reset(void)
{
 for(int o=0;o<16;++o)if(esc_reset_dirty[o]) {
  esc_reset_dirty[o]=0;esc_store_secondary(o);esc_save(o);
 }
}
static void esc_after_key(int key,int pressed)
{
 if(pressed && (key==0x24 || key==0x25) && opus_fx_list_shown()) {
  int o=opus_fx_owner();if(o>=0 && o<16)esc_choice[o]=1;
 }
 esc_flush_reset();
}
static void esc_after_knob(int index,short delta)
{
 (void)index;
 if(delta && opus_fx_list_shown()) {
  int o=opus_fx_owner();if(o>=0 && o<16)esc_choice[o]=1;
 }
 esc_flush_reset();
}
static void esc_reset_secondary(int o)
{extras[o]=123;edit[o][0]=edit[o][1]=0;acc[o][0]=acc[o][1]=0;taken[0]=taken[1]=0;}
static void esc_store_secondary(int o){store(o,123);}
static void esc_init(int pos)
{
 if(pos<0 || pos>1)return;
 int o=esc_owner(pos);
 esc_new_choice[pos]=opus_fx_list_shown() || esc_choice[o];esc_choice[o]=0;
 esc_effect->init(&esc_fx[pos]);esc_last_owner[pos]=-1;esc_buffer_owner=-1;
}
static void esc_render(int pos,short *buf,const short *native)
{
 if(pos<0 || pos>1)return;
 int owner=esc_owner(pos);
 if(esc_new_choice[pos] && native && native[4]==(short)esc_mark) {
  esc_new_choice[pos]=0;
  int defaults=1;
  for(int i=0;i<4;++i)if(native[i]!=esc_encode(i,esc_effect->defaults[i]))defaults=0;
  if(defaults) {
   for(int i=0;i<8;++i)esc_saved[owner][i]=esc_effect->defaults[i];
   esc_reset_secondary(owner);esc_reset_dirty[owner]=1;
  }
 }
 if(owner!=esc_last_owner[pos] || owner!=esc_buffer_owner){esc_effect->init(&esc_fx[pos]);esc_last_owner[pos]=owner;esc_buffer_owner=owner;}
 short full[8];esc_parameters(owner,native,full);esc_effect->mono(&esc_fx[pos],buf,full);
}
static void esc_stereo(int pos,short *l,short *r,const short *p){(void)pos;(void)l;(void)r;(void)p;}
static int esc_key(int pos,int code,int pressed){if(pos==2)return 0;return esc_old_key?esc_old_key(code,pressed):0;}
static int esc_global_key(int code,int pressed)
{
 if(code>=0x3b && code<=0x3e)return 0; /* Hosted page delivers its own pushes. */
 return esc_old_key?esc_old_key(code,pressed):0;
}
static int esc_turn(int pos,short *native,int encoder,int detents)
{
 if(pos==2)return 1;
 if(encoder<0 || encoder>3 || opus_fn_held())return 0;
 int owner=esc_owner(pos),d=esc_clamp(detents,-64,64);
 if(esc_old_knob){short delta=(short)esc_clamp(d*4096,-32768,32767);if(esc_old_knob(encoder,&delta))return 1;}
 short p[8];for(int i=0;i<8;++i)p[i]=esc_saved[owner][i];
 if(!(fw_shift_held() && esc_effect->shift_knob && esc_effect->shift_knob(p,encoder,d))) {
  int step=esc_effect->limits[encoder][2]/8;
  if(fw_shift_held())step=step>8?step/8:1;
  p[encoder]=(short)esc_clamp(p[encoder]+d*step,esc_effect->limits[encoder][0],esc_effect->limits[encoder][1]);
 }
 for(int i=0;i<8;++i)esc_saved[owner][i]=p[i];
 for(int i=0;i<4;++i)native[i]=esc_encode(i,p[i]);
 /* Native ReadInterface normalizes and validates all eight words. Extra
  * packed signed words belong only to our full-resolution sound record. */
 native[4]=(short)esc_mark;for(int i=5;i<8;++i)native[i]=0;
 esc_save(owner);return 1;
}
static void esc_screen_init(void *scr){native_screen_init(scr);}
static void esc_screen_draw(void *scr,int dt)
{
 (void)dt;int owner=esc_clamp(opus_fx_owner(),0,16),pos=owner==16?2:owner/8;
 short native[8],full[8];const short *live=0;
 if(pos<2 && opus_effect_live(pos,native))live=native;
 esc_parameters(owner,live,full);
 if(esc_begin)esc_begin();
 struct opus_fx_view v;esc_effect->view(full,&v);
 fw_screen_draw(scr);
 if(esc_under)esc_under(0);
 for(int i=0;i<4;++i) {
  int y=i&1?98:9;
  if(i<2){native_page_text(esc_self,16+i*2,9,y,10,OPUS_COLOR_E(i),v.label[i]);native_page_text(esc_self,17+i*2,9,y+20,24,OPUS_COLOR_TEXT,v.value[i]);}
  else{native_page_text_right(esc_self,16+i*2,311,y,10,OPUS_COLOR_E(i),v.label[i]);native_page_text_right(esc_self,17+i*2,311,y+20,24,OPUS_COLOR_TEXT,v.value[i]);}
 }
 if(owner==16)instrument_only_badge(esc_self);
 /* Return to the OS: native FX OFF and popups remain above this whole screen. */
}
static struct opus_hosted_effect esc_host;
int opl_init(struct opl_self *s)
{
 esc_self=s;
 int result=esc_legacy_init(s);if(result<0)return result;
 if(!esc_effect || !esc_under)return -1;
 struct opus_need n;esc_effect->need(0,&n);
 void *bulk=n.bulk?opus_alloc(s,n.bulk):0,*fast=n.fast?opus_fast(s,n.fast):0;
 if(n.fast && !fast)fast=opus_alloc(s,n.fast);
 if((n.bulk&&!bulk)||(n.fast&&!fast))return -2;
 for(int pos=0;pos<2;++pos){esc_fx[pos].state=n.state?opus_alloc(s,n.state):0;esc_fx[pos].bulk=bulk;esc_fx[pos].fast=fast;esc_fx[pos].stereo=0;if(n.state&&!esc_fx[pos].state)return -2;esc_effect->init(&esc_fx[pos]);}
 if(opus_fx_manager(s)<0)return -4;
 if(!opus_fx_records(s,ESC_RECORD_PREFIX,esc_record_loaded)){opus_log(s,"sound record prefix unavailable");return -3;}
 unsigned len=0;while(s->folder[len] && len<sizeof esc_screen_path-12){esc_screen_path[len]=s->folder[len];++len;}
 if(len>=sizeof esc_screen_path-12)return -1;
 const char *suffix="/screen.svg";while(*suffix)esc_screen_path[len++]=*suffix++;esc_screen_path[len]=0;
 if(!native_screen_start(s))return -2;
 if(!native_screen_file(s,esc_screen_path,ESC_SVG,sizeof ESC_SVG-1))return -1;
 esc_host.size=sizeof esc_host;esc_host.name="pitchdelay";esc_host.mark=0;
 for(int i=0;i<8;++i)esc_host.defaults[i]=i<4?esc_encode(i,esc_effect->defaults[i]):0;
 esc_host.init=esc_init;esc_host.render=esc_render;esc_host.render_stereo=esc_stereo;
 esc_host.svg=esc_screen_path;esc_host.screen_init=esc_screen_init;esc_host.screen_draw=esc_screen_draw;esc_host.key=esc_key;esc_host.turn=esc_turn;
 result=opus_effect_host(s,&esc_host);if(result<0)return result;esc_mark=result;
 opus_on_key(s,esc_global_key);
 opus_on_key_after(s,esc_after_key);opus_on_knob_after(s,esc_after_knob);return 0;
}
