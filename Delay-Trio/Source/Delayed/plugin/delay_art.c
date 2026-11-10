/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original native-vector adaptation of the approved imagegen concept.
 * Display thread only; no raster loading, files, allocations or audio hooks.
 */
#include "opus_plugin.h"
#include "native_screen.h"
#include "delay_art.h"
void dl_art_begin_frame(void){}
void dl_art_repair_plot(void){}
static void erase_existing_plot(unsigned short *p,int r){(void)p;(void)r;}
static void capture_clean(unsigned short *p,int r){(void)p;(void)r;}
static int clamp(int x,int a,int b) {return x<a?a:x>b?b:x;}
static int ice;
static void line(int x,int y,int a,int b,int width,int colour)
{native_page_stroke_line(x*256,y*256,a*256,b*256,width,ice?OPUS_COLOR_TEXT:colour);}
static void clock_face(int time)
{
 int x=134,y=79,r=12+time*15/32767;
 int cx=x*256,cy=y*256,a=r*256,k=r*141;
 native_page_dot(x,y,r,1,OPUS_COLOR_BACK);
 native_page_path();native_page_move(cx+a,cy);
 native_page_curve_to(cx+a,cy+k,cx+k,cy+a,cx,cy+a);
 native_page_curve_to(cx-k,cy+a,cx-a,cy+k,cx-a,cy);
 native_page_curve_to(cx-a,cy-k,cx-k,cy-a,cx,cy-a);
 native_page_curve_to(cx+k,cy-a,cx+a,cy-k,cx+a,cy);native_page_close();
 native_page_stroke(576,ice?OPUS_COLOR_TEXT:OPUS_COLOR_E1,255,OPUS_CAP_ROUND,OPUS_JOIN_ROUND);
 /* 8:20: minute hand down-right; short hour hand down-left. */
 line(x,y,x+r*78/100,y+r*45/100,576,OPUS_COLOR_E1);
 line(x,y,x-r*52/100,y+r*19/100,576,OPUS_COLOR_E1);
 line(x,y-r+2,x,y-r+4,448,OPUS_COLOR_E1);
 line(x-r+2,y,x-r+4,y,448,OPUS_COLOR_E1);
 line(x,y+r-2,x,y+r-4,448,OPUS_COLOR_E1);
}
static void sparkles(int filter)
{
 static const short stars[10][3]={{99,60,3},{169,36,3},{237,87,4},{242,69,4},{110,111,3},{144,117,2},{108,72,2},{217,53,2},{214,73,3},{98,126,2}};
 int count=2+clamp(filter,0,32767)*8/32767;
 for(int i=0;i<count;++i) {
  int x=stars[i][0]*256,y=stars[i][1]*256,r=stars[i][2]*256,k=r/3;
  native_page_path();native_page_move(x,y-r);
  native_page_curve_to(x+k,y-k,x+k,y-k,x+r,y);
  native_page_curve_to(x+k,y+k,x+k,y+k,x,y+r);
  native_page_curve_to(x-k,y+k,x-k,y+k,x-r,y);
  native_page_curve_to(x-k,y-k,x-k,y-k,x,y-r);native_page_close();
  native_page_fill_path(OPUS_COLOR_TEXT,255);
 }
 if(count>2){line(93,85,99,85,448,OPUS_COLOR_TEXT);line(96,82,96,88,448,OPUS_COLOR_TEXT);}
 if(count>4){line(232,77,238,77,448,OPUS_COLOR_TEXT);line(235,74,235,80,448,OPUS_COLOR_TEXT);}
}
void dl_art_draw(struct opl_self *self,const short par[4],unsigned page)
{
 ice=(page&2u)!=0;
 int rows=0;unsigned short *pixels=native_page_pixels(&rows);
 erase_existing_plot(pixels,rows);
 clock_face(clamp(par[0],0,32767));
 sparkles(par[2]);
 int count=2+clamp(par[1],0,32767)*7/32767;
 for(int i=0;i<count;++i) {
  int x=98+(i*17%23),y=84+i*4;
  int end=clamp(x+24+(i%3)*5,x,y>=116?132:146);
  int radius=15+clamp(par[0],0,32767)*15/32767,dy=y-79;
  if(dy<=radius) {
   int dx=0;
   while((dx+1)*(dx+1)+dy*dy<=radius*radius)++dx;
   int edge=134-dx-1;
   if(end>edge)end=edge;
  }
  if(end>x)line(x,y,end,y,448,OPUS_COLOR_E2);
 }
 native_art_draw(par[3],ice);
 if(ice) {
  /* Frost crystals and icicles: all-white scene, clearly distinct from Mix. */
  static const short snow[6][2]={{102,66},{222,83},{112,119},{213,64},{210,86},{105,92}};
  for(int i=0;i<6;++i){int x=snow[i][0],y=snow[i][1];
   line(x-4,y,x+4,y,320,OPUS_COLOR_TEXT);line(x,y-4,x,y+4,320,OPUS_COLOR_TEXT);
   line(x-3,y-3,x+3,y+3,256,OPUS_COLOR_TEXT);line(x-3,y+3,x+3,y-3,256,OPUS_COLOR_TEXT);
  }
  /* Frost on the hair and a fringe of long ice beneath the clock. */
  line(170,54,175,52,384,OPUS_COLOR_TEXT);line(175,52,181,54,384,OPUS_COLOR_TEXT);
  line(178,54,179,60,320,OPUS_COLOR_TEXT);line(179,60,182,54,320,OPUS_COLOR_TEXT);
  line(189,57,191,63,320,OPUS_COLOR_TEXT);line(191,63,193,59,320,OPUS_COLOR_TEXT);
  line(146,99,148,107,320,OPUS_COLOR_TEXT);line(148,107,151,98,320,OPUS_COLOR_TEXT);
  line(154,108,157,115,320,OPUS_COLOR_TEXT);line(157,115,159,109,320,OPUS_COLOR_TEXT);
  line(122,100,125,109,320,OPUS_COLOR_TEXT);line(125,109,128,101,320,OPUS_COLOR_TEXT);
  line(134,101,137,113,320,OPUS_COLOR_TEXT);line(137,113,140,101,320,OPUS_COLOR_TEXT);
 }

 native_page_text_right(self,0,172,10,12,OPUS_COLOR_DIM,(page&1u)?"2/2":"1/2");
 capture_clean(pixels,rows);
}

