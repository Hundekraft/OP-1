from pathlib import Path
import math
root=Path(__file__).parent
ratios=[1,2**(1/12),16/15,10/9,2**(2/12),9/8,8/7,7/6,2**(3/12),6/5,5/4,2**(4/12),9/7,4/3,2**(5/12),7/5,2**(6/12),10/7,2**(7/12),3/2,14/9,11/7,2**(8/12),8/5,5/3,2**(9/12),12/7,7/4,16/9,2**(10/12),9/5,13/7,15/8,2**(11/12),2]
labels=['0 st','+1 st','16/15','10/9','+2 st','9/8','8/7','7/6','+3 st','6/5','5/4','+4 st','9/7','4/3','+5 st','7/5','+6 st','10/7','+7 st','3/2','14/9','11/7','+8 st','8/5','5/3','+9 st','12/7','7/4','16/9','+10 st','9/5','13/7','15/8','+11 st','+12 st']
tables={'PD_TIME':[round(44.1*1000**(i/255)) for i in range(256)],'PD_RATE':[round(r*2**o*1048576) for o in range(-3,4) for r in ratios],'PD_CENTS':[round(1200*math.log2(r)) for r in ratios],'PD_CLIP':[round(16384*math.atan(i/32)) for i in range(257)]}
v=[]
for i in range(256):
 f=i/256;w=[-.5*f+f*f-.5*f**3,1-2.5*f*f+1.5*f**3,.5*f+2*f*f-1.5*f**3,-.5*f*f+.5*f**3]
 q=[round(x*16384) for x in w];q[1]+=16384-sum(q);v.extend(q)
tables['PD_CUBIC']=v
out='/* Generated on host. No floating point runs on the OP-1. */\n'
for n,v in tables.items():
 typ='short' if n in ['PD_CLIP','PD_CUBIC','PD_CENTS'] else 'int'
 out+=f'static const {typ} {n}[{len(v)}]={{\n'
 for i in range(0,len(v),12):out+=' '+','.join(map(str,v[i:i+12]))+',\n'
 out+='};\n'
out+='static const char *const PD_LABELS[35]={'+','.join('"'+s+'"' for s in labels)+'};\n'
(root/'tables.h').write_text(out)
# 17 owners x ten bits: octave+3 in 3 bits; depth-10 in 7 bits.
packed=sum(123<<(10*i) for i in range(17));words=[(packed>>(16*i))&65535 for i in range(11)]
(root/'defaults.h').write_text('static const unsigned short PD_EXTRA_DEFAULTS[11]={'+','.join(map(str,words))+'};\n')
