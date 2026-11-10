/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ESC_PITCH_PARAMS_H
#define ESC_PITCH_PARAMS_H
unsigned pd_extra_get(const unsigned short words[11],int owner);
void pd_extra_put(unsigned short words[11],int owner,unsigned value);
#endif
