'''Phase 0 additions: timers, error state, paths, screensaver, clipboard.'''

import pytest

NEW_FUNCTIONS = [
    'Delay', 'GetPerformanceCounter', 'GetPerformanceFrequency',
    'GetVersion', 'GetRevision', 'SetError', 'ClearError',
    'GetBasePath', 'GetPrefPath',
    'EnableScreenSaver', 'DisableScreenSaver', 'IsScreenSaverEnabled',
    'GetClipboardText', 'SetClipboardText', 'HasClipboardText',
]


def test_new_functions_present(sdl):
    missing = [name for name in NEW_FUNCTIONS if not hasattr(sdl, name)]
    assert not missing


def test_delay_and_timers(sdl):
    start = sdl.GetTicks()
    sdl.Delay(20)
    assert sdl.GetTicks() - start >= 10

    assert sdl.GetPerformanceFrequency() > 0
    first = sdl.GetPerformanceCounter()
    assert sdl.GetPerformanceCounter() >= first

    if hasattr(sdl, 'GetTicks64'):
        assert sdl.GetTicks64() >= start


def test_error_state_roundtrip(sdl):
    sdl.SetError('phase zero')
    assert sdl.GetError() == 'phase zero'
    sdl.ClearError()
    assert sdl.GetError() == ''


def test_paths(sdl):
    assert sdl.GetBasePath()
    pref = sdl.GetPrefPath('pysdl2', 'tests')
    assert pref and pref.endswith(('/', '\\'))


def test_screensaver_toggle(sdl):
    sdl.DisableScreenSaver()
    assert sdl.IsScreenSaverEnabled() is False
    sdl.EnableScreenSaver()
    assert sdl.IsScreenSaverEnabled() is True


def test_clipboard_roundtrip(sdl):
    try:
        sdl.SetClipboardText('phase zero clipboard')
    except sdl.error:
        pytest.skip('clipboard unavailable under this driver')
    assert sdl.HasClipboardText() is True
    assert sdl.GetClipboardText() == 'phase zero clipboard'
