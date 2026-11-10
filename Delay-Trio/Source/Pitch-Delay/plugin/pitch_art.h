/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ESC_PITCH_ART_H
#define ESC_PITCH_ART_H
void pd_art_begin_frame(void);
void pd_art_repair_plot(void);
void pd_art_draw(const short p[4],int cents);
#endif
