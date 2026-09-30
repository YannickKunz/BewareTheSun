"""Synthesize the rework's original soft chimes, leaf-like effects and ambient score."""
from pathlib import Path
import math, wave, struct, random
OUT = Path(__file__).resolve().parents[1] / 'game/assets/audio'
RATE=22050
random.seed(42)
def write(name, length, voices, noise=0):
    data=[]
    for i in range(int(length*RATE)):
        t=i/RATE; value=0
        for start,freq,duration,amp in voices:
            u=t-start
            if 0<=u<duration:
                env=min(1,u/.018)*math.exp(-u*4/duration)*min(1,(duration-u)/.06)
                value += amp*env*(math.sin(math.tau*freq*u)+.22*math.sin(math.tau*freq*2.003*u))
        value+=noise*random.uniform(-1,1)*max(0,1-t/length)**2
        data.append(struct.pack('<h',int(max(-.95,min(.95,value))*32767)))
    with wave.open(str(OUT/(name+'.wav')),'wb') as f:
        f.setnchannels(1);f.setsampwidth(2);f.setframerate(RATE);f.writeframes(b''.join(data))
write('drop',.5,[(0,880,.35,.24),(.09,1320,.4,.18)])
write('switch',.22,[(0,430,.2,.15),(.025,640,.17,.1)])
write('dash',.27,[(0,220,.22,.06)],.14)
write('hurt',.4,[(0,130,.3,.2),(.05,155,.27,.1)],.035)
write('bloom',1.7,[(i*.12,f,1.1,.14) for i,f in enumerate([523.25,659.25,783.99,1046.5])])
# 32-second pentatonic garden loop. Deliberate silence at either end prevents clicks.
notes=[261.63,329.63,392,440,523.25,392,329.63,293.66]
voices=[]
for i in range(24):
    voices.append((1+i*1.22,notes[(i*3+i//8)%len(notes)],2.1,.095))
for i,f in enumerate([130.81,164.81,146.83,130.81]):
    voices.extend([(i*8+.5,f,6.5,.085),(i*8+.5,f*1.5,6.5,.04)])
write('garden',32,voices)
print('Generated six original audio files.')
