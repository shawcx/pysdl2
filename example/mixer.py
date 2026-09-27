#!/usr/bin/env python3

# SDL2_mixer tour, no window and no sound files needed: synthesises a few
# samples, plays them on separate channels with panning, runs a looping
# "music" track under them with a custom tremolo effect, and prints a live
# level meter from a post-mix callback.
#
#   python3 example/mixer.py [--seconds N] [file-to-play-as-music]
#
# Needs the extension built with SDL2_mixer (libsdl2-mixer-dev / brew sdl2_mixer).

import argparse
import array
import io
import math
import sys
import time
import wave
import SDL2

if not hasattr(SDL2, 'Chunk'):
    sys.exit('this SDL2 build has no SDL2_mixer support')

parser = argparse.ArgumentParser(description='SDL2_mixer tour')
parser.add_argument('music', nargs='?', help='a file to play as the music track')
parser.add_argument('--seconds', type=float, default=6.0, help='how long to run')
args = parser.parse_args()

RATE = 44100


def tone(freqs, ms, volume=0.3):
    '''A mono 16-bit WAV (bytes) of the given frequencies, with a short fade-out.'''
    n = RATE * ms // 1000
    samples = array.array('h', (
        int(32767 * volume * min(1.0, (n - i) / 2000) *
            sum(math.sin(2 * math.pi * f * i / RATE) for f in freqs) / len(freqs))
        for i in range(n)))
    buf = io.BytesIO()
    with wave.open(buf, 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(samples.tobytes())
    return buf.getvalue()


SDL2.Init(SDL2.INIT_AUDIO)
print('SDL_mixer', '.'.join(map(str, SDL2.Mix_Linked_Version())))
SDL2.Mix_Init(0)
SDL2.Mix_OpenAudio(RATE, SDL2.AUDIO_S16SYS, 2, 1024)
print('device', SDL2.Mix_QuerySpec(),
      '| music decoders:', ', '.join(SDL2.Mix_GetMusicDecoder(i) for i in range(SDL2.Mix_GetNumMusicDecoders())))
SDL2.Mix_AllocateChannels(8)

blip = SDL2.Chunk(tone([880], 120))
chord = SDL2.Chunk(tone([261.6, 329.6, 392.0], 600, volume=0.2))
bass = tone([55, 110], 2000, volume=0.25)
music = SDL2.Music(args.music) if args.music else SDL2.Music(bass)
print('music type', music.GetType(), 'duration', music.Duration() if hasattr(music, 'Duration') else '?')

# Channel-finished callbacks arrive on SDL's audio thread.
finished = []
SDL2.Mix_ChannelFinished(lambda channel: finished.append(channel))

# A custom effect on the final mix: slow tremolo. Effects get the buffer as
# bytes and return replacement bytes of the same length.
phase = [0.0]


def tremolo(channel, data):
    samples = array.array('h', data)
    for i in range(0, len(samples), 2):
        gain = 0.65 + 0.35 * math.sin(phase[0])
        phase[0] += 2 * math.pi * 4 / RATE
        samples[i] = int(samples[i] * gain)
        samples[i + 1] = int(samples[i + 1] * gain)
    return samples.tobytes()


SDL2.Mix_RegisterEffect(SDL2.MIX_CHANNEL_POST, tremolo)

# A post-mix callback only observes here (returning None leaves the audio alone).
level = [0]


def meter(data):
    samples = array.array('h', data)
    level[0] = max(abs(s) for s in samples[::16]) if samples else 0


SDL2.Mix_SetPostMix(meter)

music.FadeIn(500, loops=-1)
SDL2.Mix_VolumeMusic(80)

start = time.monotonic()
beat = 0
while time.monotonic() - start < args.seconds:
    t = time.monotonic() - start
    if t > beat * 0.5:
        channel = SDL2.Mix_PlayChannel(-1, chord if beat % 4 == 0 else blip)
        pan = int(127 + 127 * math.sin(beat))  # sweep left <-> right
        SDL2.Mix_SetPanning(channel, 255 - pan, pan)
        beat += 1
    bar = '#' * (level[0] * 40 // 32768)
    print(f'\r{t:4.1f}s  beat {beat:2d}  playing {SDL2.Mix_Playing(-1)}  level |{bar:<40}|',
          end='', flush=True)
    time.sleep(0.05)
print()

SDL2.Mix_FadeOutMusic(300)
time.sleep(0.4)
print(f'{len(finished)} channel(s) finished during the run')

SDL2.Mix_SetPostMix(None)
SDL2.Mix_ChannelFinished(None)
SDL2.Mix_UnregisterAllEffects(SDL2.MIX_CHANNEL_POST)
SDL2.Mix_CloseAudio()
SDL2.Mix_Quit()
SDL2.Quit()
