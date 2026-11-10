/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include "plugin/device.c"
#include "plugin/dsp.c"

static const struct opus_effect *host;
static int shift, browser;
static struct opus_page shown;
int opus_page_now(struct opus_page *pg) { *pg=shown;return shown.module!=0; }
static int text_draws;
static unsigned char segment_fills[14][4], logo_colours[sizeof logo_banks][4];
static int control_y[6], control_size[6], text_objects;
static const struct opus_folder *folder;
static unsigned char incoming[ROM_MAX], stored[ROM_BANKS][ROM_MAX];
static unsigned position[3 + ROM_BANKS], stored_size[ROM_BANKS], incoming_size;

int opus_on_folder(struct opl_self *self, const struct opus_folder *f)
{
    assert(self && !strcmp(f->name, "mv1rom") && f->max == ROM_MAX);
    folder = f;
    return 1;
}

int opus_effect_register(struct opl_self *self,const struct opus_effect *e)
{ assert(self && e->size==sizeof *e);host=e;return 8; }
int opus_flash_headroom(void) { return 32768; }
int opus_fn_held(void) { return 0; }
int opus_fx_list_shown(void) {return browser;}
int fw_shift_held(void) { return shift; }
int fw_yaffs_stat(const char *path, unsigned *st) { (void)path; (void)st; return -1; }
int fw_yaffs_mkdir(const char *path, int mode) { (void)path; (void)mode; return 0; }
int fw_yaffs_open(const char *path, int flags, int mode)
{
    (void)mode;
    if (!strcmp(path, "/yaffs2/mod/esc.midireverb/screen.svg")) return 1;
    if (!strcmp(path, "/incoming")) { position[2] = 0; return 2; }
    for (unsigned bank = 0; bank < ROM_BANKS; ++bank) {
        if (strcmp(path, banks[bank].path)) continue;
        position[3 + bank] = 0;
        if (flags) stored_size[bank] = 0;
        else if (!stored_size[bank]) return -1;
        return 3 + (int)bank;
    }
    assert(0);
    return -1;
}
int fw_yaffs_close(int fd) { assert(fd >= 1 && fd < 3 + ROM_BANKS); return 0; }
int fw_yaffs_unlink(const char *path)
{
    for (unsigned bank = 0; bank < ROM_BANKS; ++bank)
        if (!strcmp(path, banks[bank].path)) { stored_size[bank] = 0; return 0; }
    return -1;
}
int fw_yaffs_read(int fd, void *buf, unsigned n)
{
    assert(fd >= 2 && fd < 3 + ROM_BANKS);
    unsigned size = fd == 2 ? incoming_size : stored_size[fd - 3];
    if (n > size - position[fd]) n = size - position[fd];
    memcpy(buf, (fd == 2 ? incoming : stored[fd - 3]) + position[fd], n);
    position[fd] += n;
    return (int)n;
}
int fw_yaffs_write(int fd, const void *buf, unsigned n)
{
    if (fd == 1) assert(buf == screen_svg && n == sizeof screen_svg - 1);
    else {
        assert(fd >= 3 && fd < 3 + ROM_BANKS && stored_size[fd - 3] + n <= ROM_MAX);
        memcpy(stored[fd - 3] + stored_size[fd - 3], buf, n);
        stored_size[fd - 3] += n;
    }
    return (int)n;
}
void *fw_text_new(void *mem, int x, int y, int size, const unsigned char *rgba, int align)
{
    assert(x >= 0 && x <= 320 * 1024 && y >= 0 && y + size <= 146);
    assert(rgba && align >= 0 && align <= 2);
    if (text_objects < 6) {
        control_y[text_objects] = y;
        control_size[text_objects] = size;
        assert(align == (text_objects & 1 ? 2 : 0));
    }
    ++text_objects;
    return mem;
}
void fw_text_draw(void *mem, void *raster, const int *matrix)
{ (void)mem; (void)raster; (void)matrix; ++text_draws; }
void fw_set_text(void *mem, const char *str) { (void)mem; assert(strlen(str) < 32); }
void *fw_screen_raster(void) { return 0; }
void fw_screen_draw(void *scr) { assert(scr); }

int fw_snprintf(char *dst, unsigned size, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int result = vsnprintf(dst, size, format, args);
    va_end(args);
    return result;
}

void *fw_svg_node_find(void *document, const char *id, int warn)
{
    assert(document && !warn && (!strncmp(id, "seg", 3) || !strncmp(id, "logo", 4)));
    return document;
}

void *fw_screen_fill(void *scr, const char *id)
{
    if (!strncmp(id, "logo", 4)) {
        int index = (id[4] - '0') * 10 + id[5] - '0';
        assert(scr && index >= 0 && (unsigned)index < sizeof logo_banks);
        return logo_colours[index];
    }
    int index = (id[3] - '0') * 10 + id[4] - '0';
    assert(scr && index >= 0 && index < 14);
    return segment_fills[index];
}

static double filter_gain(double frequency)
{
    struct tank state = {0};
    double in_energy = 0, out_energy = 0;
    for (int i = 0; i < 8192; ++i) {
        int in = (int)(12000 * sin(6.283185307179586 * frequency * i / 44100));
        int out = reconstruct(&state, in);
        if (i > 256) {
            in_energy += in * in;
            out_energy += out * out;
        }
    }
    return 10 * log10(out_energy / in_energy);
}

static void (*setting_callback)(const char *, int);
int opus_setting(struct opl_self *self, const char *name)
{ assert(self && (!strcmp(name, "reconstruction") || !strcmp(name, "engine"))); return !strcmp(name, "reconstruction"); }
void opus_on_setting(struct opl_self *self, void (*fn)(const char *, int))
{ assert(self && fn); setting_callback = fn; }


static struct instance states[3];
static int16_t rings[3][RING_WORDS];
static struct opus_fx fx[3];
static short params[3][8];
static struct opus_page page_for(int i,int where)
{
 struct opus_page pg={0};pg.kind=OPUS_PAGE_EFFECT;pg.where=(short)where;pg.slot=0;
 pg.n=8;pg.on=1;pg.par=params[i];pg.inst=&fx[i];pg.module=host;return pg;
}
static void fresh(int i)
{
 memset(&states[i],0,sizeof states[i]);memset(rings[i],0,sizeof rings[i]);
 fx[i]=(struct opus_fx){.state=&states[i],.bulk=rings[i]};
 memcpy(params[i],host->defaults,sizeof params[i]);host->init(&fx[i]);
}
static void run(int i,unsigned blocks)
{
 short buf[64];
 for(unsigned j=0;j<blocks;j++) {
  for(unsigned n=0;n<64;n++)buf[n]=(short)(n*97);
  host->mono(&fx[i],buf,params[i]);
 }
}
int main(int argc,char **argv)
{
 assert(argc==4);
 struct opl_self self={sizeof self,"esc.midireverb","/yaffs2/mod/esc.midireverb",0};
 assert(opl_init(&self)==0 && host && folder && !rom_ready);
 assert(!strcmp(host->name,"midireverb"));
 for(unsigned bank=0;bank<ROM_BANKS;bank++) {
  FILE *f=fopen(argv[bank+1],"rb");assert(f);
  incoming_size=(unsigned)fread(incoming,1,ROM_MAX,f);fclose(f);
  assert(folder->take(banks[bank].file,"/incoming"));
 }
 folder->changed();assert(rom_ready==7);
 struct opus_need n={0};host->need(0,&n);
 assert(n.state==sizeof(struct instance) && n.bulk==sizeof rings[0] && !n.fast);
 host->need(1,&n);assert(!n.state && !n.bulk && !n.fast);
 for(int i=0;i<3;i++)fresh(i);
 struct opus_page p=page_for(0,0),q=page_for(1,0),dr=page_for(2,1);
 // Same owner, independent first and managed second instance.
 assert(host->page_turn(&p,3,14));assert(decode(0,params[0][0],0)==14);
 assert(host->page_key(&p,0x3e,1));assert(params[0][4]==2);
 assert(host->page_turn(&p,3,22));assert(decode(0,params[0][0],2)==22);
 assert(params[1][4]==0 && decode(0,params[1][0],0)==0);
 assert(host->page_key(&p,0x3e,1));assert(params[0][4]==1);
 assert(host->page_turn(&p,3,9));
 assert(host->page_key(&p,0x3e,1));assert(params[0][4]==0 && decode(0,params[0][0],0)==14);
 assert(host->page_key(&p,0x3e,1));assert(params[0][4]==2 && decode(0,params[0][0],2)==22);
 assert(host->page_turn(&q,3,3));assert(host->page_turn(&dr,0,37));
 run(0,16);run(1,16);run(2,16);
 struct instance before=states[1];static int16_t snapshot[RING_WORDS];memcpy(snapshot,rings[1],sizeof snapshot);
 run(0,5);assert(!memcmp(&before,&states[1],sizeof before));assert(!memcmp(snapshot,rings[1],sizeof snapshot));
 // A native factory call during synth reload preserves state and effective controls.
 unsigned ptr=states[0].tank.pointer;short held[8];memcpy(held,states[0].heard,sizeof held);
 host->init(&fx[0]);assert(states[0].tank.pointer==ptr);
 memcpy(params[0],host->defaults,sizeof params[0]);run(0,1);
 assert(states[0].active_bank==2 && states[0].active_program==22);
 assert(!memcmp(held,states[0].heard,sizeof held));assert(states[0].tank.clear_at==RING_WORDS);
 host->page_turn(&p,0,1);assert(params[0][4]==2 && decode(0,params[0][0],2)==22);
 // Explicit effect browser re-selection gets default values and resets the tail.
 browser=1;host->init(&fx[0]);browser=0;
 memcpy(params[0],host->defaults,sizeof params[0]);run(0,1);
 assert(states[0].active_bank==0 && states[0].active_program==0 && !states[0].held_mask);
 assert(states[0].tank.clear_at==2048);
 // Core clearing a managed instance / fresh factory resets it independently.
 fresh(1);run(1,1);assert(states[1].active_bank==0 && states[1].active_program==0);
 // Saved eight-word records carry the ROM and each bank's preset.
 short saved[8];memcpy(saved,params[2],sizeof saved);fresh(2);memcpy(params[2],saved,sizeof saved);run(2,9);
 assert(decode(2,states[2].heard[2],0)==37);
 // Screen follows the shown instance, not owner or audio callback order.
 void *document=&self;host->screen_init(&document);
 shown=q;host->screen_draw(&document,0);assert(last_logo==0);
 host->page_key(&q,0x3e,1);shown=q;host->screen_draw(&document,0);assert(last_logo==2);
 shown=p;host->screen_draw(&document,0);assert(last_logo==0);
 for(int i=0;i<6;i+=2)assert(control_y[i]==control_y[i+1]&&control_size[i]==control_size[i+1]);
 assert(fabs(filter_gain(10136)+3.01)<0.1);
 struct opus_fx masterfx={.stereo=1};short l[64]={1},r[64]={2};host->init(&masterfx);
 host->stereo(&masterfx,l,r,host->defaults);assert(l[0]==1&&r[0]==2);
 // Every bank/program in both modes: native adapter equals direct 0.2.9 DSP.
 static int16_t reference_ring[RING_WORDS];unsigned blocks=0;
 for(unsigned bank=0;bank<3;bank++)for(unsigned prog=0;prog<banks[bank].programs;prog++)for(int engine=0;engine<2;engine++){
  fresh(0);params[0][4]=(short)bank;params[0][0]=encode(0,prog,bank);
  params[0][2]=encode(2,20,bank);params[0][3]=encode(3,65,bank);
  low_cpu=engine;memset(reference_ring,0,sizeof reference_ring);struct tank ref={.ring=reference_ring};tank_reset(&ref);
  if(bank)extra_reset(ref.extra,bank,prog);
  unsigned controls[4]={prog,90,20,65};
  for(int j=0;j<32;j++){
   short a[64],b[64];for(int k=0;k<64;k++)a[k]=b[k]=(short)((k*983+j*317)%24000-12000);
   host->mono(&fx[0],a,params[0]);tank_render(&ref,b,controls,bank,1,engine);
   assert(!memcmp(a,b,sizeof a));assert(!memcmp(rings[0],reference_ring,sizeof reference_ring));blocks++;
  }
 }
 setting_callback("reconstruction",0);assert(!reconstruction);setting_callback("engine",0);assert(!low_cpu);
 printf("PASS: native first/second/drum isolation, per-ROM presets, reload/reselection, screen, ROM import, settings and %u reference audio blocks\n",blocks);
}
