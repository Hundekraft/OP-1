/* SPDX-License-Identifier: GPL-3.0-or-later
 * Native hosted-screen drawing only. No Opus page context, pixel capture,
 * frame hooks or private frame-buffer access. Firmware #246 SDK contracts.
 */
#include "native_screen.h"
#include "op1fw.h"
#include "native_svg.h"
static const int identity[9]={1024,0,0,0,1024,0,0,0,1024};
static unsigned path_mem[13];
static void *boy_colour[ESC_BOY_SHAPES];
struct text_slot { unsigned mem[41]; unsigned char rgba[4]; char shown[48]; int active; };
static struct text_slot *texts;
/* Four small readouts and eight main label/value objects. Public slot ids
 * stay unchanged, but unused holes no longer reserve native text storage. */
#define TEXT_COUNT 12
static int text_index(int slot)
{
 if(slot>=0 && slot<=2)return slot;
 if(slot==7)return 3;
 if(slot>=16 && slot<=23)return slot-12;
 return -1;
}
static const unsigned char palette[7][4]={
 {77,158,255,255},{0,237,149,255},{255,255,255,255},
 {255,58,93,255},{255,255,255,255},{111,113,115,255},{0,0,0,255}
};
static void colour(unsigned char out[4],int c,int alpha)
{
 int index=c==OPUS_COLOR_E1?0:c==OPUS_COLOR_E2?1:c==OPUS_COLOR_E3?2:
 c==OPUS_COLOR_E4?3:c==OPUS_COLOR_TEXT?4:c==OPUS_COLOR_DIM?5:6;
 for(int i=0;i<3;++i)out[i]=(unsigned char)(palette[index][i]*alpha/255);
 out[3]=255;
}
static void create_text(int slot,int x,int y,int size,int c,int align)
{
 int index=text_index(slot);if(index<0)return;
 struct text_slot *t=&texts[index];colour(t->rgba,c,255);
 /* Alignment 2 is the native right-alignment value: verified against the
  * owner's firmware #246 Opus text-right wrapper (private audit, not bundled). */
 fw_text_new(t->mem,x*1024,y,size,t->rgba,align);
 t->active=1;t->shown[0]=0;
}
int native_screen_start(struct opl_self *self)
{
 texts=opus_alloc(self,TEXT_COUNT*sizeof *texts);if(!texts)return 0;
 fw_path_init(path_mem);
 /* Reserve more segments than any live decorative path needs, at startup.
  * fw_path_clear retains segment capacity for subsequent frames. */
 int xy[2]={0,0};fw_path_move_to(path_mem,xy,1);
 for(int i=0;i<20;++i){xy[0]=(i+1)*1024;xy[1]=(i&1)*1024;fw_path_line_to(path_mem,xy,1);}
 fw_path_clear(path_mem);
 return 1;
}
void native_screen_init(void *scr)
{
 if(!texts || !scr)return;
 char id[20];
 for(unsigned i=0;i<ESC_BOY_SHAPES;++i){
  fw_snprintf(id,sizeof id,"boy%i",(int)i);
  /* Nonfatal lookup first: style lookup itself asserts on an absent id. */
  if(fw_svg_node_find(*(void **)scr,id,0))
   boy_colour[i]=ESC_BOY_FILL[i]?fw_screen_fill(scr,id):fw_screen_line_style(scr,id);
 }
 if(texts[0].active)return;
 create_text(0,172,10,12,OPUS_COLOR_DIM,2);
 create_text(1,311,134,8,OPUS_COLOR_E4,2);
 create_text(2,311,118,14,OPUS_COLOR_TEXT,2);
 create_text(7,249,85,12,OPUS_COLOR_TEXT,2);
 for(int i=0;i<4;++i){int y=(i&1)?98:9;
  create_text(16+i*2,i<2?9:311,y,10,OPUS_COLOR_E(i),i<2?0:2);
  create_text(17+i*2,i<2?9:311,y+20,24,OPUS_COLOR_TEXT,i<2?0:2);
 }
}
void native_screen_prepare(int mix)
{
 if(mix<0)mix=0;
 if(mix>32767)mix=32767;
 int intensity=128+(mix*127+16383)/32767;
 for(unsigned i=0;i<ESC_BOY_SHAPES;++i)if(boy_colour[i])colour(boy_colour[i],OPUS_COLOR_E4,intensity);
}
void native_page_path(void){fw_path_clear(path_mem);}
/* The old artwork's x256 coordinates are converted explicitly to native Q10. */
void native_page_move(int x,int y){int xy[2]={x*4,y*4};fw_path_move_to(path_mem,xy,1);}
void native_page_line_to(int x,int y){int xy[2]={x*4,y*4};fw_path_line_to(path_mem,xy,1);}
void native_page_curve_to(int a,int b,int c,int d,int e,int f){fw_path_cubic(path_mem,a*4,b*4,c*4,d*4,e*4,f*4,1);}
void native_page_close(void){fw_path_close(path_mem);}
void native_page_stroke(int width,int c,int alpha,int cap,int join)
{
 unsigned style[5];unsigned char rgba[4];colour(rgba,c,alpha);
 fw_path_finish(path_mem,0,1);
 fw_line_style_make(style,rgba,width*4,cap,join,4096);
 fw_stroke_path(fw_screen_raster(),0,style,path_mem,identity,0);
}
void native_page_stroke_line(int x,int y,int a,int b,int width,int c)
{
 native_page_path();native_page_move(x,y);native_page_line_to(a,b);
 native_page_stroke(width,c,255,OPUS_CAP_ROUND,OPUS_JOIN_ROUND);
}
void native_page_dot(int x,int y,int r,int fill,int c)
{
 (void)fill;int cx=x*256,cy=y*256,a=r*256,k=r*141;
 native_page_path();native_page_move(cx+a,cy);
 native_page_curve_to(cx+a,cy+k,cx+k,cy+a,cx,cy+a);
 native_page_curve_to(cx-k,cy+a,cx-a,cy+k,cx-a,cy);
 native_page_curve_to(cx-a,cy-k,cx-k,cy-a,cx,cy-a);
 native_page_curve_to(cx+k,cy-a,cx+a,cy-k,cx+a,cy);
 native_page_close();native_page_stroke(256,c,255,OPUS_CAP_ROUND,OPUS_JOIN_ROUND);
}
void native_page_fill(int x,int y,int a,int b,int c)
{
 unsigned char rgba[4];colour(rgba,c,255);
 /* Native vector-table ABI from fw_draw_path's SDK declaration. */
 struct vector { unsigned unused;void *first,*last,*end; } fills={0,rgba,rgba+4,rgba+4},lines={0,0,0,0};
 native_page_path();native_page_move(x*256,y*256);native_page_line_to(a*256,y*256);
 native_page_line_to(a*256,b*256);native_page_line_to(x*256,b*256);native_page_close();
 fw_path_finish(path_mem,1,0);fw_draw_path(fw_screen_raster(),0,&fills,&lines,path_mem,identity,0,0);
}
unsigned short *native_page_pixels(int *rows){*rows=0;return 0;}
void native_page_text(struct opl_self *self,int slot,int x,int y,int size,int c,const char *s)
{
 (void)self;(void)x;(void)y;(void)size;(void)c;
 int index=text_index(slot);
 if(!texts || index<0 || !texts[index].active || !s)return;
 struct text_slot *t=&texts[index];int same=1,n=0;
 while(s[n] && n<47){if(t->shown[n]!=s[n])same=0;++n;}
 if(s[n])return;
 if(t->shown[n])same=0;
 if(!same){for(int i=0;i<=n;++i)t->shown[i]=s[i];fw_set_text(t->mem,t->shown);}
 if(n)fw_text_draw(t->mem,fw_screen_raster(),identity);
}
void native_page_text_right(struct opl_self *self,int slot,int x,int y,int size,int c,const char *s)
{native_page_text(self,slot,x,y,size,c,s);}
