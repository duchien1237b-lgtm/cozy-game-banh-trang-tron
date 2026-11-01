"""SFX bổ sung cho bản nâng cấp: giờ cao điểm, combo, thành tựu, quà, đếm ngược, xu bay.
Tổng hợp nguyên bản bằng numpy; không cần khi chơi/build."""
from pathlib import Path
import numpy as np
import wave
root = Path(__file__).resolve().parents[1] / 'assets/audio/sfx'
sr = 22050
rng = np.random.default_rng(2026)
def hz(m): return 440 * 2 ** ((m - 69) / 12)
def write(name, s):
    root.mkdir(parents=True, exist_ok=True)
    s = np.clip(s, -.9, .9)
    with wave.open(str(root / (name + '.wav')), 'wb') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(sr)
        w.writeframes((s * 32767).astype('<i2').tobytes())
def bell(freq, length, decay=6, gain=.3):
    t = np.arange(int(sr * length)) / sr
    env = (1 - np.exp(-t * 400)) * np.exp(-t * decay)
    tone = np.sin(2 * np.pi * freq * t) + .5 * np.sin(2 * np.pi * freq * 2.76 * t) * np.exp(-t * 4) + .25 * np.sin(2 * np.pi * freq * 5.4 * t) * np.exp(-t * 9)
    return env * tone * gain
def place(out, at, s):
    a = int(at * sr); n = min(len(s), len(out) - a)
    if n > 0: out[a:a + n] += s[:n]
def seq(notes, gap, length=.5, decay=6, gain=.3):
    out = np.zeros(int(sr * (gap * len(notes) + length)))
    for i, n in enumerate(notes): place(out, i * gap, bell(hz(n), length, decay, gain))
    return out
write('rush_start', seq([67, 72, 76, 79, 84], .09, .6, 5, .26))
write('combo_up', seq([76, 79, 83], .06, .35, 9, .28))
write('achievement', seq([72, 76, 79, 84, 79, 84, 88], .1, .8, 4, .26))
write('reward_claim', seq([79, 84, 88, 91, 96], .07, .7, 4.5, .24))
write('warning_tick', bell(1320, .12, 28, .35))
write('coin_fly', seq([88, 95], .05, .22, 14, .22))
write('undo', seq([72, 65], .05, .2, 16, .25))
t = np.arange(int(sr * .35)) / sr
write('whoosh', np.convolve(rng.normal(0, 1, len(t)), np.ones(40) / 40, mode='same') * np.sin(np.pi * t / .35) * .5)
