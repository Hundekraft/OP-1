from pathlib import Path
import subprocess,sys,tempfile
root=Path(__file__).resolve().parent;t=root/'tests'
if len(sys.argv)!=2:raise SystemExit('Usage: python3 Source/test.py /path/to/plugin-kit')
kit=Path(sys.argv[1]).resolve()
base=['clang','-std=c99','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all']
with tempfile.TemporaryDirectory() as tmp:
 for name,flag,files in [('Bubbler','TEST_BUBBLER',['bubbler_art.c','bubbler_dsp.c','bubbler_params.c','bubbler_midi.c','native_screen.c']),('Delayed','TEST_DELAY',['delay_art.c','delay_dsp.c','native_screen.c']),('Pitch-Delay','TEST_PITCH',['pitch_art.c','pitch_dsp.c','pitch_params.c','native_screen.c'])]:
  p=root/name/'plugin';exe=str(Path(tmp)/name)
  subprocess.run(base+['-D'+flag,'-I'+str(kit/'sdk'),'-I'+str(p),str(root/name/'test_native.c')]+[str(p/f) for f in files]+['-o',exe],check=True)
  subprocess.run([exe],check=True)
 p=root/'Delayed/plugin';obj=str(Path(tmp)/'ref.o')
 macros=['-D'+n+'=ref_'+n[3:] for n in ['dl_init','dl_render','dl_wave','dl_time','dl_freq','dl_rate_milli']]
 subprocess.run(base+macros+['-c',str(t/'reference/delay_dsp.c'),'-o',obj],check=True)
 for name,flags,files in [('smooth',['-DDL_SMOOTH_TEST'],[t/'test_smooth.c',p/'delay_dsp.c',Path(obj)]),('stress',[],[t/'test_stress.c',p/'delay_dsp.c'])]:
  exe=str(Path(tmp)/name)
  subprocess.run(base+flags+['-I'+str(p)]+[str(x) for x in files]+['-o',exe],check=True)
  subprocess.run([exe],check=True)
