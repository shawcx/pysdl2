'''Shared pytest setup: run headless, import the freshly built extension.'''

import glob
import os
import pathlib
import subprocess
import sys
import sysconfig

import pytest

# Drivers that need no display, sound card, or window server.
os.environ.setdefault('SDL_VIDEODRIVER', 'dummy')
os.environ.setdefault('SDL_AUDIODRIVER', 'dummy')

_ROOT = pathlib.Path(__file__).resolve().parent.parent


def _built_module_dir():
    '''The build/lib* dir holding an extension for *this* interpreter.'''
    suffix = sysconfig.get_config_var('EXT_SUFFIX')
    for path in sorted(glob.glob(str(_ROOT / 'build' / 'lib*'))):
        if os.path.exists(os.path.join(path, 'SDL2' + suffix)):
            return path
    return None


# Always build: setup.py only recompiles when a source changed, and skipping
# this would silently test a stale .so after editing the C code.
subprocess.run([sys.executable, 'setup.py', '-q', 'build'], cwd=_ROOT, check=True,
               stdout=subprocess.DEVNULL)
_moddir = _built_module_dir()

if _moddir is not None and _moddir not in sys.path:
    sys.path.insert(0, _moddir)


@pytest.fixture(scope='session')
def sdl():
    '''SDL initialised once for the whole test session.'''
    import SDL2
    SDL2.Init(SDL2.INIT_VIDEO | SDL2.INIT_AUDIO | SDL2.INIT_EVENTS | SDL2.INIT_TIMER)
    yield SDL2
    SDL2.Quit()


@pytest.fixture
def window(sdl):
    return sdl.Window('pysdl2-test', (64, 48))


@pytest.fixture
def renderer(sdl, window):
    try:
        return window.CreateRenderer()
    except sdl.error:
        pytest.skip('no renderer available under this driver')


@pytest.fixture
def texture(sdl, renderer):
    return renderer.CreateTextureFromSurface(sdl.CreateRGBSurface((8, 8)))
