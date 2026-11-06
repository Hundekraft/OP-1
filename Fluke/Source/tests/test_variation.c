#include <assert.h>
#include <stdio.h>
#include "../esc.fluke/core.h"
int main(void){
 unsigned total=0,unchanged=0;
 for(int seed=1;seed<=300;seed++)for(int scale=0;scale<6;scale++)for(int range=-3;range<=3;range++){
  Pattern a,b;initialise(&a,(unsigned)seed);a.p[SCALE]=scale;a.p[OCTAVE]=range;generate(&a);b=a;generate(&b);
  for(int type=0;type<10;type++)for(int i=0;i<16;i++){
   int note=a.phrase[type][i],low=range<0?range*12:0,high=low+12*((range<0?-range:range)+1)-1;
   assert(note>=low&&note<=high&&phrase_allowed(note,type,scale));assert(note==b.phrase[type][i]);
   if(type!=3){total++;unchanged+=note==raw_interval(a.pitch[type][i],range);}
   if(type>=7)assert(note==raw_interval(a.pitch[type][i],range));
  }
  if(range){
   int first=-1,second=-1;
   for(int i=0;i<16;i++)if(note_on_grid(&a,i)){if(first<0)first=i;else {second=i;break;}}
   if(second>=0){int p=a.phrase[3][visible_note(&a,first)],q=a.phrase[3][visible_note(&a,second)];assert(p!=q&&positive_pc(p)==0&&positive_pc(q)==0);}
  }else for(int i=0;i<16;i++)assert(a.phrase[3][i]==0);
 }
 assert(unchanged*100>=total*75);
 printf("PASS: 12600 seed/scale/range cases, note vocabulary/range/determinism; %u/%u original proposals retained; Root octave variety; motif/reply/anchor preserved\n",unchanged,total);
 return 0;
}
