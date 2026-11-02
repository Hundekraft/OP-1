#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "opus_plugin.h"
#include "op1fw.h"
static int t_list;
static void (*t_after_key)(int,int);static void (*t_after_knob)(int,short);
void opus_on_key_after(struct opl_self *s,void (*f)(int,int)){(void)s;t_after_key=f;}
void opus_on_knob_after(struct opl_self *s,void (*f)(int,short)){(void)s;t_after_knob=f;}
static int t_owner,t_shift,t_fn,t_pos,t_slot[2],t_page=1,t_frames,t_paths,t_text,t_puts;
static const struct opus_hosted_effect *t_host;
static void (*t_loaded)(int,const struct opus_fx_record *),(*t_setting)(const char *,int);
static int (*t_key)(int,int);
static struct opus_fx_record t_records[17];
static short t_native[8];
static void *t_allocated[32];static unsigned t_count,t_bytes;
static int t_values[12];
void *opus_alloc(struct opl_self *s,unsigned n){(void)s;assert(t_count<32);void *p=calloc(1,n);assert(p);t_allocated[t_count++]=p;t_bytes+=n;return p;}
void *opus_fast(struct opl_self *s,unsigned n){return opus_alloc(s,n);}
int opus_fx_manager(struct opl_self *s){(void)s;return 1;}
int opus_fx_records(struct opl_self *s,const char *p,void (*f)(int,const struct opus_fx_record *)){(void)s;assert(p&&strlen(p)<=7);t_loaded=f;return 1;}
int opus_fx_record_put(struct opl_self *s,int owner,const struct opus_fx_record *r){(void)s;assert(owner>=0&&owner<17);t_records[owner]=*r;++t_puts;return 1;}
int opus_effect_host(struct opl_self *s,const struct opus_hosted_effect *h){(void)s;t_host=h;assert(h->screen_draw&&h->svg&&h->mark==0);for(int i=0;i<8;++i)assert(h->defaults[i]>=0);return 0x53a1;}
int opus_effect_live(int pos,short *p){(void)pos;for(int i=0;i<8;++i)p[i]=t_native[i];return 1;}
void opus_on_key(struct opl_self *s,int (*f)(int,int)){(void)s;t_key=f;}
void opus_on_setting(struct opl_self *s,void (*f)(const char *,int)){(void)s;t_setting=f;}
int opus_fx_owner(void){return t_owner;}int opus_instrument_now(void){return t_pos;}int opus_sound_slot(int i){return t_slot[i];}
int opus_fx_page_shown(void){return t_page;}int opus_fx_list_shown(void){return t_list;}int opus_fn_held(void){return t_fn;}int fw_shift_held(void){return t_shift;}
int opus_setting(struct opl_self *s,const char *n){(void)s;int i=n[0]=='e'?(n[1]-'0')*10+n[2]-'0':n[0]=='r'?n[1]-'0':11;return t_values[i];}
int opus_set_setting(struct opl_self *s,const char *n,int v){(void)s;int i=n[0]=='e'?(n[1]-'0')*10+n[2]-'0':n[0]=='r'?n[1]-'0':11;t_values[i]=v;if(t_setting)t_setting(n,v);return 1;}
void opus_log(struct opl_self *s,const char *text){(void)s;fprintf(stderr,"log:%s\n",text);}
int fw_snprintf(char *s,unsigned n,const char *f,...){va_list a;va_start(a,f);int result=vsnprintf(s,n,f,a);va_end(a);return result;}

int opus_flash_headroom(void){return 1000000;}
int fw_yaffs_stat(const char *p,unsigned *s){(void)p;(void)s;return -1;}
int fw_yaffs_mkdir(const char *p,int m){(void)p;(void)m;return 0;}
int fw_yaffs_open(const char *p,int f,int m){(void)f;(void)m;assert(strstr(p,"/screen.svg"));return 1;}
int fw_yaffs_read(int f,void *p,unsigned n){(void)f;(void)p;(void)n;return -1;}
int fw_yaffs_write(int f,const void *p,unsigned n){assert(f==1&&p&&n);return (int)n;}
int fw_yaffs_close(int f){assert(f==1);return 0;}
/* These checks model native object boundaries and draw ordering; they are
 * not an emulation of TE's parser/rasterizer. No page API stubs exist. */
static int t_document,t_raster,t_draw_active,t_constructed,t_sets,t_figures;
static void *t_screen[4]={&t_document,0,0,0};
static unsigned char t_nodes[100][96],t_styles[100][20],t_group[96];
static void *t_text_mem[32];static int t_text_n;
static void t_native_drawing(void){assert(t_draw_active);}
void fw_screen_draw(void *scr){assert(scr==t_screen&&*(void **)scr==&t_document&&!t_group[0x50]);++t_frames;t_draw_active=1;}
void *fw_svg_node_find(void *doc,const char *id,int warn){assert(doc==&t_document&&!warn);if(!strcmp(id,"figures"))return t_group;int i=-1;assert(sscanf(id,"figure%d",&i)==1&&i>=0&&i<100);return t_nodes[i];}
void *fw_screen_fill(void *scr,const char *id){assert(scr==t_screen);if(!strcmp(id,"figures"))return t_group;int i=-1;assert(sscanf(id,"figure%d",&i)==1&&i>=0&&i<100);return t_styles[i];}
void *fw_screen_line_style(void *scr,const char *id){return fw_screen_fill(scr,id);}
void *fw_screen_raster(void){t_native_drawing();return &t_raster;}
void fw_path_init(void *p){assert(p);memset(p,0,52);}
void fw_path_clear(void *p){assert(p);++t_paths;}
void fw_path_move_to(void *p,const int *xy,int absolute){assert(p&&xy&&absolute==1);}
void fw_path_line_to(void *p,const int *xy,int absolute){assert(p&&xy&&absolute==1);}
void fw_path_cubic(void *p,int a,int b,int c,int d,int e,int f,int absolute){(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;assert(p&&absolute==1);}
void fw_path_close(void *p){assert(p);}
void fw_path_finish(void *p,int f,int l){assert(p&&((f==0&&l==1)||(f==1&&l==0)));}
void *fw_line_style_make(void *p,const unsigned char *rgba,int width,int cap,int join,int miter){assert(p&&rgba&&width>0&&cap==1&&join==1&&miter==4096);memcpy(p,rgba,4);return p;}
void fw_stroke_path(void *r,int unused,const void *s,void *p,const int *m,int extra){t_native_drawing();assert(r==&t_raster&&!unused&&s&&p&&m[0]==1024&&m[4]==1024&&m[8]==1024&&!extra);}
void fw_draw_path(void *r,int cache,const void *f,const void *l,void *p,const int *m,int override,int extra){t_native_drawing();assert(r==&t_raster&&!cache&&f&&l&&p&&m&&!override&&!extra);}
void *fw_text_new(void *mem,int x,int y,int size,const unsigned char *rgba,int align){assert(!t_draw_active&&t_text_n<32&&mem&&x>=0&&x<=320*1024&&y>=0&&y+size<=146&&rgba&&(align==0||align==2));memset(mem,0,164);t_text_mem[t_text_n++]=mem;++t_constructed;return mem;}
void fw_set_text(void *mem,const char *s){t_native_drawing();int found=0;for(int i=0;i<t_text_n;++i)if(t_text_mem[i]==mem)found=1;assert(found&&s);++t_sets;}
void fw_text_draw(void *mem,void *r,const int *m){t_native_drawing();assert(mem&&r==&t_raster&&m[0]==1024);++t_text;}
void fw_group_draw(void *node,void *doc,const int *m,void *r){t_native_drawing();assert(node==t_group&&t_group[0x50]&&doc==&t_document&&m[0]==1024&&r==&t_raster);++t_figures;}
void orig_01a85750(void *p,int v){(void)p;(void)v;}
#include "device.c"
static void t_draw(void){t_draw_active=0;int figures=t_figures;t_host->screen_draw(t_screen,16);assert(t_figures==figures+1&&!t_group[0x50]);t_draw_active=0;}
int main(void)
{
 /* Pitch extra-settings defaults are packed words; the other adapters ignore these keys. */
#ifdef TEST_PITCH
 static const unsigned short defaults[]={60539,51121,31518,45548,7879,60539,51121,31518,45548,7879,123};
 for(int i=0;i<11;++i)t_values[i]=(short)defaults[i];
#endif
 struct opl_self s={sizeof s,"esc.test","/yaffs2/mod/esc.test",0};assert(opl_init(&s)==0);assert(t_host&&t_loaded&&t_key);
 for(int i=0;i<8;++i)t_native[i]=t_host->defaults[i];t_native[4]=(short)esc_mark;
 t_host->screen_init(t_screen);t_draw();assert(t_frames==1&&t_paths>0&&t_text>=8);
#ifdef TEST_DELAY
 assert(t_constructed==13);
#else
 assert(t_constructed==11);
#endif
 int sets=t_sets,objects=t_constructed;t_draw();assert(t_sets==sets&&t_constructed==objects);
 short before[8];for(int i=0;i<8;++i)before[i]=esc_saved[0][i];
 assert(t_host->turn(0,t_native,0,1));assert(esc_saved[0][0]>before[0]);assert(t_native[4]==(short)esc_mark);assert(t_puts==1);
 struct opus_fx_record saved=t_records[0];esc_saved[0][4]=0;t_loaded(0,&saved);assert(esc_saved[0][4]==saved.par[4]);
 short decoded[8];esc_parameters(0,t_native,decoded);assert(!memcmp(decoded,esc_saved[0],sizeof decoded));
 /* Every extra word survives the reserved hosted mark. */
 for(int i=4;i<8;++i)assert(decoded[i]==esc_effect->defaults[i]);
 /* Main-parameter LFO deltas do not replace the stored full-resolution base. */
 short base=esc_saved[0][0];t_native[0]+=17;esc_parameters(0,t_native,decoded);assert(decoded[0]>base&&esc_saved[0][0]==base);t_native[0]=esc_encode(0,base);
#ifndef TEST_PITCH
 t_shift=1;t_draw();short extra=esc_saved[0][4];assert(t_host->turn(0,t_native,0,1));assert(esc_saved[0][4]!=extra&&t_records[0].par[4]==esc_saved[0][4]);assert(t_native[4]==(short)esc_mark);t_shift=0;
#endif
 #ifdef TEST_PITCH
 t_draw();short pitch_before=esc_saved[0][2];assert(t_host->turn(0,t_native,2,1));assert(esc_saved[0][2]>pitch_before);
#endif
 /* A white push goes through the same page-scoped handler. */
 t_draw();t_host->key(0,0x3d,1);t_host->key(0,0x3d,0);
 short primary=t_native[2];
#ifdef TEST_DELAY
 int extra_before=resonance[0];assert(t_host->turn(0,t_native,2,1));assert(resonance[0]>extra_before&&t_native[2]==primary);
 t_draw();t_host->key(0,0x3e,1);t_host->key(0,0x3e,0);t_draw();assert(frozen[0]);
 for(unsigned i=0;i<ESC_ART_SHAPES;++i)if(ESC_ART_RED[i])assert(t_styles[i][0]==255&&t_styles[i][1]==255&&t_styles[i][2]==255);
 t_host->key(0,0x3e,1);t_host->key(0,0x3e,0);t_draw();assert(!frozen[0]);
#else
 unsigned extra_before=extras[0];assert(t_host->turn(0,t_native,2,1));assert(extras[0]!=extra_before&&t_native[2]==primary);
 t_draw();short mix_before=t_native[3];extra_before=extras[0];
 assert(t_host->key(0,0x3e,1));assert(t_host->key(0,0x3e,0));assert(t_host->turn(0,t_native,3,1));
 assert(extras[0]!=extra_before&&t_native[3]==mix_before);
#endif
 /* Shared buffer handover cannot leak synth into drum, or another slot. */
 short samples[64]={0};samples[0]=12345;t_host->init(0);t_host->render(0,samples,t_native);
 t_pos=1;t_owner=8;t_slot[1]=0;memset(samples,0,sizeof samples);t_host->init(1);t_host->render(1,samples,t_native);for(int i=0;i<64;++i)assert(samples[i]==0);
 t_pos=0;t_owner=1;t_slot[0]=1;memset(samples,0,sizeof samples);t_host->render(0,samples,t_native);for(int i=0;i<64;++i)assert(samples[i]==0);
 short l[64],r[64],bl[64],br[64];for(int i=0;i<64;++i)l[i]=bl[i]=(short)(i*77),r[i]=br[i]=(short)(-i*19);
 t_host->render_stereo(2,l,r,t_native);assert(!memcmp(l,bl,sizeof l)&&!memcmp(r,br,sizeof r));
 t_owner=16;t_draw();
 /* Missing record resets to defaults; a loaded sound must not inherit the old one. */
 struct opus_fx_record empty={0};t_loaded(0,&empty);assert(!memcmp(esc_saved[0],esc_effect->defaults,sizeof decoded));
 printf("Hosted adapter passed: native screen, controls, complete sound records, mark isolation, shared history and dry master; allocated%u bytes\n",t_bytes);

 /* A new browser choice restores all packed defaults and secondary controls. */
 t_owner=0;t_pos=0;t_slot[0]=0;t_shift=0;t_fn=0;
 for(int i=0;i<8;++i){esc_saved[0][i]=1234;t_native[i]=t_host->defaults[i];}
 t_native[4]=(short)esc_mark;
 resonance[0]=47;frozen[0]=1;res_edit[0]=1;

 t_list=1;t_after_knob(0,1024);t_host->init(0);t_list=0;
 short silence[64]={0};t_host->render(0,silence,t_native);
 assert(!memcmp(esc_saved[0],esc_effect->defaults,sizeof esc_saved[0]));
 assert(esc_reset_dirty[0]);t_after_key(0x24,0);assert(!esc_reset_dirty[0]);
 assert(!memcmp(t_records[0].par,esc_effect->defaults,sizeof esc_saved[0]));
 assert(resonance[0]==0 && !frozen[0] && !res_edit[0]);

 /* Bypass/re-init and a saved preset recall retain their settings. */
 esc_saved[0][0]=20000;t_native[0]=esc_encode(0,20000);
 t_host->init(0);t_host->render(0,silence,t_native);assert(esc_saved[0][0]==20000);
 struct opus_fx_record preset=t_records[0];preset.par[0]=21000;preset.par[5]=16000;
 t_list=1;t_after_knob(0,1024);t_list=0;t_loaded(0,&preset);
 t_native[0]=esc_encode(0,21000);t_host->init(0);t_host->render(0,silence,t_native);
 assert(esc_saved[0][0]==21000 && esc_saved[0][5]==16000);

 /* FN/help neither hides the drawing nor activates a secondary edit. */
 t_draw();int paths_before=t_paths;t_draw();int expected=t_paths-paths_before;
 t_fn=1;paths_before=t_paths;t_draw();assert(t_paths-paths_before==expected);
 assert(!t_key(0,1));assert(!t_key(0,0));
 assert(!t_host->key(0,0x3d,1));assert(!t_host->key(0,0x3e,1));
 assert(!t_host->turn(0,t_native,2,1));t_fn=0;
 puts("Defaults after effect re-selection, preset preservation and help behavior passed");
 for(unsigned i=0;i<t_count;++i)free(t_allocated[i]);
}
