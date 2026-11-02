"""Native API/control fixtures; not a TE renderer emulator."""
from pathlib import Path
import subprocess,sys,tempfile
r=Path(__file__).resolve().parent
if len(sys.argv)!=2:raise SystemExit('Usage: python3 Source/test.py /path/to/plugin-kit')
kit=Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory(prefix='esc-native-pair-') as temp:
 for name,flag,files in [('Delayed','TEST_DELAY',['delay_art.c','delay_dsp.c','native_screen.c']),('Pitch-Delay','TEST_PITCH',['pitch_art.c','pitch_dsp.c','pitch_params.c','native_screen.c'])]:
  p=r/name/'plugin';exe=str(Path(temp)/name)
  cmd=['clang','-std=c99','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all','-D'+flag,'-I'+str(kit/'sdk'),'-I'+str(p),str(r/'tests/test_native.c')]+[str(p/f) for f in files]+['-o',exe]
  subprocess.run(cmd,check=True);subprocess.run([exe],check=True)
