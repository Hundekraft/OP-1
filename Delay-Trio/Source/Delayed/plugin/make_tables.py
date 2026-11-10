from pathlib import Path
import math
root=Path(__file__).parent
tables={'DL_TIME':[round(44.1*1000**(i/255)) for i in range(256)],'DL_FREQ':[round(20*1000**(i/127)) for i in range(128)],'DL_RATE':[round(0.01*1000**(i/127)*2**32/44100) for i in range(128)],'DL_SINE':[round(32767*math.sin(2*math.pi*i/1024)) for i in range(1024)],'DL_CLIP':[round(16384*math.atan(i/32)) for i in range(257)]}
res=[]
for tone in range(128):
 f0=tables['DL_FREQ'][tone]/44100
 c=1/(f0*math.pi) if f0<.1 else math.tan((.5-f0)*math.pi)
 for r in range(64):
  damping=(1-r/63)*(math.sqrt(2)-.1)+.1
  a=1/(1+damping*c+c*c)
  res.extend(round(v*(1<<28)) for v in [a,2*(1-c*c)*a,(1-damping*c+c*c)*a])
tables['DL_FILTER']=res
cubic=[]
for i in range(256):
 f=i/256
 w=[-.5*f+f*f-.5*f*f*f,1-2.5*f*f+1.5*f*f*f,.5*f+2*f*f-1.5*f*f*f,-.5*f*f+.5*f*f*f]
 q=[round(x*16384) for x in w];q[1]+=16384-sum(q);cubic.extend(q)
tables['DL_CUBIC']=cubic
out='/* Generated on host; no floating point on device. */\n'
for name,v in tables.items():
 typ='short' if name in ('DL_SINE','DL_CLIP','DL_CUBIC') else 'int'
 out+=f'static const {typ} {name}[{len(v)}]={{\n'
 for i in range(0,len(v),12):out+=' '+','.join(map(str,v[i:i+12]))+',\n'
 out+='};\n'
(root/'tables.h').write_text(out)
