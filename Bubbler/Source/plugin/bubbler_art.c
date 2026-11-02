/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original native-vector adaptation of the approved imagegen concept.
 * Display thread only; no raster loading, files, allocations or audio hooks.
 */
#include "opus_plugin.h"
#include "native_screen.h"
#include "bubbler_art.h"
#include "bubbler_art_data.h"
#define ART_UP 3
void bb_art_begin_frame(void){}
void bb_art_repair_plot(void){}
static void erase_existing_plot(unsigned short *p,int r){(void)p;(void)r;}
static void capture_clean(unsigned short *p,int r){(void)p;(void)r;}
static int clamp(int x,int a,int b) {
    return x<a?a:x>b?b:x;
}
static void ellipse(int x,int y,int rx,int ry,int colour)
{
    int cx=x*256,cy=(y-ART_UP)*256,a=rx*256,b=ry*256,kx=rx*141,ky=ry*141;
    native_page_path();
    native_page_move(cx+a,cy);
    native_page_curve_to(cx+a,cy+ky,cx+kx,cy+b,cx,cy+b);
    native_page_curve_to(cx-kx,cy+b,cx-a,cy+ky,cx-a,cy);
    native_page_curve_to(cx-a,cy-ky,cx-kx,cy-b,cx,cy-b);
    native_page_curve_to(cx+kx,cy-b,cx+a,cy-ky,cx+a,cy);
    native_page_close();
    native_page_stroke(448,colour,255,OPUS_CAP_ROUND,OPUS_JOIN_ROUND);
}
static void wand(int delay)
{
    int r=4+delay/8;
    ellipse(143,80,r,7,OPUS_COLOR_E1);
    native_page_stroke_line(143*256,(87-ART_UP)*256,143*256,(91-ART_UP)*256,448,OPUS_COLOR_E1);
    int length=3+delay/4;
    for(int i=-1;i<=1;++i)
    native_page_stroke_line((137-length)*256,(80+i*4-ART_UP)*256,136*256,(80+i*3-ART_UP)*256,448,OPUS_COLOR_E1);
}
static void bubbles(int count,int grain)
{
    for(int i=0;i<count;++i) {
        int x=BB_BUBBLES[i][0],y=BB_BUBBLES[i][1]-ART_UP,r=BB_BUBBLES[i][2];
        native_page_dot(x,y,r,0,OPUS_COLOR_E2);
        if(r<4) continue;
        int radius=(r-2)*256;
        /* A short white highlight lengthens with the grain-size setting. */
        int cy=y*256,cx=x*256;
        int end_x=cx-radius*(80-grain*45/31)/100;
        int end_y=cy-radius*(40+grain*50/31)/100;
        native_page_path();
        native_page_move(cx-radius*9/10,cy-radius/10);
        native_page_curve_to(cx-radius*9/10,cy-radius*5/10,
        end_x-radius/10,end_y+radius/10,end_x,end_y);
        native_page_stroke(480,OPUS_COLOR_TEXT,255,OPUS_CAP_ROUND,OPUS_JOIN_ROUND);
    }
}
static void sparkles(int grain)
{
    /* Small crosses and curved four-point diamonds echo bubble glints.
         * Fixed accents stay within the picture, away from native readouts. */
    int count=(grain*4+15)/31;
    static const short crosses[2][2]= {
        {
            132,115
        } , {
            211,116
        }
    } ;
    for(int i=0;i<2;++i) {
        if(count<1+i*2)continue;
        int x=crosses[i][0]*256,y=(crosses[i][1]-ART_UP)*256,r=3*256;
        native_page_stroke_line(x-r,y,x+r,y,448,OPUS_COLOR_TEXT);
        native_page_stroke_line(x,y-r,x,y+r,448,OPUS_COLOR_TEXT);
    }
    static const short stars[2][3]= {
        {
            142,44,4
        } , {
            226,45,5
        }
    } ;
    for(int i=0;i<2;++i) {
        if(count<2+i*2)continue;
        native_page_path();
        int x=stars[i][0]*256,y=(stars[i][1]-ART_UP)*256,r=stars[i][2]*256,k=r/3;
        native_page_move(x,y-r);
        native_page_curve_to(x+k,y-k,x+k,y-k,x+r,y);
        native_page_curve_to(x+k,y+k,x+k,y+k,x,y+r);
        native_page_curve_to(x-k,y+k,x-k,y+k,x-r,y);
        native_page_curve_to(x-k,y-k,x-k,y-k,x,y-r);
        native_page_close();
        native_page_stroke(384,OPUS_COLOR_TEXT,255,OPUS_CAP_ROUND,OPUS_JOIN_ROUND);
    }
}
void bb_art_draw(struct opl_self *self,const short par[4],unsigned page)
{
    int mix=clamp(par[3],0,32767);
    int alpha=128+(mix*127+16383)/32767;
    int count=1+clamp(par[1],0,32767)*11/32767;
    int delay=clamp(par[0],0,32767)>>11,grain=clamp(par[2],0,32767)>>10;
    int rows=0;
    unsigned short *pixels=native_page_pixels(&rows);
    /* Native readouts may already be composited when this hook runs.
         * Never clear their regions or capture/restore them as artwork. Drawing
         * vectors directly avoids retaining a previous page's glyph pixels. */
    erase_existing_plot(pixels,rows);
    (void)alpha; /* Boy is a native SVG, recoloured before screen draw. */
    wand(delay);
    bubbles(count,grain);
    sparkles(grain);
    const char *counter=page==0?"1/3":page==1?"2/3":"3/3";
    native_page_text_right(self,0,172,10,12,OPUS_COLOR_DIM,counter);
    capture_clean(pixels,rows);
}
