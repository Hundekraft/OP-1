/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ESC_NATIVE_SCREEN_H
#define ESC_NATIVE_SCREEN_H
#include "opus_plugin.h"
int native_screen_start(struct opl_self *self);
void native_screen_init(void *scr);
void native_art_draw(int mix,int frozen);
void native_page_fill_path(int colour,int alpha);
void native_page_path(void);
void native_page_move(int x,int y);
void native_page_line_to(int x,int y);
void native_page_curve_to(int a,int b,int c,int d,int e,int f);
void native_page_close(void);
void native_page_stroke(int width,int colour,int alpha,int cap,int join);
void native_page_stroke_line(int x,int y,int a,int b,int width,int colour);
void native_page_dot(int x,int y,int r,int fill,int colour);
void native_page_fill(int x,int y,int a,int b,int colour);
unsigned short *native_page_pixels(int *rows);
void native_page_text(struct opl_self *self,int slot,int x,int y,int size,int colour,const char *text);
void native_page_text_right(struct opl_self *self,int slot,int x,int y,int size,int colour,const char *text);
#endif
