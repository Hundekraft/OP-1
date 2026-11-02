"""Regenerate the embedded SVG header after editing a plugin's screen.svg."""
from pathlib import Path
import sys,json,xml.etree.ElementTree as ET
if len(sys.argv)!=2:raise SystemExit('Usage: python3 Source/embed_svg.py /path/to/plugin')
p=Path(sys.argv[1]);svg=(p/'screen.svg').read_text();root=ET.fromstring(svg)
group=next(n for n in root.iter() if n.attrib.get('id')=='figures');nodes=list(group)
for i,n in enumerate(nodes):assert n.attrib['id']=='figure'+str(i)
fill=[int(n.attrib.get('fill','none')!='none') for n in nodes]
red=[int(n.attrib.get('fill')=='#FF3A5D' or n.attrib.get('stroke')=='#FF3A5D') for n in nodes]
minimum=48 if 'id        esc.delay\n' in (p/'manifest.txt').read_text() else 128
h='/* Original approved artwork; GPL-3.0-or-later. */\nstatic const char ESC_SVG[]=\n'+''.join(json.dumps(line+'\n')+'\n' for line in svg.splitlines())+';\n#define ESC_ART_SHAPES '+str(len(nodes))+'\nstatic const unsigned char ESC_ART_FILL[] = {'+','.join(map(str,fill))+'};\nstatic const unsigned char ESC_ART_RED[] = {'+','.join(map(str,red))+'};\n#define ESC_ART_MIN '+str(minimum)+'\n'
(p/'native_svg.h').write_text(h)
