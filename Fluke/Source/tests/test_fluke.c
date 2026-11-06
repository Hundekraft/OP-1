#define FLUKE_HOST
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
/* The freestanding memory helpers have unsigned lengths; avoid libc aliases here. */
#define memcpy plugin_memcpy
#define memset plugin_memset
#include "../esc.fluke/fluke.c"
#undef memcpy
#undef memset
static int ons,offs,held[128],max_held,shift,mode,art_result,clock_tick,first_on[32],first_n;
static unsigned char disk[4][2048] __attribute__((aligned(32)));
static int disk_size[4],disk_pos[4],flash_room=100000,write_fail_after=-1,max_read=256,max_write=2048,close_fail,write_calls,inject_new_lift;
static void (*main_pass_fn)(void);
int opus_flash_headroom(void){return flash_room;}
void opus_on_main_pass(struct opl_self *self,void (*fn)(void)){(void)self;main_pass_fn=fn;assert(fn==snapshot_main);}
int fw_yaffs_mkdir(const char *path,int mode){(void)mode;assert(strstr(path,"esc.fluke"));return 0;}
int fw_yaffs_open(const char *path,int flags,int mode){
 (void)mode;assert(strstr(path,"/esc.fluke/snapshot"));int slot=-1;const char *name=strstr(path,"/snapshot");assert(name);sscanf(name,"/snapshot%i.dat",&slot);assert(slot>=0&&slot<4);disk_pos[slot]=0;
 if(flags){assert(flags==(2|0x40|0x200));disk_size[slot]=0;return slot;}
 return disk_size[slot]?slot:-1;
}
int fw_yaffs_close(int fd){assert(fd>=0&&fd<4);return close_fail?-1:0;}
int fw_yaffs_read(int fd,void *buf,unsigned n){
 assert(!((uintptr_t)buf&31));int remaining=disk_size[fd]-disk_pos[fd];if(remaining<0)remaining=0;
 if(n>(unsigned)remaining)n=(unsigned)remaining;if(n>(unsigned)max_read)n=(unsigned)max_read;
 memcpy(buf,disk[fd]+disk_pos[fd],n);disk_pos[fd]+=(int)n;return (int)n;
}
int fw_yaffs_write(int fd,const void *buf,unsigned n){
 write_calls++;
 if(write_fail_after>=0&&disk_pos[fd]>=write_fail_after)return -1;
 if(n>(unsigned)max_write)n=(unsigned)max_write;
 if(write_fail_after>=0&&disk_pos[fd]+(int)n>write_fail_after)n=(unsigned)(write_fail_after-disk_pos[fd]);
 assert(disk_pos[fd]+(int)n<=2048);memcpy(disk[fd]+disk_pos[fd],buf,n);disk_pos[fd]+=(int)n;disk_size[fd]=disk_pos[fd];
 if(inject_new_lift){inject_new_lift=0;initialise(&pattern,98765);snapshot_save();}
 return (int)n;
}
static const struct opus_sequencer *registered;
static struct {unsigned char shown[84];int matrix[9];} mock_nodes[4][16];
static struct {int x,y,size;unsigned char color[4];char value[64];} mock_text[34];
static unsigned char mock_groups[3][84];
static int text_slot(void *p){return (int)(((unsigned *)p-&texts[0][0])/41);}
static void snapshot(int number){
 void *screen[4]={0};screen_init(screen);screen_draw(screen,0);
 char name[512];const char *dir=getenv("FLUKE_TEST_OUTPUT");snprintf(name,sizeof name,"%s/native-screen-%i.json",dir?dir:".",number);
 FILE *f=fopen(name,"w");assert(f);fprintf(f,"{\"texts\":[");
 fprintf(f," ");
 for(int i=0;i<17;i++){int n=draw_mono&&i<16?i+17:i;fprintf(f,"%s{\"x\":%i,\"y\":%i,\"size\":%i,\"value\":\"%s\",\"color\":\"#%02x%02x%02x\"}",i?",":"",mock_text[n].x,mock_text[n].y,mock_text[n].size,mock_text[n].value,mock_text[n].color[0],mock_text[n].color[1],mock_text[n].color[2]);}
 fprintf(f,"],\"nodes\":[");
 for(int k=0;k<4;k++)for(int i=0;i<16;i++)fprintf(f,"%s[%i,%i,%i,%i]",k||i?",":"",k,i,mock_nodes[k][i].shown[80],mock_nodes[k][i].matrix[7]/1024);
 fprintf(f,"]}\n");fclose(f);
}
void fw_vc_note_on(void *p,int param){(void)param;int *e=p;assert(e[0]>=0&&e[0]<128&&e[1]>0&&e[1]<128);assert(!held[e[0]]);held[e[0]]=1;ons++;int n=0;for(int i=0;i<128;i++)n+=held[i];if(n>max_held)max_held=n;if(first_n<32)first_on[first_n++]=clock_tick;}
void fw_vc_note_off(void *p){int n=*(int *)p;assert(held[n]);held[n]=0;offs++;}
void fw_make_midinote(void *p,int k,int v){int *e=p;e[0]=53+k;e[1]=v;e[2]=1;e[3]=0;}
int fw_shift_held(void){return shift;}
int opus_seq_mode(void){return mode;}
int opus_instrument_now(void){return 0;}
int opus_art_text(struct opl_self *s,const char *n,const char *t,unsigned len){(void)s;(void)n;assert(len==sizeof fluke_svg-1);assert(t==fluke_svg);return art_result;}
void opus_clock(struct opus_clock *c){c->year=2026;c->month=11;c->day=3;c->hour=20;c->minute=0;c->second=0;c->weekday=2;}
int opus_seq_register(struct opl_self *s,const struct opus_sequencer *q){(void)s;registered=q;return 1;}
int fw_snprintf(char *p,unsigned n,const char *f,...){va_list a;va_start(a,f);int r=vsnprintf(p,n,f,a);va_end(a);return r;}
void *fw_svg_node_find(void *d,const char *s,int warn){(void)d;assert(!warn);if(!strcmp(s,"color"))return mock_groups[0];if(!strcmp(s,"mono"))return mock_groups[1];if(!strcmp(s,"popup"))return mock_groups[2];if(s[0]=='m')s++;int k=s[0]=='n'?0:s[0]=='a'?1:s[0]=='p'?2:3;int i=0;while(*s&&(*s<'0'||*s>'9'))s++;sscanf(s,"%i",&i);assert(i>=0&&i<16);return &mock_nodes[k][i];}
void fw_node_set_matrix(void *n,const int *m){for(int k=0;k<4;k++)for(int i=0;i<16;i++)if(n==&mock_nodes[k][i])memcpy(mock_nodes[k][i].matrix,m,9*sizeof(int));}
void fw_group_draw(void *n,void *d,const int *m,void *r){(void)n;(void)d;(void)m;(void)r;}
void fw_screen_draw(void *s){(void)s;for(int k=0;k<34;k++)mock_text[k].value[0]=0;}
void *fw_screen_raster(void){return 0;}
void fw_set_text(void *p,const char *s){int k=text_slot(p);assert(k>=0&&k<34);snprintf(mock_text[k].value,64,"%s",s);}
void *fw_text_new(void *p,int x,int y,int z,const unsigned char *c,int a){(void)c;(void)a;int k=text_slot(p);assert(k>=0&&k<34);mock_text[k].x=x/1024;mock_text[k].y=y;mock_text[k].size=z;memcpy(mock_text[k].color,c,4);assert(y>=2&&y+z<=146);return p;}
void fw_text_draw(void *t,void *r,const int *m){(void)t;(void)r;assert(m[0]==1024);}
static void ticks(int n){for(int i=0;i<n;i++,clock_tick++)tick(0,0);}
static void clear_events(void){stop(&run);assert(ons==offs);ons=offs=first_n=clock_tick=max_held=0;}
int main(void){
 struct opl_self self={sizeof self,"esc.fluke","/yaffs2/mod/esc.fluke",0};
 assert(opl_init(&self)==0);assert(pattern.p[OCTAVE]==1);assert(registered==&seq&&!strcmp(seq.name,"fluke"));
 uint32_t seed=pattern.seed;assert(opl_init(&self)==0&&pattern.seed!=seed);
 for(int p=0;p<PARAMS;p++)pattern.p[p]=hi[p];
 for(int i=0;i<8;i++)turn_acc[i]=2048;
 run.latch=1;run.transpose=12;chosen(0,0);chosen(0,1);
 for(int p=0;p<PARAMS;p++)assert(pattern.p[p]==defaults[p]);
 for(int i=0;i<8;i++)assert(!turn_acc[i]);
 assert(!run.running&&!run.latch&&!run.transpose);
 /* Explicit scales catch incorrect bit masks independently of the quantizer. */
 const int degrees[6][13]={{0,1,2,3,4,5,6,7,8,9,10,11,-1},{0,2,4,5,7,9,11,-1},{0,2,3,5,7,8,10,-1},{0,2,4,7,9,-1},{0,2,3,5,7,9,10,-1},{0,2,4,5,7,9,10,-1}};
 for(int sc=0;sc<6;sc++){
  unsigned mask=0;for(int i=0;degrees[sc][i]>=0;i++)mask|=1u<<degrees[sc][i];assert(mask==scale_masks[sc]);
  for(int sd=1;sd<=300;sd++){
   Pattern a,b;initialise(&a,(uint32_t)sd);a.p[SCALE]=sc;generate(&a);b=a;generate(&b);assert(!memcmp(&a,&b,sizeof a));
   for(int t=0;t<10;t++)for(int i=0;i<16;i++){
    int note=a.pitch[t][i];assert(note>=0&&note<=24);assert(mask&(1u<<(note%12)));
    if(t<4)assert(scale_weights[sc][note%12]>=type_thresholds[t]);
    if(t==4)assert(note%12==0||note%12==7||note%12==((scale_masks[sc]&(1u<<4))?4:3));
    if(t==9&&!(i&1))assert(note==0);
    if(t==2)assert(note%12==0||note%12==7);if(t==3)assert(note%12==0);
   }
   /* Regression: Triad must choose the nearest chord tone, not fall back to root. */
   int third=(mask&(1u<<4))?4:3;
   const int triad[7]={0,third,7,12,12+third,19,24};
   uint32_t random=a.seed;
   for(int i=0;i<16;i++){
    int value=(int)(random_word(&random)>>16)*24,nearest=0;
    while(nearest<6&&value>=(triad[nearest]+triad[nearest+1])*32768)nearest++;
    assert(a.pitch[4][i]==triad[nearest]);
   }
   int scale_notes[25],note_count=0;
   for(int n=0;n<=24;n++)if(mask&(1u<<(n%12)))scale_notes[note_count++]=n;
   for(int i=1;i<16;i++){
    int prev=0,next=0;while(scale_notes[prev]!=a.pitch[5][i-1])prev++;while(scale_notes[next]!=a.pitch[5][i])next++;
    int delta=next-prev;if(delta<0)delta=-delta;assert(delta<=3);
    prev=next=0;while(scale_notes[prev]!=a.pitch[6][i-1])prev++;while(scale_notes[next]!=a.pitch[6][i])next++;
    delta=next-prev;if(delta<0)delta=-delta;assert(delta>=3);
   }
   for(int i=0;i<16;i++){
    if(!((i/4)&1)||i%4!=3)assert(a.pitch[7][i]==a.pitch[0][i%4]);
    if(i<8)assert(a.pitch[8][i]==a.pitch[0][i]);
    if((i&1))assert(a.pitch[9][i]%12!=0);
   }
   assert(a.pitch[8][15]==0);
   for(int d=0;d<16;d++){
    int count=0;for(int i=0;i<16;i++){assert(a.vel[d][i]<=1);count+=a.vel[d][i];if(d&&a.vel[d-1][i])assert(a.vel[d][i]);}assert(count==d+1);
   }
   a.p[DENSITY]=16;a.p[ROOT]=0;int c=get_step(&a,0,0).pitch;a.p[ROOT]=7;assert(get_step(&a,0,0).pitch==c+7);
  }
 }
 /* Quantizer picks nearest enabled bound, with exact midpoint ties upward. */
 assert(marbles_quantize(1<<16,0,0)==1);
 assert(marbles_quantize(1<<16,0,1)==2);
 assert(marbles_quantize((1<<16)-1,0,1)==0);
 assert(marbles_quantize(6<<16,3,1)==12);
 initialise(&pattern,123);pattern.p[DENSITY]=16;pattern.p[SWING]=100;start(&run);ticks(100);
 assert(first_on[0]==0&&first_on[1]==36&&first_on[2]==48&&first_on[3]==84);clear_events();
 for(int sc=0;sc<6;sc++)for(int rate=0;rate<14;rate++){
  initialise(&pattern,123);pattern.p[SCALE]=sc;pattern.p[RATE]=rate;pattern.p[SWING]=100;pattern.p[DENSITY]=16;generate(&pattern);
  start(&run);ticks(5000);assert(max_held<=2);clear_events();
 }
 const int expected[2][4]={{LENGTH,DENSITY,OFFSET,RATE},{OCTAVE,TYPE,DIRECTION,SWING}};
 for(int pg=0;pg<2;pg++)for(int k=0;k<4;k++){
  assert(map[pg][k]==expected[pg][k]);shift=pg;turn(0,k,1000000);if(map[pg][k]==OFFSET)assert(pattern.p[OFFSET]>=0&&pattern.p[OFFSET]<=15);else assert(pattern.p[map[pg][k]]==(map[pg][k]==RATE?lo[RATE]:hi[map[pg][k]]));
  turn(0,k,-1000000);if(map[pg][k]==OFFSET)assert(pattern.p[OFFSET]>=0&&pattern.p[OFFSET]<=15);else assert(pattern.p[map[pg][k]]==(map[pg][k]==RATE?hi[RATE]:lo[map[pg][k]]));
 }
 shift=0;down(&players[0],1,0);assert(pattern.p[ROOT]==6&&pattern.p[REGISTER]==3);ticks(50);up(&players[0],1);assert(!run.running);clear_events();
 initialise(&pattern,123);Pattern before=pattern;key(0,0x3b,0);assert(pattern.seed==before.seed);
 key(0,0x3b,1);assert(pattern.seed!=before.seed);before=pattern;
 for(int i=0;i<6;i++){
  shift=0;key(0,0x3d,1);assert(pattern.seed==before.seed);assert(pattern.p[SCALE]==(before.p[SCALE]+i+1)%6);
  assert(!memcmp(pattern.vel,before.vel,sizeof before.vel));
 }
 shift=0;assert(!memcmp(&pattern,&before,sizeof pattern));
 key(0,0x3e,1);assert(run.latch&&run.running);
 shift=1;seed=pattern.seed;int scale=pattern.p[SCALE];
 for(int c=0x3b;c<=0x3e;c++)key(0,c,1);
 assert(run.latch&&pattern.seed==seed&&pattern.p[SCALE]==scale);
 shift=0;key(0,0x3e,1);assert(!run.latch&&!run.running);clear_events();
 /* Pulse counts and cyclic gaps are balanced for every length, count and rotation. */
 for(int length=1;length<=16;length++)for(int pulses=0;pulses<=length;pulses++)for(int rotate=0;rotate<16;rotate++){
  initialise(&pattern,123);pattern.p[LENGTH]=length;pattern.p[RHYTHM]=1;pattern.p[PULSES]=pulses;pattern.p[OFFSET]=rotate;
  int count=0,positions[16];
  for(int i=0;i<length;i++)if(get_step(&pattern,i,0).velocity)positions[count++]=i;
  assert(count==pulses);
  if(count)for(int i=0;i<count;i++){
   int gap=(i+1<count?positions[i+1]:positions[0]+length)-positions[i];
   assert(gap==length/count||gap==(length+count-1)/count);
  }
 }
 initialise(&pattern,123);pattern.p[LENGTH]=5;key(0,0x3c,1);assert(pattern.p[RHYTHM]&&pattern.p[PULSES]==5);
 turn(0,1,1000000);assert(pattern.p[PULSES]==5);turn(0,1,-1000000);assert(pattern.p[PULSES]==0);
 pattern.p[PULSES]=5;turn(0,0,-1000000);assert(pattern.p[LENGTH]==1&&pattern.p[PULSES]==1);
 key(0,0x3c,1);assert(!pattern.p[RHYTHM]);
 /* Every direction, including odd/even jumps, at every length. */
 for(int len=1;len<=16;len++)for(int dir=0;dir<5;dir++){
  initialise(&pattern,123);pattern.p[LENGTH]=len;pattern.p[DIRECTION]=dir;pattern.p[DENSITY]=16;
  int order[32],n=0;
  if(dir==0)for(int i=0;i<len;i++)order[n++]=i;
  if(dir==1)for(int i=len-1;i>=0;i--)order[n++]=i;
  if(dir==2){for(int i=0;i<len;i++)order[n++]=i;for(int i=len-2;i>0;i--)order[n++]=i;}
  if(dir>=3){for(int i=0;i<len;i+=2)order[n++]=i;for(int i=1;i<len;i+=2)order[n++]=i;if(dir==4)for(int i=0;i<n/2;i++){int t=order[i];order[i]=order[n-1-i];order[n-1-i]=t;}}
  start(&run);for(int i=0;i<n*3;i++){ticks(i?rate_ticks[pattern.p[RATE]]:1);assert(run.current==order[i%n]);}clear_events();
 }
 initialise(&pattern,123);snapshot(0);shift=1;snapshot(1);
 for(int sc=0;sc<6;sc++){
  pattern.p[SCALE]=sc;generate(&pattern);
  for(int t=0;t<10;t++){pattern.p[TYPE]=t;snapshot(2+sc*10+t);assert(!strcmp(mock_text[7].value,type_names[t]));assert(!strcmp(mock_text[1].value,scale_names[sc]));assert(!mock_text[12].value[0]&&!mock_text[13].value[0]&&!mock_text[14].value[0]);}
 }
 shift=0;pattern.p[DIRECTION]=4;key(0,0x3e,1);ticks(1);snapshot(62);assert(!strcmp(mock_text[10].value,"new"));
 assert(mock_text[13].color[0]==102&&mock_text[14].color[0]==255&&mock_text[14].color[1]==66);
 assert(mock_text[10].color[0]==155&&mock_text[10].color[1]==174&&mock_text[11].color[0]==138&&mock_text[11].color[1]==203);
 assert(mock_text[12].color[0]==255&&mock_text[12].color[1]==255&&mock_text[12].color[2]==255);
 assert(mock_text[3].color[0]==0&&mock_text[3].color[1]==237&&mock_text[3].color[2]==149);clear_events();run.latch=0;
 pattern.p[RHYTHM]=1;pattern.p[LENGTH]=13;pattern.p[PULSES]=5;generate(&pattern);snapshot(64);assert(!strcmp(mock_text[3].value,"pulses"));
 mode=1;assert(!down(&players[1],3,0));snapshot(63);mode=0;
 initialise(&pattern,123);pattern.p[ROOT]=1;pattern.p[OCTAVE]=2;pattern.p[RATE]=0;snapshot(65);
 shift=1;pattern.p[TYPE]=9;pattern.p[SCALE]=5;generate(&pattern);snapshot(66);
 pattern.p[OCTAVE]=0;snapshot(67);shift=0;
 pattern.p[RHYTHM]=1;pattern.p[PULSES]=0;snapshot(68);
 /* Rotate wraps in either direction, including 0 backwards to 15. */
 initialise(&pattern,123);turn_acc[2]=0;shift=0;
 turn(0,2,-4096);assert(pattern.p[OFFSET]==15);turn(0,2,4096);assert(pattern.p[OFFSET]==0);
 turn(0,2,15*4096);assert(pattern.p[OFFSET]==15);turn(0,2,4096);assert(pattern.p[OFFSET]==0);
 /* Octave is root-relative range, not a transposition of the keyboard root. */
 for(int sc=0;sc<6;sc++)for(int range=-3;range<=3;range++)for(int type=0;type<10;type++){
  initialise(&pattern,123);pattern.p[SCALE]=sc;pattern.p[OCTAVE]=range;pattern.p[TYPE]=type;pattern.p[ROOT]=7;pattern.p[REGISTER]=2;generate(&pattern);
  for(int i=0;i<16;i++){
   Step step=get_step(&pattern,i,0);int interval=step.pitch-43;
   int lower=range<0?12*range:0,upper=range>0?12*(range+1):12;
   assert(interval>=lower&&interval<upper);assert(scale_masks[sc]&(1u<<((interval%12+12)%12)));
  }
 }
 pattern.p[OCTAVE]=2;down(&players[0],1,0);assert(pattern.p[REGISTER]==3&&pattern.p[ROOT]==6&&pattern.p[OCTAVE]==2);up(&players[0],1);clear_events();
 assert(!strcmp(type_names[8],"reply"));
 /* Rotating a shortened view preserves exactly its visible members. */
 for(int mode=0;mode<2;mode++)for(int length=1;length<=16;length++)for(int amount=-17;amount<=17;amount++){
  initialise(&pattern,123);pattern.p[RHYTHM]=mode;set_length(&pattern,length);
  rotate_visible(&pattern,amount);assert(visible_count(&pattern)==length);
  for(int i=0;i<length;i++){int expected=(i-amount)%length;if(expected<0)expected+=length;assert(visible_note(&pattern,i)==expected);}
  int first=pattern.first_note;set_length(&pattern,16);
  for(int i=0;i<16;i++)assert(visible_note(&pattern,i)==(first+i)%16);
 }
 /* 16 -> 13, rotate right: M A..L; expanding restores M N O P A..L. */
 initialise(&pattern,123);set_length(&pattern,13);rotate_visible(&pattern,1);
 assert(visible_note(&pattern,0)==12&&visible_note(&pattern,1)==0&&visible_note(&pattern,12)==11);
 set_length(&pattern,14);assert(visible_note(&pattern,0)==12&&visible_note(&pattern,1)==13&&visible_note(&pattern,2)==0);
 set_length(&pattern,16);for(int i=0;i<16;i++)assert(visible_note(&pattern,i)==(12+i)%16);assert(pattern.p[OFFSET]==4);
 /* Shrinking an already rotated sequence retains the prefix currently visible. */
 rotate_visible(&pattern,3);int prefix[13];for(int i=0;i<13;i++)prefix[i]=visible_note(&pattern,i);
 set_length(&pattern,13);for(int i=0;i<13;i++)assert(visible_note(&pattern,i)==prefix[i]);
 /* Length-aware encoder wrap. */
 initialise(&pattern,123);set_length(&pattern,13);turn_acc[2]=0;turn(0,2,-4096);assert(pattern.p[OFFSET]==12);
 chosen(0,0);chosen(0,1);assert(pattern.visible_mask==0xffffu&&pattern.first_note==0);
 /* Right speeds up Rate; left slows it. Shift-white controls Direction. */
 initialise(&pattern,123);shift=0;turn_acc[3]=0;
 for(int rate=1;rate<14;rate++){
  pattern.p[RATE]=rate;turn(0,3,4096);assert(rate_ticks[pattern.p[RATE]]<rate_ticks[rate]);
  turn(0,3,-4096);assert(pattern.p[RATE]==rate);
 }
 pattern.p[RATE]=0;turn(0,3,4096);assert(pattern.p[RATE]==0);
 pattern.p[RATE]=13;turn(0,3,-4096);assert(pattern.p[RATE]==13);
 shift=1;turn_acc[6]=0;pattern.p[DIRECTION]=0;turn(0,2,4096);assert(pattern.p[DIRECTION]==1);shift=0;
 art_result=-1;assert(opl_init(&self)==-1);
 puts("PASS Fluke: six scale masks, 1800 seed/scale cases, named types, exact quantization, unchanged melody on scale cycle, new seed on New, reset, keyboard, all rates, note cleanup, five directions/lengths 1-16, native screens/colors; six additional profiles; Euclidean counts and balanced gaps for all lengths/counts/rotations; Main-only pushes");
 return 0;
}
