"""Run native API contract fixtures; requires clang and a local Opus kit."""
from pathlib import Path
import subprocess,sys,tempfile
root=Path(__file__).resolve().parent
if len(sys.argv)!=2:
 raise SystemExit('Usage: python3 Source/test.py /path/to/plugin-kit')
kit=Path(sys.argv[1]).resolve();p=root/'plugin'
with tempfile.TemporaryDirectory(prefix='bubbler-native-') as tmp:
 cmd=['clang','-std=c99','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all','-DTEST_BUBBLER','-I'+str(kit/'sdk'),'-I'+str(p),str(root/'tests/test_native.c')]
 cmd += [str(p/n) for n in ['bubbler_art.c','bubbler_dsp.c','bubbler_params.c','bubbler_midi.c','native_screen.c']]
 cmd += ['-o',str(Path(tmp)/'test')]
 subprocess.run(cmd,check=True);subprocess.run([str(Path(tmp)/'test')],check=True)
