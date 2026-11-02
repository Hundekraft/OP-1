/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "pitch_params.h"
unsigned pd_extra_get(const unsigned short w[11],int owner)
{
 if(owner<0 || owner>16)return 123;
 unsigned b=(unsigned)owner*10,i=b>>4,s=b&15;
 unsigned v=w[i]>>s;
 if(s>6)v|=(unsigned)w[i+1]<<(16-s);
 return v&1023u;
}
void pd_extra_put(unsigned short w[11],int owner,unsigned value)
{
 if(owner<0 || owner>16)return;
 unsigned b=(unsigned)owner*10,i=b>>4,s=b&15;
 unsigned mask=1023u<<s;value&=1023u;
 w[i]=(unsigned short)(((unsigned)w[i]&~mask)|(value<<s));
 if(s>6){unsigned bits=s-6,low=(1u<<bits)-1u;w[i+1]=(unsigned short)(((unsigned)w[i+1]&~low)|(value>>(16-s)));}
}
