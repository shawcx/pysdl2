'''Audio callback trampoline: it runs, survives a bad return value, and the
device can be reopened after Close (the (callback, userdata) tuple is released).'''

import time

import pytest


@pytest.fixture
def device_name(sdl):
    if sdl.GetNumAudioDevices() < 1:
        pytest.skip('no audio playback device')
    return sdl.GetAudioDeviceName(0)


def _run(audio, seconds=0.15):
    audio.Pause(0)
    time.sleep(seconds)
    audio.Pause(1)


def test_playback_callback_is_invoked(sdl, device_name):
    calls = []

    def callback(size, userdata):
        calls.append((size, userdata))
        return b'\x00' * size

    audio = sdl.Audio()
    freq, fmt, channels, samples = audio.Open(
        device_name, freq=48000, channels=1, samples=1024, callback=callback, userdata='tag')
    assert freq == 48000
    _run(audio)
    audio.Close()

    assert calls, 'callback never fired'
    assert calls[0][1] == 'tag'


def test_reopen_after_close(sdl, device_name):
    audio = sdl.Audio()
    audio.Open(device_name, freq=48000, channels=1, samples=512, callback=lambda s, u: b'\x00' * s)
    audio.Close()
    # Would raise "Audio device already open" if Close failed to reset state.
    audio.Open(device_name, freq=48000, channels=1, samples=512, callback=lambda s, u: b'\x00' * s)
    audio.Close()


def test_bad_callback_return_does_not_crash(sdl, device_name, capfd):
    audio = sdl.Audio()
    audio.Open(device_name, freq=48000, channels=1, samples=512, callback=lambda s, u: 'not bytes')
    _run(audio, 0.1)
    audio.Close()
    # the trampoline reports the error on stderr instead of propagating it
    assert 'must return bytes' in capfd.readouterr().err
