/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef DELAY_ART_H
#define DELAY_ART_H
struct opl_self;
void dl_art_begin_frame(void);
void dl_art_repair_plot(void);
void dl_art_draw(struct opl_self *self,const short par[4],unsigned page);
#endif
