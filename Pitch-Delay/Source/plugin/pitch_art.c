/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original native-vector adaptation of the approved imagegen concept.
 * Display thread only; no raster loading, files, allocations or audio hooks.
 */
#include "opus_plugin.h"
#include "native_screen.h"
#include "pitch_art.h"
void pd_art_begin_frame(void){}
void pd_art_repair_plot(void){}
static void erase_existing_plot(unsigned short *p,int r){(void)p;(void)r;}
static void capture_clean(unsigned short *p,int r){(void)p;(void)r;}
static int clamp(int x,int a,int b) {return x<a?a:x>b?b:x;}
static void line(int x,int y,int a,int b,int colour,int width)
{native_page_stroke_line(x*256,y*256,a*256,b*256,width,colour);}
static void star(int x,int y,int r,int colour)
{
 x*=256;y*=256;r*=256;int k=r/3;
 native_page_path();native_page_move(x,y-r);
 native_page_curve_to(x+k,y-k,x+k,y-k,x+r,y);native_page_curve_to(x+k,y+k,x+k,y+k,x,y+r);
 native_page_curve_to(x-k,y+k,x-k,y+k,x-r,y);native_page_curve_to(x-k,y-k,x-k,y-k,x,y-r);
 native_page_close();native_page_fill_path(colour,255);
}
void pd_art_draw(const short p[4],int cents)
{
 int rows=0;unsigned short *pixels=native_page_pixels(&rows);erase_existing_plot(pixels,rows);
 int sparkle=clamp((cents+3600)*12/8400,0,12);
 /* Stable lift pose. Only the surrounding accents follow Time/Pitch/Feedback. */
 static const short blue[9][3]={{105,65,4},{134,37,4},{118,71,3},{100,82,3},{239,103,3},{108,119,3},{237,44,3},{126,57,2},{234,126,3}};
 for(int i=0;i<5+clamp(p[0],0,32767)*4/32767;++i)star(blue[i][0],blue[i][1],blue[i][2],OPUS_COLOR_E1);
 static const short white[12][3]={{211,31,4},{233,64,4},{135,72,3},{222,119,3},{130,111,3},{236,84,3},{150,48,3},{228,49,2},{146,34,2},{126,126,2},{221,135,2},{105,99,2}};
 for(int i=0;i<6+sparkle/2;++i)star(white[i][0],white[i][1],white[i][2],OPUS_COLOR_TEXT);
 line(229,52,235,52,OPUS_COLOR_TEXT,352);line(232,49,232,55,OPUS_COLOR_TEXT,352);
 line(122,77,128,77,OPUS_COLOR_TEXT,352);line(125,74,125,80,OPUS_COLOR_TEXT,352);
 /* Thin separated streaks on both sides, outside the character silhouettes. */
 int stripes=2+clamp(p[1],0,32767)*3/32767;
 for(int i=0;i<stripes;++i) {
  int y=88+i*8;
  line(101+(i%2)*7,y,121+(i%3)*2,y,OPUS_COLOR_E2,224);
  line(226+(i%2)*4,y,245-(i%3)*2,y,OPUS_COLOR_E2,224);
 }
 native_art_draw(p[3],0);capture_clean(pixels,rows);
}
