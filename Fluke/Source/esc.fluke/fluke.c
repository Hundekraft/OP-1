/* Private native Opus 2.8 adaptation. Original OP-1 firmware #246 only. */
#include "opus_plugin.h"
#include "op1fw.h"
#include "core.h"
#include "art.h"
static Pattern banks[2];
static Pattern *current_pattern=&banks[0],*previous_pattern=&banks[1];
#define pattern (*current_pattern)
static int turn_acc[8];
static Player run;
static char art_path[256];
static unsigned texts[34][41];
static int draw_mono;
static unsigned char mono_colors[16][4];
static void *color_art,*mono_art,*popup;
static void snapshot_tick(void);
static const int identity[9]={1024,0,0,0,1024,0,0,0,1024};
static void *note_nodes[16],*active_nodes[16],*rest_nodes[16];
static void *mono_notes[16],*mono_active[16],*mono_rests[16];
static unsigned char rgba[5][4]={{105,142,255,255},{0,237,149,255},{205,205,255,255},{255,66,63,255},{150,150,150,255}};
static const unsigned char hold_dim[4]={102,26,25,255};
static const unsigned char page_color[4]={111,113,115,255};
static const unsigned char root_color[4]={255,240,207,255};
static const unsigned char soft_blue[4]={155,174,224,255};
static const unsigned char soft_green[4]={138,203,177,255};
static const unsigned char caption_white[4]={255,255,255,255};
static void note_on(int pitch,int vel){int ev[4]={pitch,vel,1,0};fw_vc_note_on(ev,0);}
static void note_off(int pitch){int ev[4]={pitch,0,1,0};fw_vc_note_off(ev);}
static int mode_ok(int mode){return mode==0;}
static int active_mode(void){int m=opus_seq_mode();return m<0?opus_instrument_now():m;}
static int noop(void *p){(void)p;return 0;}
static void halt(void *p){(void)p;stop(&run);}
static int down(void *p,int key,int shift){
 (void)p;(void)shift;if(active_mode()!=0||key<0||key>23)return 0;
 int ev[4];fw_make_midinote(ev,key,100);
 int root=bounded(ev[0],0,127)%12;
 int octave=bounded(ev[0]/12-1,lo[REGISTER],hi[REGISTER]);
 pattern.p[ROOT]=root;pattern.p[REGISTER]=octave;
 run.transpose=0;run.held|=1<<key;
 if(!run.running)start(&run);
 return 1;
}
static void up(void *p,int key){(void)p;if(key<0||key>23)return;run.held&=~(1<<key);if(!run.held&&!run.latch)stop(&run);}
static void tick(void *p,int pos){(void)p;(void)pos;if(active_mode()==0){snapshot_tick();advance(&run,&pattern);}}
static const struct opus_os_vfn vtable[8]={{0,0,0},{0,0,noop},{0,0,down},{0,0,up},{0,0,halt},{0,0,tick},{0,0,noop},{0,0,halt}};
static struct {const struct opus_os_vfn *vt;unsigned pad[7];} players[2]={{vtable,{0}},{vtable,{0}}};
#ifndef FLUKE_HOST
_Static_assert(sizeof(struct opus_os_vfn)==8,"OS function entry");
_Static_assert(sizeof players[0]==32,"OS player object");
#endif
static void *get_player(int m){return m>=0&&m<2?&players[m]:0;}
static uint32_t entropy(void){
 unsigned cycles;
#ifdef FLUKE_HOST
 static unsigned counter;cycles=++counter*7919u;
#else
 __asm__ volatile ("%0 = CYCLES;" : "=d"(cycles));
#endif
 struct opus_clock c;opus_clock(&c);
 return cycles^(unsigned)c.year*3661u^(unsigned)c.month*2678400u^
  (unsigned)c.day*86400u^(unsigned)c.hour*3600u^(unsigned)c.minute*60u^c.second;
}
static void fresh_pattern(void){
 *previous_pattern=pattern;
 uint32_t s=pattern.seed^entropy();previous_pattern->seed=random_word(&s);
 if(previous_pattern->seed==pattern.seed)previous_pattern->seed++;
 generate(previous_pattern);
 Pattern *old=current_pattern;current_pattern=previous_pattern;previous_pattern=old;
}
#include "snapshot.h"
static void chosen(int m,int on){
 if(m!=0)return;
 stop(&run);run.latch=0;recall_pending=0;
 if(on){
  for(int i=0;i<PARAMS;i++)pattern.p[i]=defaults[i];
  pattern.visible_mask=0xffffu;pattern.first_note=0;
  for(int i=0;i<8;i++)turn_acc[i]=0;
  run.transpose=0;
  fresh_pattern();
 }
}
static void midi_start(int m){if(mode_ok(m)&&run.running)start(&run);}
static int key(int m,int code,int pressed){
 if(!mode_ok(m))return 0;
 if(code==0x0c){if(pressed&&run.running)run.jump_pending=1;return 1;}
 if(code==0x0a||code==0x0b){
  if(pressed&&!fw_shift_held()){if(code==0x0a)snapshot_save();else snapshot_restore();}
  return 1;
 }
 if(code<0x3b||code>0x3e)return 0;
 /* Encoder pushes are Main-page actions. Shifted pushes have no action. */
 if(fw_shift_held())return 1;
 if(!pressed)return 1;
 if(code==0x3c){pattern.p[RHYTHM]^=1;pattern.p[PULSES]=bounded(pattern.p[PULSES],0,pattern.p[LENGTH]);}
 else if(code==0x3d){
  *previous_pattern=pattern;previous_pattern->p[SCALE]=(pattern.p[SCALE]+1)%6;
  generate(previous_pattern);
  Pattern *old=current_pattern;current_pattern=previous_pattern;previous_pattern=old;
 }
 else if(code==0x3b)fresh_pattern();
 else {run.latch=!run.latch;if(run.latch&&!run.running)start(&run);else if(!run.latch&&!run.held)stop(&run);}
 return 1;
}
static const int map[2][4]={{LENGTH,DENSITY,OFFSET,RATE},{OCTAVE,TYPE,DIRECTION,SWING}};
static int turn(int m,int enc,int delta){
 if(!mode_ok(m)||enc<0||enc>7)return 0;
 int sh=enc>=4||fw_shift_held(),lane=enc&3,pg=sh?1:0;
 int *a=&turn_acc[sh*4+lane];if((*a>0&&delta<0)||(*a<0&&delta>0))*a=0;
 /* OS native turns use Q10; four units per detent, as existing sequencer adapters. */
 *a+=delta;int n=*a/4096;*a-=n*4096;if(!n)return 1;
 int k=map[pg][lane];if(!pg&&lane==1&&pattern.p[RHYTHM])k=PULSES;
 if(k==RATE)n=-n;
 int maximum=k==PULSES?pattern.p[LENGTH]:hi[k];
 int v=bounded(pattern.p[k]+n,lo[k],maximum);
 if(k==OFFSET){rotate_visible(&pattern,n);return 1;}
 if(k==LENGTH){set_length(&pattern,v);return 1;}
 pattern.p[k]=v;
 if(k==OCTAVE)shape_phrases(&pattern);
 return 1;
}
static void show(void *n,int on){if(n)((unsigned char *)n)[0x50]=(unsigned char)!!on;}
static void text(int slot,const char *s){int n=slot<16?slot+draw_mono*17:slot;fw_set_text(texts[n],s);fw_text_draw(texts[n],fw_screen_raster(),identity);}
static void screen_init(void *scr){
 void *doc=*(void **)scr;char id[20];
 for(int i=0;i<16;i++){
  fw_snprintf(id,sizeof id,"note%i",i);note_nodes[i]=fw_svg_node_find(doc,id,0);
  fw_snprintf(id,sizeof id,"play%i",i);active_nodes[i]=fw_svg_node_find(doc,id,0);
  fw_snprintf(id,sizeof id,"rest%i",i);rest_nodes[i]=fw_svg_node_find(doc,id,0);
  fw_snprintf(id,sizeof id,"mnote%i",i);mono_notes[i]=fw_svg_node_find(doc,id,0);
  fw_snprintf(id,sizeof id,"mplay%i",i);mono_active[i]=fw_svg_node_find(doc,id,0);
  fw_snprintf(id,sizeof id,"mrest%i",i);mono_rests[i]=fw_svg_node_find(doc,id,0);
 }
 color_art=fw_svg_node_find(doc,"color",0);mono_art=fw_svg_node_find(doc,"mono",0);popup=fw_svg_node_find(doc,"popup",0);
 fw_text_new(texts[0],230*1024,9,11,root_color,0);
 fw_text_new(texts[1],266*1024,9,11,root_color,0);
 for(int k=0;k<4;k++){
  fw_text_new(texts[2+k],(8+k*78)*1024,93,11,rgba[k],0);
  fw_text_new(texts[6+k],(8+k*78)*1024,107,18,rgba[k],0);
  fw_text_new(texts[10+k],(8+k*78)*1024,134,10,k==0?soft_blue:k==1?soft_green:k==3?hold_dim:caption_white,0);
 }
 fw_text_new(texts[14],242*1024,134,10,rgba[3],0);
 fw_text_new(texts[15],160*1024,9,11,page_color,1);
 fw_text_new(texts[16],160*1024,48,16,caption_white,1);
 /* Native text objects keep independent monochrome colors. */
 for(int i=0;i<16;i++){
  int x=i==0?230:i==1?266:i==15?160:8+((i<6?i-2:i<10?i-6:i<14?i-10:3)*78);
  int y=i<2||i==15?9:i<6?93:i<10?107:134;
  int size=i<6||i==15?11:i<10?18:10;
  const unsigned char *c=i==13?hold_dim:i==15?page_color:i<2?root_color:i<10?rgba[(i-2)%4]:i==10?soft_blue:i==11?soft_green:caption_white;
  unsigned char *grey=mono_colors[i];grey[0]=grey[1]=grey[2]=(c[0]*77+c[1]*150+c[2]*29)/256;grey[3]=255;
  fw_text_new(texts[17+i],x*1024,y,size,grey,i==15);
 }
}
static void screen_draw(void *scr,int dt){
 (void)dt;if(active_mode()!=0){
  for(int i=0;i<16;i++){show(note_nodes[i],0);show(active_nodes[i],0);show(rest_nodes[i],0);}
  fw_screen_draw(scr);text(0,"synth only");return;
 }
 int pg=!!fw_shift_held();
 draw_mono=snapshot_notice_frames>0;
 show(color_art,!draw_mono);show(mono_art,draw_mono);show(popup,0);
 for(int i=0;i<16;i++){
  Step st=get_step(&pattern,i,0);int active=i<pattern.p[LENGTH],v=st.velocity;
  int base=12*(pattern.p[REGISTER]+1)+pattern.p[ROOT];
  int range=pattern.p[OCTAVE],span=12*((range<0?-range:range)+1)-1;
  int bottom=base+(range<0?12*range:0);
  int note_y=bounded(79-(st.pitch-bottom)*39/span,40,79);
  int m[9]={1024,0,0,0,1024,0,0,(note_y-58)*1024,1024};
  if(note_nodes[i])fw_node_set_matrix(note_nodes[i],m);
  if(mono_notes[i])fw_node_set_matrix(mono_notes[i],m);
  show(mono_notes[i],active&&v);show(mono_rests[i],active&&!v);show(mono_active[i],run.running&&run.current==i);
  show(note_nodes[i],active&&v);
  show(rest_nodes[i],active&&!v);show(active_nodes[i],run.running&&run.current==i);
 }
 fw_screen_draw(scr);
 static const char *const roots[13]={"c","c#","d","d#","e","f","f#","g","g#","a","a#","b","c+12"};
 int root_note=12*(pattern.p[REGISTER]+1)+pattern.p[ROOT];
 char title[24];fw_snprintf(title,sizeof title,"%s%i",roots[root_note%12],root_note/12-1);text(0,title);
 text(1,scale_names[pattern.p[SCALE]]);text(15,pg?"2/2":"1/2");
 static const char *const labels[PARAMS]={"style","density","length","root","rate","swing","rotate","range","dir","scale","rhythm","pulses","register"};
 char value[24];
 for(int k=0;k<4;k++){
  int p=map[pg][k];if(!pg&&k==1&&pattern.p[RHYTHM])p=PULSES;text(2+k,labels[p]);
  if(p==DIRECTION){static const char *const dirs[5]={"fwd","rev","f+r","jump","rjump"};fw_snprintf(value,sizeof value,"%s",dirs[pattern.p[p]]);}
  else if(p==RATE)fw_snprintf(value,sizeof value,"%s",rate_names[pattern.p[p]]);
  else if(p==ROOT)fw_snprintf(value,sizeof value,"%s",roots[pattern.p[p]]);
  else if(p==TYPE)fw_snprintf(value,sizeof value,"%s",type_names[pattern.p[p]]);
  else if(p==OCTAVE&&pattern.p[p]>0)fw_snprintf(value,sizeof value,"+%i",pattern.p[p]);
  else fw_snprintf(value,sizeof value,"%i",pattern.p[p]);
  text(6+k,value);
 }
 text(10,pg?"":"new");text(11,pg?"":pattern.p[RHYTHM]?"density":"pulses");
 text(12,pg?"":"scale");
 if(!pg)text(run.latch?14:13,run.latch?"hold on":"hold off");
 if(draw_mono){
  static const char *const messages[6]={"","saved","recalled","no save","save fail","saving"};
  show(popup,1);fw_group_draw(popup,*(void **)scr,identity,fw_screen_raster());show(popup,0);
  /* Draw the popup subtree last without clearing the background. */
  text(16,messages[snapshot_notice]);snapshot_notice_frames--;
 }
}
static struct opus_sequencer seq={.size=sizeof seq,.name="fluke",.player=get_player,.key=key,.turn=turn,
 .midi_start=midi_start,.chosen=chosen,.screen_init=screen_init,.screen_draw=screen_draw};
int opl_init(struct opl_self *self){
 emit_on=note_on;emit_off=note_off;
 initialise(&pattern,entropy());stop(&run);
 fw_snprintf(art_path,sizeof art_path,"%s/screen.svg",self->folder);
 /* opus_art_text writes only missing/changed art with headroom checks. */
 if(opus_art_text(self,"screen.svg",fluke_svg,sizeof fluke_svg-1)<0)return -1;
 if(!snapshot_init(self))return -3;
 opus_on_main_pass(self,snapshot_main);
 seq.svg=art_path;
 return opus_seq_register(self,&seq)<0?-2:0;
}
void *memcpy(void *d,const void *s,unsigned n){unsigned char *a=d;const unsigned char *b=s;for(unsigned i=0;i<n;i++)a[i]=b[i];return d;}
void *memset(void *d,int c,unsigned n){unsigned char *a=d;for(unsigned i=0;i<n;i++)a[i]=(unsigned char)c;return d;}
