'''SDL2_mixer: Chunk, Music and the Mix_* functions, on the dummy audio driver
(which runs a real audio thread, so callbacks fire off the main thread).
Skipped when the extension was built without SDL2_mixer.'''

import gc
import io
import math
import os
import struct
import subprocess
import sys
import threading
import time
import wave

import pytest

RATE = 22050


def _wav(ms=100, freq=440, rate=RATE):
    '''A mono 16-bit sine WAV file, as bytes.'''
    n = rate * ms // 1000
    samples = [int(8000 * math.sin(2 * math.pi * freq * i / rate)) for i in range(n)]
    buf = io.BytesIO()
    with wave.open(buf, 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(rate)
        w.writeframes(struct.pack('<%dh' % n, *samples))
    return buf.getvalue()


def _wait(condition, timeout=2.0):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if condition():
            return True
        time.sleep(0.01)
    return condition()


@pytest.fixture
def mixer(sdl):
    if not hasattr(sdl, 'Chunk'):
        pytest.skip('built without SDL2_mixer')
    sdl.Mix_OpenAudio(RATE, sdl.AUDIO_S16SYS, 2, 512)
    sdl.Mix_AllocateChannels(8)
    yield sdl
    for reset in (sdl.Mix_ChannelFinished, sdl.Mix_HookMusicFinished,
                  sdl.Mix_SetPostMix, sdl.Mix_HookMusic):
        reset(None)
    sdl.Mix_HaltChannel(-1)
    sdl.Mix_HaltMusic()
    sdl.Mix_CloseAudio()


def _run(sdl, code, timeout=30):
    env = dict(os.environ, PYTHONPATH=os.path.dirname(sdl.__file__))
    out = subprocess.run([sys.executable, '-c', code], env=env, capture_output=True,
                         text=True, timeout=timeout)
    assert out.returncode == 0 and out.stdout.strip().endswith('ok'), out.stderr or out.stdout


def test_versions_and_decoders(mixer):
    assert mixer.Mix_Linked_Version()[0] == 2
    chunk = [mixer.Mix_GetChunkDecoder(i) for i in range(mixer.Mix_GetNumChunkDecoders())]
    music = [mixer.Mix_GetMusicDecoder(i) for i in range(mixer.Mix_GetNumMusicDecoders())]
    assert 'WAVE' in chunk and 'WAVE' in music
    assert mixer.Mix_GetChunkDecoder(999) is None
    if hasattr(mixer, 'Mix_HasChunkDecoder'):
        assert mixer.Mix_HasChunkDecoder('WAVE') is True
        assert mixer.Mix_HasChunkDecoder('NOPE') is False
    assert isinstance(mixer.Mix_Init(), int)  # 0 flags just queries


def test_query_spec(mixer):
    frequency, fmt, channels = mixer.Mix_QuerySpec()
    assert (frequency, fmt, channels) == (RATE, mixer.AUDIO_S16SYS, 2)


def test_chunk_needs_open_audio(sdl):
    if not hasattr(sdl, 'Chunk'):
        pytest.skip('built without SDL2_mixer')
    with pytest.raises(sdl.error, match='opened'):
        sdl.Chunk(_wav())
    with pytest.raises(sdl.error):
        sdl.Mix_QuerySpec()


def test_chunk_sources_and_volume(mixer, tmp_path):
    path = tmp_path / 'beep.wav'
    path.write_bytes(_wav())
    for src in (_wav(), str(path), path):
        chunk = mixer.Chunk(src)
        assert chunk.Volume() == mixer.MIX_MAX_VOLUME
    assert chunk.Volume(32) == mixer.MIX_MAX_VOLUME
    assert chunk.Volume() == 32
    with pytest.raises(mixer.error):
        mixer.Chunk(b'not audio')
    empty = mixer.Chunk()
    with pytest.raises(mixer.error, match='not loaded'):
        empty.Volume()
    chunk.Free()
    chunk.Free()


def test_play_keeps_chunk_alive_and_finishes_off_thread(mixer):
    finished = []
    mixer.Mix_ChannelFinished(lambda channel: finished.append((channel, threading.get_ident())))
    channel = mixer.Mix_PlayChannel(-1, mixer.Chunk(_wav(50)))  # temporary: kept alive
    assert mixer.Mix_Playing(channel) == 1
    assert type(mixer.Mix_GetChunk(channel)).__name__ == 'Chunk'
    assert _wait(lambda: finished)
    assert finished[0][0] == channel
    assert finished[0][1] != threading.get_ident()  # on the audio thread


def test_quickload_raw_pins_buffer(mixer):
    data = bytearray(RATE * 4 // 20)  # 50 ms of stereo s16 silence
    chunk = mixer.Mix_QuickLoad_RAW(data)
    del data
    gc.collect()
    channel = mixer.Mix_PlayChannel(-1, chunk)
    assert _wait(lambda: mixer.Mix_Playing(channel) == 0)


def test_channel_controls(mixer):
    chunk = mixer.Chunk(_wav(500))
    channel = mixer.Mix_PlayChannel(-1, chunk, loops=-1)
    assert mixer.Mix_Volume(channel, 64) == mixer.MIX_MAX_VOLUME
    assert mixer.Mix_Volume(channel) == 64
    mixer.Mix_Pause(channel)
    assert mixer.Mix_Paused(channel) == 1
    mixer.Mix_Resume(channel)
    assert mixer.Mix_Paused(channel) == 0
    assert mixer.Mix_FadeOutChannel(channel, 200) == 1
    assert mixer.Mix_FadingChannel(channel) == mixer.MIX_FADING_OUT
    mixer.Mix_HaltChannel(channel)
    assert mixer.Mix_Playing(channel) == 0

    channel = mixer.Mix_FadeInChannel(-1, chunk, 0, 50)
    assert mixer.Mix_FadingChannel(channel) == mixer.MIX_FADING_IN
    assert mixer.Mix_ExpireChannel(channel, 20) == 1
    assert _wait(lambda: mixer.Mix_Playing(channel) == 0)


def test_no_free_channel_raises(mixer):
    mixer.Mix_AllocateChannels(1)
    chunk = mixer.Chunk(_wav(500))
    mixer.Mix_PlayChannel(-1, chunk, loops=-1)
    with pytest.raises(mixer.error):
        mixer.Mix_PlayChannel(-1, chunk)
    with pytest.raises(TypeError):
        mixer.Mix_PlayChannel(-1, 'not a chunk')


def test_groups(mixer):
    assert mixer.Mix_GroupChannels(0, 3, 7) == 4
    assert mixer.Mix_GroupCount(7) == 4
    assert mixer.Mix_GroupChannel(5, 7) is True
    assert mixer.Mix_GroupCount(7) == 5
    assert mixer.Mix_GroupAvailable(7) in range(8)
    channel = mixer.Mix_PlayChannel(0, mixer.Chunk(_wav(500)), loops=-1)
    assert mixer.Mix_GroupOldest(7) == channel == mixer.Mix_GroupNewer(7)
    assert mixer.Mix_FadeOutGroup(7, 10) >= 1
    mixer.Mix_HaltGroup(7)
    assert mixer.Mix_ReserveChannels(2) == 2
    mixer.Mix_ReserveChannels(0)


def test_positional_effects(mixer):
    mixer.Mix_SetPanning(0, 255, 64)
    mixer.Mix_SetPosition(0, 90, 10)
    mixer.Mix_SetDistance(0, 50)
    mixer.Mix_SetReverseStereo(0, True)
    mixer.Mix_UnregisterAllEffects(0)
    with pytest.raises(mixer.error):
        mixer.Mix_SetPanning(99, 255, 0)  # no such channel (255, 255 would mean "remove")
    with pytest.raises(mixer.error):
        mixer.Mix_RegisterEffect(99, lambda channel, data: None)
    mixer.Mix_RegisterEffect(0, lambda channel, data: None)  # the failed one left no residue
    mixer.Mix_UnregisterAllEffects(0)


def test_custom_effect_changes_the_mix(mixer):
    mixed = []
    mixer.Mix_SetPostMix(lambda data: mixed.append(data))
    silenced = []

    def silence(channel, data):
        silenced.append(channel)
        return bytes(len(data))

    channel = mixer.Mix_PlayChannel(-1, mixer.Chunk(_wav(500)), loops=-1)
    mixer.Mix_RegisterEffect(channel, silence)
    assert _wait(lambda: len(silenced) > 2)
    mixed.clear()
    assert _wait(lambda: len(mixed) > 2)
    assert all(not any(block) for block in mixed[1:])  # the only channel is silenced

    mixer.Mix_UnregisterEffect(channel, silence)
    mixed.clear()
    assert _wait(lambda: any(any(block) for block in mixed))  # audible again


def test_effect_done_callbacks(mixer):
    done = []
    effect = lambda channel, data: None  # noqa: E731
    channel = mixer.Mix_PlayChannel(-1, mixer.Chunk(_wav(500)), loops=-1)
    mixer.Mix_RegisterEffect(channel, effect, lambda ch: done.append(('unregistered', ch)))
    mixer.Mix_UnregisterEffect(channel, effect)
    assert done == [('unregistered', channel)]
    with pytest.raises(ValueError):
        mixer.Mix_UnregisterEffect(channel, effect)

    mixer.Mix_RegisterEffect(channel, effect, lambda ch: done.append(('stopped', ch)))
    mixer.Mix_HaltChannel(channel)  # SDL_mixer drops a channel's effects when it stops
    assert _wait(lambda: ('stopped', channel) in done)
    with pytest.raises(TypeError):
        mixer.Mix_RegisterEffect(channel, 'not callable')


def test_post_mix_and_music_hook(mixer):
    blocks = []
    mixer.Mix_SetPostMix(lambda data: blocks.append(len(data)))
    assert _wait(lambda: blocks)
    mixer.Mix_SetPostMix(None)

    requested = []

    def hook(nbytes):
        requested.append(nbytes)
        return b'\x01\x00' * (nbytes // 4)  # half a buffer; the rest stays silent

    mixer.Mix_HookMusic(hook)
    assert mixer.Mix_GetMusicHookData() is hook
    assert _wait(lambda: requested)
    mixer.Mix_HookMusic(None)
    assert mixer.Mix_GetMusicHookData() is None
    with pytest.raises(TypeError):
        mixer.Mix_SetPostMix(42)


def test_music(mixer, tmp_path):
    path = tmp_path / 'song.wav'
    path.write_bytes(_wav(300))
    for src in (str(path), path.read_bytes()):
        music = mixer.Music(src)
        assert music.GetType() == mixer.MUS_WAV
    if hasattr(music, 'Duration'):
        assert music.Duration() == pytest.approx(0.3, abs=0.01)
        assert isinstance(music.GetTitle(), str)
    with pytest.raises(mixer.error):
        mixer.Music(b'not music')
    with pytest.raises(mixer.error, match='not loaded'):
        mixer.Music().GetType()

    finished = []
    mixer.Mix_HookMusicFinished(lambda: finished.append(True))
    music.Play()
    assert mixer.Mix_PlayingMusic() == 1
    mixer.Mix_PauseMusic()
    assert mixer.Mix_PausedMusic() == 1
    mixer.Mix_ResumeMusic()
    mixer.Mix_RewindMusic()
    assert mixer.Mix_VolumeMusic(40) == mixer.MIX_MAX_VOLUME
    assert mixer.Mix_VolumeMusic() == 40
    assert _wait(lambda: finished)

    music.FadeIn(50, loops=-1)
    assert mixer.Mix_FadingMusic() == mixer.MIX_FADING_IN
    assert mixer.Mix_FadeOutMusic(20) is True
    assert _wait(lambda: mixer.Mix_PlayingMusic() == 0)


def test_music_from_bytes_outlives_source(mixer):
    data = _wav(200)
    music = mixer.Music(data)
    del data
    gc.collect()  # music streams from the bytes: the wrapper must pin them
    music.Play()
    assert _wait(lambda: mixer.Mix_PlayingMusic() == 0)


def test_playing_objects_released_on_close(mixer):
    chunk = mixer.Chunk(_wav(500))
    before = sys.getrefcount(chunk)
    mixer.Mix_PlayChannel(-1, chunk, loops=-1)
    assert sys.getrefcount(chunk) == before + 1  # held while it plays
    mixer.Mix_CloseAudio()
    assert sys.getrefcount(chunk) == before
    mixer.Mix_OpenAudio(RATE, mixer.AUDIO_S16SYS, 2, 512)  # the fixture closes it again


def test_sound_font_helpers(mixer):
    mixer.Mix_SetSoundFonts(None)
    fonts = mixer.Mix_GetSoundFonts()
    assert fonts is None or isinstance(fonts, str)
    seen = []
    mixer.Mix_EachSoundFont(lambda path: seen.append(path))
    with pytest.raises(ZeroDivisionError):
        mixer.Mix_SetSoundFonts('/nonexistent.sf2')
        mixer.Mix_EachSoundFont(lambda path: 1 / 0)
    mixer.Mix_SetSoundFonts(None)


def test_mix_quit_invalidates_music(sdl):
    if not hasattr(sdl, 'Chunk'):
        pytest.skip('built without SDL2_mixer')
    _run(sdl, f'''
import gc, io, wave, SDL2
SDL2.Init(SDL2.INIT_AUDIO)
SDL2.Mix_OpenAudio({RATE}, SDL2.AUDIO_S16SYS, 2, 512)
buf = io.BytesIO()
with wave.open(buf, "wb") as w:
    w.setnchannels(1); w.setsampwidth(2); w.setframerate({RATE}); w.writeframes(bytes(4000))
music = SDL2.Music(buf.getvalue())
SDL2.Mix_CloseAudio()
SDL2.Mix_Quit()
try:
    music.GetType()
    raise SystemExit("old music still usable after Mix_Quit")
except SDL2.error:
    pass
del music
gc.collect()                     # must not free through unloaded decoders
SDL2.Mix_OpenAudio({RATE}, SDL2.AUDIO_S16SYS, 2, 512)
assert SDL2.Music(buf.getvalue()).GetType() == SDL2.MUS_WAV
SDL2.Mix_CloseAudio()
SDL2.Quit()
print("ok")
''')


def test_callbacks_reentering_mixer_do_not_deadlock(sdl):
    # The audio thread runs callbacks with the device locked and they take the
    # GIL; the main thread hammering Mix_* must release it, or this hangs (the
    # subprocess timeout turns a hang into a failure).
    if not hasattr(sdl, 'Chunk'):
        pytest.skip('built without SDL2_mixer')
    _run(sdl, f'''
import io, wave, time, SDL2
SDL2.Init(SDL2.INIT_AUDIO)
SDL2.Mix_OpenAudio({RATE}, SDL2.AUDIO_S16SYS, 2, 256)
SDL2.Mix_AllocateChannels(32)
buf = io.BytesIO()
with wave.open(buf, "wb") as w:
    w.setnchannels(1); w.setsampwidth(2); w.setframerate({RATE}); w.writeframes(bytes(1200))
chunk = SDL2.Chunk(buf.getvalue())
finished = []
SDL2.Mix_ChannelFinished(lambda ch: (finished.append(ch), SDL2.Mix_Playing(-1)))
SDL2.Mix_RegisterEffect(SDL2.MIX_CHANNEL_POST, lambda ch, data: None)
SDL2.Mix_SetPostMix(lambda data: None)
for i in range(3000):
    try:
        SDL2.Mix_PlayChannel(-1, chunk, ticks=3)
    except SDL2.error:
        pass
    SDL2.Mix_Volume(-1, 100)
    SDL2.Mix_Playing(-1)
    if i % 100 == 0:
        SDL2.Mix_HaltChannel(-1)
time.sleep(0.1)
assert finished
SDL2.Mix_ChannelFinished(None)
SDL2.Mix_SetPostMix(None)
SDL2.Mix_UnregisterAllEffects(SDL2.MIX_CHANNEL_POST)
SDL2.Mix_CloseAudio()
SDL2.Quit()
print("ok")
''', timeout=60)
