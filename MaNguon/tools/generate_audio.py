"""Âm nhạc và SFX tổng hợp nguyên bản. Cần Python + numpy để tái tạo, không cần khi chơi."""
from pathlib import Path
import numpy as np
import wave
root=Path(__file__).resolve().parents[1]/'assets/audio'
sr=22050
rng=np.random.default_rng(1317)
def write(name,samples):
 p=root/name;p.parent.mkdir(parents=True,exist_ok=True)
 samples=np.clip(samples,-.9,.9)
 with wave.open(str(p),'wb') as w:
  w.setnchannels(1);w.setsampwidth(2);w.setframerate(sr);w.writeframes((samples*32767).astype('<i2').tobytes())
def note(freq,length=.5,soft=False):
 t=np.arange(int(sr*length))/sr
 env=(1-np.exp(-t*100))*np.exp(-t*(2 if soft else 9))
 return env*(np.sin(2*np.pi*freq*t)+.18*np.sin(4*np.pi*freq*t)+.07*np.sin(6*np.pi*freq*t))
def hz(midi):return 440*2**((midi-69)/12)
for scene,beat,gain in [('cozy_shop',.5,.115),('day_result',.65,.10)]:
 length=beat*48;out=np.zeros(int(length*sr))
 chords=[[57,60,64,67],[53,57,60,64],[48,52,55,59],[55,59,62,65]]
 def add(at,s):
  start=int(at*sr)
  for offset in range(0,len(s),max(1,len(out)-start)):
   n=min(len(s)-offset,len(out)-start);out[start:start+n]+=s[offset:offset+n];start=0
 for bar in range(12):
  chord=chords[bar%4]
  for k in range(4):
   add(bar*4*beat+k*beat,note(hz(chord[(k+bar)%4]+12),beat*2)*gain)
  add(bar*4*beat,note(hz(chord[0]-12),beat*3,True)*gain*.55)
  if scene=='cozy_shop':
   for k in [1,3]:
    t=np.arange(int(sr*.12))/sr;tap=rng.normal(0,1,len(t))*np.exp(-t*55)*.012;add((bar*4+k)*beat,tap)
 write('music/'+scene+'.wav',out)
for name,notes in {'ui_click':[72],'coin_receive':[79,84,88],'customer_happy':[72,76,79],'customer_leave':[64,60],'day_end':[72,67,64,60],'perfect':[84,88],'upgrade_buy':[67,72,76,79],'mix':[55],'topping_drop':[60]}.items():
 out=np.zeros(int(sr*(.1*len(notes)+.35)))
 for i,n in enumerate(notes):
  s=note(hz(n),.3)*.22;start=int(i*.1*sr);out[start:start+len(s)]+=s
 write('sfx/'+name+'.wav',out)
for name,duration,volume in [('paper_tear',.28,.15)]:
 t=np.arange(int(sr*duration))/sr;noise=rng.normal(0,1,len(t));noise=np.convolve(noise,np.ones(5)/5,mode='same')
 env=np.sin(np.pi*t/duration)**2
 write('sfx/'+name+'.wav',volume*noise*env)
print('Created original music and SFX')
