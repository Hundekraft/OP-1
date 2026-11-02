from pathlib import Path
import subprocess,sys,tempfile
root=Path(__file__).resolve().parent
if len(sys.argv)!=2:raise SystemExit('Usage: python3 Source/test.py /path/to/plugin-kit')
kit=Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory() as tmp:
 for name,flag,files in [('Bubbler','TEST_BUBBLER',['bubbler_art.c','bubbler_dsp.c','bubbler_params.c','bubbler_midi.c','native_screen.c']),('Delayed','TEST_DELAY',['delay_art.c','delay_dsp.c','native_screen.c']),('Pitch-Delay','TEST_PITCH',['pitch_art.c','pitch_dsp.c','pitch_params.c','native_screen.c'])]:
  p=root/name/'Source/plugin';exe=str(Path(tmp)/name)
  subprocess.run(['clang','-std=c99','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all','-D'+flag,'-I'+str(kit/'sdk'),'-I'+str(p),str(root/name/'Source/test_native.c')]+[str(p/f) for f in files]+['-o',exe],check=True)
  subprocess.run([exe],check=True)
