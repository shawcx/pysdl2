'''The bulk constant table from src/_constants.c is present and integer-typed.'''

import pytest

SAMPLE = [
    'INIT_VIDEO', 'INIT_EVERYTHING',
    'QUIT', 'KEYDOWN', 'KEYUP', 'MOUSEMOTION', 'WINDOWEVENT', 'USEREVENT',
    'WINDOWEVENT_RESIZED',
    'WINDOW_OPENGL', 'WINDOW_FULLSCREEN', 'WINDOW_RESIZABLE',
    'RENDERER_ACCELERATED', 'RENDERER_PRESENTVSYNC',
    'FLIP_NONE', 'FLIP_HORIZONTAL',
    'K_ESCAPE', 'K_SPACE', 'K_a', 'K_UP',
    'KMOD_LSHIFT', 'KMOD_CTRL',
    'SCANCODE_A', 'SCANCODE_RETURN',
    'PIXELFORMAT_RGBA8888',
    'GL_CONTEXT_MAJOR_VERSION', 'GL_DOUBLEBUFFER',
    'AUDIO_S16', 'AUDIO_F32SYS', 'AUDIO_ALLOW_ANY_CHANGE',
    'ENABLE', 'DISABLE',
]


@pytest.mark.parametrize('name', SAMPLE)
def test_constant_is_int(sdl, name):
    assert isinstance(getattr(sdl, name), int)


def test_error_type_exposed(sdl):
    assert issubclass(sdl.error, Exception)
