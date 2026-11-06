#define main base_test_main
#include "test_fluke.c"
#undef main
static void boot(struct opl_self *self){assert(opl_init(self)==0);assert(main_pass_fn==snapshot_main);}
int main(void){
 struct opl_self self={sizeof self,"esc.fluke","/yaffs2/mod/esc.fluke",0};
 boot(&self);assert(!snapshot_ready);Pattern before=pattern;
 assert(key(0,0x0b,1)&&snapshot_notice==3);assert(!memcmp(&before,&pattern,sizeof pattern));
 /* Snapshot the actual tables, controls, rotation membership and hidden notes. */
 pattern.p[SCALE]=2;pattern.p[TYPE]=4;pattern.p[RHYTHM]=1;pattern.p[PULSES]=5;
 set_length(&pattern,13);rotate_visible(&pattern,3);generate(&pattern);before=pattern;
 start(&run);ticks(1);int current=run.current;
 assert(key(0,0x0a,1));assert(run.running&&run.current==current&&snapshot_ready);
 initialise(&pattern,456);assert(key(0,0x0b,1));assert(recall_pending&&run.running);ticks(rate_ticks[pattern.p[RATE]]);assert(!recall_pending&&!memcmp(&before,&pattern,sizeof pattern));
 clear_events();main_pass_fn();assert(snapshot_notice==1&&snapshot_generation==1);
 /* Boot remains fresh/default, with a saved cache available for explicit Drop. */
 boot(&self);assert(snapshot_ready&&pattern.p[LENGTH]==16&&pattern.p[OCTAVE]==1);key(0,0x0b,1);
 assert(!memcmp(&before,&pattern,sizeof pattern)&&!run.running);
 /* Both presses/releases are consumed; Shift-page Lift/Drop work too. */
 shift=1;assert(key(0,0x0a,0)&&key(0,0x0b,0));shift=0;
 /* Partial aligned reads and short writes are handled. */
 initialise(&pattern,789);Pattern second=pattern;max_write=31;key(0,0x0a,1);main_pass_fn();assert(snapshot_notice==1);
 max_write=2048;max_read=17;boot(&self);key(0,0x0b,1);assert(!memcmp(&second,&pattern,sizeof pattern));max_read=256;
 /* Interrupted alternate write leaves previous committed file recoverable. */
 initialise(&pattern,111);write_fail_after=99;key(0,0x0a,1);main_pass_fn();assert(snapshot_notice==4);
 write_fail_after=-1;boot(&self);key(0,0x0b,1);assert(!memcmp(&second,&pattern,sizeof pattern));
 /* Low space fails without attempting a file write. RAM recall still works. */
 initialise(&pattern,222);before=pattern;flash_room=0;int writes=write_calls;
 key(0,0x0a,1);main_pass_fn();assert(snapshot_notice==4&&write_calls==writes);
 initialise(&pattern,333);key(0,0x0b,1);assert(!memcmp(&before,&pattern,sizeof pattern));flash_room=100000;
 /* A newer Lift while main writes remains pending and is saved on next pass. */
 initialise(&pattern,555);key(0,0x0a,1);inject_new_lift=1;main_pass_fn();assert(snapshot_revision!=snapshot_ack);
 before=snapshot_cache;main_pass_fn();assert(snapshot_revision==snapshot_ack&&snapshot_notice==1);
 boot(&self);key(0,0x0b,1);assert(!memcmp(&before,&pattern,sizeof pattern)&&pattern.seed==98765);
 /* Corrupt latest file falls back to the older complete commit. */
 disk[snapshot_slot][24]^=1;boot(&self);key(0,0x0b,1);assert(pattern.seed==555);
 /* File with a valid checksum but invalid controls is rejected. */
 struct saved_pattern invalid={0};invalid.magic=0x464c4b31u;invalid.format=1;invalid.bytes=sizeof(Pattern);invalid.data=pattern;
 invalid.data.p[LENGTH]=99;invalid.checksum=snapshot_hash(&invalid);assert(!snapshot_valid(&invalid));
 /* Split requests physical step one, without moving any step boundary. */
 for(int rate=0;rate<14;rate++)for(int direction=0;direction<5;direction++)for(int sw=0;sw<=100;sw+=100){
  initialise(&pattern,123);pattern.p[DENSITY]=16;pattern.p[RATE]=rate;pattern.p[DIRECTION]=direction;pattern.p[SWING]=sw;generate(&pattern);
  start(&run);int period=rate_ticks[rate];ticks(period*4+1);
  int elapsed=run.elapsed,current=run.current;key(0,0x0c,1);
  assert(run.jump_pending&&run.elapsed==elapsed&&run.current==current);
  key(0,0x0c,0);assert(run.jump_pending);ticks(period-elapsed);
  assert(run.elapsed==0&&run.jump_pending);ticks(1);
  assert(run.current==0&&!run.jump_pending&&run.elapsed==1);
  ticks(period);assert(run.elapsed==1);clear_events();
 }
 /* Recall waits for the old clock boundary, then starts step one at saved Rate. */
 for(int oldrate=0;oldrate<14;oldrate++)for(int newrate=0;newrate<14;newrate++){
  initialise(&pattern,123);pattern.p[DENSITY]=16;pattern.p[RATE]=newrate;pattern.p[DIRECTION]=4;generate(&pattern);snapshot_save();Pattern saved=pattern;
  initialise(&pattern,456);pattern.p[RATE]=oldrate;pattern.p[DENSITY]=16;generate(&pattern);
  start(&run);ticks(3);int elapsed=run.elapsed;uint32_t seed=pattern.seed;
  key(0,0x0b,1);assert(recall_pending&&pattern.seed==seed&&run.elapsed==elapsed);
  int wait=(rate_ticks[oldrate]-elapsed)%rate_ticks[oldrate];
  for(int i=0;i<wait;i++){tick(0,0);clock_tick++;assert(recall_pending&&pattern.seed==seed);}
  assert(run.elapsed==0);tick(0,0);clock_tick++;
  assert(!recall_pending&&!memcmp(&saved,&pattern,sizeof saved)&&run.current==0&&run.elapsed==1);
  ticks(rate_ticks[newrate]);assert(run.elapsed==1);clear_events();
 }
 /* No playback starts when pressing Split while stopped. */
 key(0,0x0c,1);key(0,0x0c,0);assert(!run.running&&!run.jump_pending);
 /* Both pages and monochrome popup use independent native text objects. */
 snapshot_notice_frames=0;shift=0;snapshot(100);assert(!strcmp(mock_text[15].value,"1/2"));
 shift=1;snapshot(101);assert(!strcmp(mock_text[15].value,"2/2"));
 snapshot_feedback(1);assert(snapshot_notice_frames==30);snapshot(102);assert(draw_mono&&!strcmp(mock_text[16].value,"saved"));
 for(int i=17;i<33;i++)assert(mock_text[i].color[0]==mock_text[i].color[1]&&mock_text[i].color[1]==mock_text[i].color[2]);
 snapshot_feedback(2);snapshot(103);assert(!strcmp(mock_text[16].value,"recalled"));shift=0;
 /* Shifted Lift/Drop are ignored and cannot change the saved phrase. */
 initialise(&pattern,1001);snapshot_save();Pattern saved=pattern;
 initialise(&pattern,2002);Pattern unsaved=pattern;unsigned revision=snapshot_revision;
 shift=1;key(0,0x0a,1);key(0,0x0b,1);
 assert(snapshot_revision==revision&&!memcmp(&unsaved,&pattern,sizeof pattern));
 shift=0;key(0,0x0b,1);assert(!memcmp(&saved,&pattern,sizeof pattern));
 /* Both new octave extremes survive save/recall and validation. */
 for(int range=-3;range<=3;range+=6){
  pattern.p[OCTAVE]=range;generate(&pattern);snapshot_save();main_pass_fn();boot(&self);key(0,0x0b,1);
  assert(pattern.p[OCTAVE]==range&&snapshot_valid(&snapshot_io));
 }
 /* Overlay expires after exactly 30 frames. */
 snapshot_feedback(1);void *screen[4]={0};screen_init(screen);
 for(int i=0;i<30;i++){screen_draw(screen,0);assert(draw_mono);}
 screen_draw(screen,0);assert(!draw_mono&&!snapshot_notice_frames);
 /* Header labels stay compact, with a padded field inside x=311. */
 for(int sc=0;sc<6;sc++){
  assert(strlen(scale_names[sc])<=6);pattern.p[SCALE]=sc;
  screen_draw(screen,0);assert(mock_text[1].x==266&&mock_text[0].x==230);
  assert(!strcmp(mock_text[1].value,scale_names[sc]));
 }
 mode=1;assert(!key(1,0x0a,1)&&!key(1,0x0b,1)&&!key(1,0x0c,1));mode=0;
 puts("PASS: exact Lift/Drop snapshots, restart recall/fresh boot, interrupted writes, low space, short I/O, concurrent Lift, corrupt fallback; Split jump and recall boundary sync for all rates/directions, swing, page indicators and monochrome status");
 return 0;
}
