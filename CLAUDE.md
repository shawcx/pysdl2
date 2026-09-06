# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

A CPython C extension that exposes SDL2 (plus SDL2_image) to Python 3 as a single
module named `SDL2`. It is a thin, hand-written binding: only the subset of SDL
the author needed is wrapped, and wrapped functions/methods drop the `SDL_`
prefix (`SDL_GetPlatform` → `SDL2.GetPlatform`, `SDL_RenderPresent` →
`Renderer.Present`).

`docs/ROADMAP.md` is the phased plan for expanding coverage toward the full SDL2
API in this same style — consult it before adding a new subsystem.

## Build & install

System dependencies must be present first:
- macOS: `brew install sdl2 sdl2_image`
- Debian/Ubuntu: `apt install libsdl2-dev libsdl2-image-dev`

Then:
- `pip install .` — build and install the extension
- `python3 setup.py build` — compile in place under `build/` without installing
- `python3 setup.py sdist` — source tarball; `MANIFEST.in` bundles `src/` so the
  Debian package build (`stdeb`/`deb_dist`, see `.gitignore`) can compile from it

`setup.py` asks `sdl2-config` / `pkg-config` where SDL2 is and always appends
`/usr/local` and `/opt/homebrew` as a fallback. All of `src/*.c` is globbed into
one `Extension`, so a new `.c` file is picked up automatically.

`tools/gen_constants.py` reports SDL enum/#define symbols not yet in
`src/_constants.c` (`--emit <substr>` prints pasteable `PyModule_AddIntConstant`
lines). Run it when wiring up a new subsystem.

## Tests

`tests/` is a headless pytest suite (`pip install pytest`, then `python3 -m
pytest`). `tests/conftest.py` forces the `dummy` video/audio drivers, builds the
extension if `build/` is missing, and puts it on `sys.path`; the `sdl` fixture
does one `Init`/`Quit` per session. Run one file with `python3 -m pytest
tests/test_audio.py`.

`test/` (singular) holds runnable example programs, most of which open a window
and need a display:
- `python3 test/info.py` — prints CPU/display/renderer info, no window
- `python3 test/simple.py <image>` — load an image, show it, event loop
- `python3 test/audio.py` — audio + OpenGL visualizer (also needs a `pygl` module)
- `test/adjust.py` — fullscreen test pattern on every display

## Architecture

`src/pysdl.h` is the shared header: it declares every wrapper struct, its
`PyTypeObject`, the module-wide `pysdl_Error` exception, and the helpers in
`pysdl_util.c`. Every `.c` file includes only this.

- `src/pysdl.c` — module definition and `PyInit_SDL2`. Holds all module-level
  functions (`Init`, `PollEvent`, `LoadImage`, display/GL/CPU/audio-device
  queries, timers, error/clipboard/screensaver) and the `_event()` helper that
  flattens an `SDL_Event` into a `(type, data_tuple)` pair for
  `PollEvent`/`WaitEvent`.
- `src/pysdl_Window.c`, `pysdl_Renderer.c`, `pysdl_Surface.c`,
  `pysdl_Texture.c`, `pysdl_Audio.c` — one wrapped SDL object per file, each a
  full `PyTypeObject` with `PySDL_<Type>_<Method>` functions.
- `src/pysdl_util.c` — `PySDL_New()` (wrapper allocation), `PySDL_ThreadEnter` /
  `PySDL_ThreadLeave` (GIL handling for SDL-owned threads), and the
  `PyToRect` / `PyToPoint` / `PyToColor` / `RectToPy` / `PointToPy` converters.
- `src/_constants.c` — `_constants(module)` bulk-registers ~700 SDL enum/#define
  values as module int constants. Add new constants here; some are wrapped in
  `#ifdef` for SDL version portability.

### Object model

Each wrapper struct is `PyObject_HEAD` plus one raw SDL pointer/handle. The
pointer is acquired lazily (not in `tp_init`, which just nulls it) and released
in `tp_dealloc`. Internally, code creates a wrapper with
`PySDL_New(&PySDL_X_Type)` and then assigns the SDL pointer.

Only `Window` and `Audio` are added to the module namespace. `Renderer`,
`Surface`, and `Texture` have no public constructor — they are only returned
from methods (`Window.CreateRenderer()`, `SDL2.LoadImage()`,
`Renderer.CreateTextureFromSurface()`, etc.).

`PySDL_Surface` carries a `shouldFree` flag: surfaces it owns (loaded images,
`CreateRGBSurface`) are `SDL_FreeSurface`d on dealloc; a borrowed surface like
`Window.GetWindowSurface()` sets `shouldFree = 0`.

### Threading

Blocking SDL calls (`SDL_LoadBMP`, `IMG_Load`, `SDL_WaitEvent`,
`SDL_GL_SwapWindow`, `SDL_Delay`) are wrapped in `Py_BEGIN_ALLOW_THREADS` /
`Py_END_ALLOW_THREADS`. This is **mandatory** for any SDL call that can block on
the audio callback thread (`SDL_CloseAudioDevice`, `SDL_LockAudioDevice`,
`SDL_PauseAudioDevice`): that thread needs the GIL, so holding it here deadlocks.

Audio uses SDL's callback thread. `Audio.Open(..., callback=fn, userdata=obj)`
stores `(fn, userdata)` as a tuple in both `SDL_AudioSpec.userdata` and the
wrapper's `pycallback` field (released in `Close`/dealloc — SDL holds no other
reference). The C trampolines `_playback_callback` / `_capture_callback` go
through `PySDL_ThreadEnter` (which bails if `Py_IsFinalizing()`), call the Python
callable, and report any exception with `PyErr_Print()`. A playback callback
returning a non-`bytes` or short buffer yields silence, not a crash.

New off-main-thread callbacks (timers, event filters) must follow the same
`PySDL_ThreadEnter` / `PySDL_ThreadLeave` pattern.

### Conventions

- Errors: set `pysdl_Error` (exposed as `SDL2.error`) with `SDL_GetError()` /
  `IMG_GetError()` and return `NULL`.
- Create wrapper objects with `PySDL_New(&PySDL_X_Type)`.
- Rect/point/colour args go through `PyToRect` / `PyToPoint` / `PyToColor`
  (tuple or list; a 2-item rect leaves `w`/`h` as `-1`, a 3-item colour sets
  `a = 255`). Check the return — they raise and return 0 on bad input. Pass
  `None` for optional `src`/`dst` rect arguments; guard with
  `if (arg && arg != Py_None)` since an omitted optional stays `NULL`.
- C99 designated initializers for `PyTypeObject`; `PY_SSIZE_T_CLEAN` is set.
