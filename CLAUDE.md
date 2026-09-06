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

`example/` holds runnable example programs, most of which open a window and need
a display:
- `python3 example/info.py` — prints CPU/display/renderer info, no window
- `python3 example/draw.py` — primitives, blend modes, render-to-texture, geometry
- `python3 example/surface.py [out.png]` — software-surface compositing, no window
- `python3 example/keyboard.py` — keyboard / text-input / mouse event echo, cursors
- `python3 example/gamepad.py [--virtual]` — game controller / joystick monitor
- `python3 example/events.py` — dump every event, custom + cross-thread events, filter
- `python3 example/timer.py` — fixed-rate animation driven by SDL2.Timer + power state
- `python3 example/simple.py <image>` — load an image, show it, event loop
- `python3 example/audio.py` — audio + OpenGL visualizer (also needs a `pygl` module)
- `example/adjust.py` — fullscreen test pattern on every display

## Architecture

`src/pysdl.h` is the shared header: it declares every wrapper struct, its
`PyTypeObject`, the module-wide `pysdl_Error` exception, and the helpers in
`pysdl_util.c`. Every `.c` file includes only this.

- `src/pysdl.c` — module definition and `PyInit_SDL2`. Holds `Init` /
  `InitSubSystem` / `Quit`, `LoadImage`, display / GL / CPU / audio-device
  queries, timers, error / clipboard / screensaver, surface & blend-mode
  factories.
- `src/pysdl_events.c` — the event queue: `_event()` (an `SDL_Event` ->
  `(type, data)` converter; `data` is a tuple for structured events, a bare
  `str` for `TEXTINPUT`, `None` for `QUIT` and unknown types), plus
  `PollEvent` / `WaitEvent` / `PushEvent` / `PeepEvents` / filters etc. `_event`
  takes a `consume` flag — 1 frees the `drop.file` string SDL handed us, 0 when
  the event still belongs to SDL (event filter, `SDL_PEEKEVENT`).
- `src/pysdl_Window.c`, `pysdl_Renderer.c`, `pysdl_Surface.c`, `pysdl_Texture.c`,
  `pysdl_Audio.c`, `pysdl_PixelFormat.c`, `pysdl_Palette.c`, `pysdl_Cursor.c`,
  `pysdl_Joystick.c`, `pysdl_GameController.c`, `pysdl_Timer.c`, `pysdl_Haptic.c`,
  `pysdl_Sensor.c` — one wrapped SDL object per file, each a full `PyTypeObject`
  with `PySDL_<Type>_<Method>` functions.
- `src/pysdl_input.c` — module-level keyboard / mouse / touch / text-input
  functions. Its own `PyMethodDef` array (`pysdl_input_methods`) is merged into
  the module in `PyInit_SDL2` with `PyModule_AddFunctions`; `pysdl_events.c`,
  `pysdl_Cursor.c`, `pysdl_Joystick.c`, `pysdl_GameController.c`, `pysdl_Haptic.c`,
  `pysdl_Sensor.c` do the same for their functions. Use this pattern to add a
  batch of module functions from a new file.
- `src/pysdl_util.c` — `PySDL_New()` (wrapper allocation), `PySDL_ThreadEnter` /
  `PySDL_ThreadLeave` (GIL handling for SDL-owned threads), and the
  `PyToRect` / `PyToPoint` / `PyToColor` / `PyToFRect` / `PyToFPoint` /
  `PyToPixel` / `RectToPy` / `PointToPy` converters.
- `src/_constants.c` — `_constants(module)` bulk-registers ~700 SDL enum/#define
  values as module int constants. Add new constants here; some are wrapped in
  `#ifdef` for SDL version portability.

### Object model

Each wrapper struct is `PyObject_HEAD` plus one raw SDL pointer/handle (plus
tracking fields where needed: `Surface`/`Window`/`Cursor`/`Joystick`.`shouldFree`,
`Renderer.target`, `Audio.pycallback`, `Surface.pixels`). The SDL pointer is
filled in either by `tp_init` (public construction) or afterwards by the C code
that allocated the wrapper with `PySDL_New(&PySDL_X_Type)`; it is released in
`tp_dealloc` unless `shouldFree` is 0 (a *borrowed* pointer SDL still owns —
`Window.GetWindowSurface()`, `GetKeyboardFocus()`/`GetMouseFocus()` via
`PySDL_WrapWindow()`, `GetCursor()`/`GetDefaultCursor()`,
`GameController.GetJoystick()`).

`Window`, `Audio`, `Renderer`, `Texture`, `PixelFormat`, `Palette`, `Cursor`,
`Joystick`, `GameController`, `Timer`, `Haptic`, and `Sensor` are in the module
namespace and constructible: `SDL2.Window(title=None, size=…, …)`,
`SDL2.Renderer(window, …)`, `SDL2.Texture(renderer, …)`,
`SDL2.PixelFormat(format_enum)`, `SDL2.Palette(ncolors)`,
`SDL2.Cursor(system_cursor_id)`, `SDL2.Joystick(device_index)`,
`SDL2.GameController(device_index)`, `SDL2.Timer(interval_ms, callback)`,
`SDL2.Haptic(device_index)`, `SDL2.Sensor(device_index)`. Every `tp_init` takes
its primary arg as *optional* — with none given it just nulls the pointer, the
path `PySDL_New` and the C-side factory functions use. `Window`/`Renderer`/
`Texture` lean on SDL's NULL-pointer tolerance; the rest go through a
`_fmt()`/`_pal()`/`_js()`/`_gc()`/`_h()`/`_s()` guard that raises on an
uninitialised instance. `Surface` is still constructed only via
module/Window/Renderer functions.

`SDL2.Timer(interval, callback)` registers `SDL_AddTimer`; the callback runs on
SDL's timer thread through the `PySDL_ThreadEnter` trampoline and returns the
next interval (`None` = same, `0` or falsy = stop). `Timer.Remove()` — and
dropping the wrapper, which calls it from `tp_dealloc` — cancels the timer (GIL
dropped around `SDL_RemoveTimer`, which joins the callback thread).

`PySDL_Surface` carries a `shouldFree` flag: surfaces it owns (loaded images,
`CreateRGBSurface`) are `SDL_FreeSurface`d on dealloc; a borrowed surface like
`Window.GetWindowSurface()` sets `shouldFree = 0`. It also holds a `Py_buffer
pixels` (`pixels.obj == NULL` unless it is a `…SurfaceFrom` surface) that keeps
the caller's buffer alive for the surface's lifetime — SDL does not copy it.

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
  `IMG_GetError()` and return `NULL`. `pysdl_Renderer.c` / `pysdl_Texture.c` /
  `pysdl_Surface.c` have a local `_raise()` helper that does exactly this.
- Create wrapper objects with `PySDL_New(&PySDL_X_Type)`; `Py_DECREF` the wrapper
  on the SDL-failure path (it owns nothing yet, but the wrapper itself leaks).
- Method names drop `SDL_`. `Renderer` keeps the rest verbatim
  (`RenderSetViewport`, `GetRendererInfo`); `Texture` and `Surface` also drop the
  type word (`SDL_UpdateTexture` → `Update`, `SDL_SetSurfaceBlendMode` →
  `SetBlendMode`, `SDL_BlitSurface` → `Blit`) — except the pre-existing
  `Surface.LockSurface` / `UnlockSurface` / `SaveBMP`, kept for compatibility.
- Take a `PySDL_<Type> *` argument with `O!` + `&PySDL_<Type>_Type`, or check
  with `PyObject_TypeCheck` before casting — never cast an unchecked `O`.
- Rect/point/colour args go through `PyToRect` / `PyToPoint` / `PyToColor` /
  `PyToFRect` / `PyToFPoint` (tuple or list; a 2-item rect leaves `w`/`h` as
  `-1`, a 3-item colour sets `a = 255`). Check the return — they raise and
  return 0 on bad input. Pass `None` for optional rect args; guard with
  `if (arg && arg != Py_None)` since an omitted optional stays `NULL`.
- `PyToPixel(obj, format, &Uint32)` accepts an already-mapped int verbatim or
  maps an `(r,g,b[,a])` sequence through `format` — used for fill/colour-key.
- Renderer draw primitives (`DrawPoint(s)`, `DrawLine(s)`, `DrawRect(s)`,
  `FillRect(s)`) call the SDL `*F` float functions internally and accept int or
  float coordinates. `Copy`/`CopyEx` take integer rects; `CopyF`/`CopyExF` take a
  float dst rect for sub-pixel placement.
- C99 designated initializers for `PyTypeObject`; `PY_SSIZE_T_CLEAN` is set.
