# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

A CPython C extension that exposes SDL2 (plus SDL2_image, and SDL2_ttf /
SDL2_mixer when available) to Python 3 as a single module named `SDL2`. It is a thin,
hand-written binding that covers essentially the whole SDL2 API (deliberate
omissions are listed in `docs/ROADMAP.md`); wrapped functions/methods drop the
`SDL_` prefix (`SDL_GetPlatform` → `SDL2.GetPlatform`, `SDL_RenderPresent` →
`Renderer.Present`).

`docs/ROADMAP.md` records the phased build-out, what was deliberately left
unwrapped and why, and the pattern for adding another satellite library —
consult it before adding a new subsystem.

## Build & install

System dependencies must be present first:
- macOS: `brew install sdl2 sdl2_image` (+ `sdl2_ttf`, `sdl2_mixer`)
- Debian/Ubuntu: `apt install libsdl2-dev libsdl2-image-dev` (+ `libsdl2-ttf-dev`,
  `libsdl2-mixer-dev`)

SDL2_ttf and SDL2_mixer are **optional**: `setup.py`'s `OPTIONAL` table probes
each (`pkg-config`, else the header) and, when found, links it and defines
`PYSDL_HAVE_TTF` / `PYSDL_HAVE_MIXER`, which compile in `SDL2.Font` / `TTF_*`
and `SDL2.Chunk` / `SDL2.Music` / `Mix_*`. `PYSDL_TTF=0` / `PYSDL_MIXER=0`
force a build without one. Everything for a satellite library is inside its
`#ifdef`; add another by extending `OPTIONAL`.

Then:
- `pip install .` — build and install the extension
- `python3 setup.py build` — compile in place under `build/` without installing
- `python3 setup.py sdist` — source tarball; `MANIFEST.in` bundles `src/` so the
  Debian package build can compile from it
- `make` — build a `.deb` into `deb_dist/` via stdeb (`pip install 'stdeb>=0.11'`:
  Ubuntu 24.04's packaged 0.10 breaks on Python 3.12). `stdeb.cfg` sets the
  maintainer and Build-Depends (all four SDL dev packages, so the package always
  has SDL2_ttf / SDL2_mixer); runtime Depends come from `${shlibs:Depends}`.

`setup.py` asks `sdl2-config` / `pkg-config` where SDL2 is and always appends
`/usr/local` and `/opt/homebrew` as a fallback. All of `src/*.c` is globbed into
one `Extension`, so a new `.c` file is picked up automatically.

`tools/check_version_guards.py` checks that every SDL symbol used inside an
`#if *_VERSION_ATLEAST(x,y,z)` block exists in release x.y.z (headers are
fetched from GitHub and cached in `~/.cache/pysdl2-headers`). Run it after adding
guarded code: a guard that is too early compiles on newer SDL and on older SDL
alike, and only breaks on the releases in between. CI runs it.

`tools/gen_constants.py` reports SDL enum/#define symbols not yet in
`src/_constants.c` (`--emit <substr>` prints pasteable `PyModule_AddIntConstant`
lines). Run it when wiring up a new subsystem.

## Tests

`tests/` is a headless pytest suite (`pip install pytest`, then `python3 -m
pytest`). Python 3.10+ is required (the C code uses `Py_NewRef`). CI
(`.github/workflows/tests.yml`) runs it on Ubuntu 22.04 (the oldest stack:
SDL 2.0.20, SDL_image 2.0.5, SDL_ttf 2.0.18, SDL_mixer 2.0.4, Python 3.10),
Ubuntu 24.04 (Python 3.12 / 3.13), without the optional libraries, and on macOS
(Homebrew), plus the version-guard check and a `.deb` build. Tests that depend on
behaviour that changed between SDL releases must accept both (see the touchpad,
window-grab and `Measure` tests).

**sdl2-compat**: Homebrew's `sdl2` (and newer distros) is now sdl2-compat, the
SDL2 API on SDL3; it reports version 2.32.50+ (`_is_sdl2_compat` in
`tests/test_joystick.py`). Known differences the tests allow for: virtual-joystick
callback results come back inverted (an sdl2-compat bug — its wrappers return the
SDL2 `int` as SDL3's `bool`), and closing a rumbling joystick sends no final
`rumble(0, 0)`. To reproduce on Linux, build SDL3 3.4 (`-DSDL_UNIX_CONSOLE_BUILD=ON`
for a headless build) and sdl2-compat from source and put its `lib/` on
`LD_LIBRARY_PATH`. The joystick tests also set
`SDL_JOYSTICK_ALLOW_BACKGROUND_EVENTS`: SDL drops joystick input while windows
exist but none has focus, and SDL 2.0.20's dummy driver never gives focus. `tests/conftest.py` forces the `dummy` video/audio drivers, runs
`setup.py build` (a no-op when nothing changed) and puts the `build/lib*` dir for
the running interpreter on `sys.path`; the `sdl` fixture
does one `Init`/`Quit` per session. Run one file with `python3 -m pytest
tests/test_audio.py`. `tests/test_mixer.py` skips without SDL2_mixer; it runs on
the dummy audio driver (a real audio thread) and polls with deadlines instead of
fixed sleeps. `tests/test_ttf.py` skips itself when the build has no
SDL2_ttf or no TrueType font can be found (it tries DejaVu, Arial, then
`fc-match`); its `ttf` fixture does `TTF_Init` / `TTF_Quit` per test.

`example/` holds runnable example programs, most of which open a window and need
a display:
- `python3 example/info.py` — prints CPU/display/renderer info, no window
- `python3 example/draw.py` — primitives, blend modes, render-to-texture, geometry
- `python3 example/surface.py [out.png]` — software-surface compositing, no window
- `python3 example/keyboard.py` — keyboard / text-input / mouse event echo, cursors
- `python3 example/gamepad.py [--virtual]` — game controller / joystick monitor
- `python3 example/events.py` — dump every event, custom + cross-thread events, filter
- `python3 example/timer.py` — fixed-rate animation driven by SDL2.Timer + power state
- `python3 example/window.py` — window-state playground: border/grab/opacity/flash, message box, display + Vulkan info
- `python3 example/wav.py [file.wav]` — LoadWAV + AudioStream resample + queue playback
- `python3 example/rects.py` — live rect intersection / union / enclose / line-clip
- `python3 example/image.py [file]` — SDL_image: format probes, animations, SVG / XPM, encode to bytes; no window
- `python3 example/mixer.py [--seconds N] [music-file]` — SDL2_mixer: synthesised chunks, panning, music, custom effect, level meter; no window
- `python3 example/text.py [--font PATH] [--frames N]` — SDL2_ttf render modes, styles, wrapped text, metrics
- `python3 example/logical.py [--frames N]` — logical-size canvas: mouse picking, RenderGeometryRaw, LockToSurface, vsync toggle
- `python3 example/simple.py <image>` — load an image, show it, event loop
- `python3 example/audio.py` — audio callback + OpenGL visualizer (also needs a `pygl` module)
- `example/adjust.py` — fullscreen test pattern on every display

## Architecture

`src/pysdl.h` is the shared header: it declares every wrapper struct, its
`PyTypeObject`, the module-wide `pysdl_Error` exception, and the shared
helpers (mostly in `pysdl_util.c`). Every `.c` file includes only this.

- `src/pysdl.c` — module definition and `PyInit_SDL2`. Holds `Init` /
  `InitSubSystem` / `Quit`, `LoadBMP`, display / GL / CPU / audio-device
  queries, timers, error / clipboard / screensaver, surface & blend-mode
  factories. Also defines `PySDL_New()` (wrapper allocation).
- `src/pysdl_Events.c` — the event queue: `_event()` (an `SDL_Event` ->
  `(type, data)` converter; `data` is a tuple for structured events, a bare
  `str` for `TEXTINPUT`, `None` for `QUIT` and unknown types), plus
  `PollEvent` / `WaitEvent` / `PushEvent` / `PeepEvents` / filters etc. `_event`
  takes a `consume` flag — 1 frees the `drop.file` string SDL handed us, 0 when
  the event still belongs to SDL (event filter, `SDL_PEEKEVENT`).
- `src/pysdl_Window.c`, `pysdl_Renderer.c`, `pysdl_Surface.c`, `pysdl_Texture.c`,
  `pysdl_Audio.c`, `pysdl_AudioStream.c`, `pysdl_PixelFormat.c`, `pysdl_Palette.c`,
  `pysdl_Cursor.c`, `pysdl_Joystick.c`, `pysdl_GameController.c`, `pysdl_Timer.c`,
  `pysdl_Haptic.c`, `pysdl_Sensor.c`, `pysdl_Font.c` — one wrapped SDL object per
  file, each a full `PyTypeObject` with `PySDL_<Type>_<Method>` functions.
  `pysdl_Font.c` (SDL2_ttf) also holds the `TTF_*` module functions
  (`pysdl_ttf_methods`, registered only under `PYSDL_HAVE_TTF`).
- `src/pysdl_Mixer.c` (SDL2_mixer, under `PYSDL_HAVE_MIXER`) — one file for the
  whole library: the `Chunk` and `Music` types and the `Mix_*` module functions
  (`pysdl_mixer_methods`). SDL functions whose first argument is a chunk / music
  are methods (`Mix_VolumeChunk` → `Chunk.Volume`, `Mix_PlayMusic` →
  `Music.Play`, `Mix_GetMusicTitle` → `Music.GetTitle`); the rest keep `Mix_`.
- `src/pysdl_Input.c` — module-level keyboard / mouse / touch / text-input
  functions. Its own `PyMethodDef` array (`pysdl_input_methods`) is merged into
  the module in `PyInit_SDL2` with `PyModule_AddFunctions`; `pysdl_Events.c`,
  `pysdl_Video.c` (extra display queries, message boxes, hints, `OpenURL` /
  locales, GL / Vulkan loaders, `GetWindowFromID` / `GetGrabbedWindow`),
  `pysdl_Audio.c` (drivers, `LoadWAV`, `MixAudioFormat`, device-spec queries),
  `pysdl_Rect.c` (rect / point geometry), `pysdl_Image.c` (SDL_image:
  `IMG_Init` / `IMG_Quit`, `LoadImage`, `IMG_is*`, `IMG_LoadAnimation`, SVG /
  XPM), `pysdl_Cursor.c`, `pysdl_Joystick.c`,
  `pysdl_GameController.c`, `pysdl_Haptic.c`, `pysdl_Sensor.c` do the same for
  their functions. Use this pattern to add a batch of module functions from a
  new file.
- Joystick GUIDs are 32-char hex strings in Python; `GUIDToPy` / `PyToGUID`
  (in `pysdl_Joystick.c`, declared in `pysdl.h`) convert, and `PyToGUID` also
  takes the raw 16 bytes.
- `src/pysdl_util.c` — `PySDL_ThreadEnter` /
  `PySDL_ThreadLeave` (GIL handling for SDL-owned threads), and the
  `PyToRect` / `PyToPoint` / `PyToColor` / `PyToFRect` / `PyToFPoint` /
  `PyToPixel` / `RectToPy` / `PointToPy` converters, plus the RWops helpers:
  `PySDL_RWFromObject` (a str / os.PathLike path or any bytes-like -> readable
  RWops; release the returned `Py_buffer` after closing it) and `PySDL_RWBuffer`
  / `PySDL_RWBufferBytes` (a growable write RWops -> `bytes`).
- `src/_constants.c` — `_constants(module)` bulk-registers ~900 SDL enum/#define
  values as module int constants. Add new constants here. Portability guard:
  `#ifdef SDL_FOO` works **only** for `#define`d names — for an *enum member*
  (most `SDL_WINDOW_*`, `SDL_BLENDMODE_*`, event types, …) `#ifdef` is always
  false and silently drops the constant, so gate those with `#if
  SDL_VERSION_ATLEAST(x,y,z)` at the version they were introduced.

### Object model

Each wrapper struct is `PyObject_HEAD` plus one raw SDL pointer/handle (plus
tracking fields where needed: `Surface`/`Window`/`Renderer`/`Cursor`.`shouldFree`,
`Renderer.target`, `Texture.locked`, `Texture.renderer`, `Audio.pycallback`,
`Surface.pixels`, `Surface.window_id`). The SDL pointer is
filled in either by `tp_init` (public construction) or afterwards by the C code
that allocated the wrapper with `PySDL_New(&PySDL_X_Type)`; it is released in
`tp_dealloc` unless `shouldFree` is 0 (a *borrowed* pointer SDL still owns —
`Window.GetWindowSurface()`, `GetKeyboardFocus()`/`GetMouseFocus()` via
`PySDL_WrapWindow()`, `GetDefaultCursor()`, `Window.GetRenderer()`,
`Texture.LockToSurface()`). `GetCursor()` returns the owning `Cursor` object when
the current cursor is one of ours, so only SDL's default cursor is ever borrowed. A borrowed pointer SDL frees while Python may still
hold it gets swapped out rather than left dangling: `PySDL_SurfaceDetach()`
points a Surface at a fresh empty 0x0 surface (without freeing the old one),
since Surface methods dereference `->surface` without checks. `Texture.Unlock`
(and the texture's dealloc) does this for the `LockToSurface` Surface.

**Window invalidation** (`pysdl_Window.c`, built on `PySDL_Registry` in
`pysdl_util.c` — a list of *borrowed* wrapper pointers each wrapper adds itself
to and removes in `tp_dealloc`):
- Every `Window` wrapper, owned or borrowed, is registered in `tp_init`.
- `GetWindowSurface()` registers the Surface it returns, keyed by window id
  (`Surface.window_id`), so every wrapper of a window shares one Surface object.
  SDL frees a window surface on the next `SDL_GetWindowSurface` after a resize,
  on `SDL_DestroyWindowSurface` and with the window; each of those paths empties
  the registered Surface first (`_drop_surface`). A replacement that happens to
  get the same address keeps the same Surface object (the pointer is valid
  again), so tests must not assume a new object after a resize.
- The owning `Window`'s dealloc nulls every other wrapper of that window
  (`_forget_window`) before `SDL_DestroyWindow`.
- `Quit`, `VideoQuit`, `VideoInit` (which restarts video) and `QuitSubSystem`
  (only when video actually stopped — subsystems are refcounted) call
  `PySDL_InvalidateWindows()`: every Window's pointer and GL context are nulled
  (methods then raise "Invalid window" / "Video subsystem has not been
  initialized") and every window Surface is emptied. Nothing here touches an
  `SDL_Window` that may already be freed.
- Cursors: every `Cursor` wrapper registers in `tp_init` (`pysdl_Cursor.c`).
  An owning Cursor's dealloc nulls any other wrapper of the same cursor before
  `SDL_FreeCursor`, and `PySDL_InvalidateWindows()` also calls
  `PySDL_InvalidateCursors()`, because video shutdown frees every cursor —
  including ones we own, which must then not be freed again. A NULL cursor
  raises in `Cursor.Set` / `SetCursor` (for SDL, `SDL_SetCursor(NULL)` means
  "redraw").
- Renderers and textures are **not** freed by video shutdown (SDL keeps them
  until `SDL_DestroyRenderer`), so they need no invalidation for `Quit`.

**Renderer / texture invalidation** (`pysdl_Renderer.c`): `SDL_DestroyRenderer`
frees every texture of that renderer. Every `Renderer` wrapper registers in
`tp_init`; every Texture wrapper records its owning `SDL_Renderer` and registers
via `PySDL_TextureTrack()` at each creation site (`Texture(...)`,
`CreateTextureFromSurface`, `LoadTexture` — a new site must call it too). The
owning Renderer's dealloc invalidates that renderer's textures
(`PySDL_TextureInvalidate`: pointer nulled, `LockToSurface` Surface emptied) and
its borrowed wrappers before `SDL_DestroyRenderer`. SDL calls that read a NULL
texture as "none" (`SetRenderTarget`, `RenderGeometry`, `RenderGeometryRaw`)
take Texture args through `_texture_arg()`, which raises instead.
Joystick/controller lookups
(`JoystickFromInstanceID`, `GameControllerFromPlayerIndex`,
`GameController.GetJoystick()`, …) avoid borrowing:
they re-open the device by index (`PySDL_JoystickIndexForInstance`), which bumps
SDL's refcount and yields an owned wrapper.

`Window`, `Audio`, `AudioStream`, `Renderer`, `Texture`, `PixelFormat`,
`Palette`, `Cursor`, `Joystick`, `GameController`, `Timer`, `Haptic`, and
`Sensor` are in the module namespace and constructible: `SDL2.Window(title=None,
size=…, …)`, `SDL2.Renderer(window, …)`, `SDL2.Texture(renderer, …)`,
`SDL2.PixelFormat(format_enum)`, `SDL2.Palette(ncolors)`,
`SDL2.Cursor(system_cursor_id)`, `SDL2.Joystick(device_index)`,
`SDL2.GameController(device_index)`, `SDL2.Timer(interval_ms, callback)`,
`SDL2.Haptic(device_index)`, `SDL2.Sensor(device_index)`,
`SDL2.AudioStream(src_format, src_channels, src_rate, dst_…)`, and with SDL2_ttf
`SDL2.Font(path_or_bytes, ptsize, index=0, *, hdpi, vdpi)`. Every `tp_init` takes
its primary arg as *optional* — with none given it just nulls the pointer, the
path `PySDL_New` and the C-side factory functions use. `Window`/`Renderer`/
`Texture` lean on SDL's NULL-pointer tolerance; the rest go through a
`_fmt()`/`_pal()`/`_js()`/`_gc()`/`_h()`/`_s()`/`_font()` guard that raises on an
uninitialised instance.

`Chunk` / `Music` (SDL2_mixer): a playing chunk or music is kept alive by the
binding (`_channel_chunks`, one strong reference per channel, replaced when the
channel is reused; `_playing_music`), both released when audio is fully closed —
so `Mix_PlayChannel(-1, Chunk(...))` with a temporary doesn't cut out.
bytes-backed `Music` and `Mix_QuickLoad_RAW` chunks pin their buffer (music
streams from it). `Mix_Quit` can unload decoder libraries, so `Music` carries a
`session` like `Font`: after `Mix_Quit` it raises and is leaked, not freed.
`Mix_QuickLoad_WAV` is deliberately absent — it takes no length and trusts the
WAV header.

`Font` keeps two extra fields: `data`, a `Py_buffer` pinning the font file's
bytes (SDL_ttf reads the RWops lazily for the font's whole life, so the buffer is
released only after `TTF_CloseFont`), and `session`. `TTF_CloseFont` after the
last `TTF_Quit` touches freed FreeType state, so the `TTF_Quit` wrapper advances
a module-level session counter when SDL_ttf actually shuts down; a font from an
older session raises on use and is dropped without being closed. `Surface` is still constructed only via
module/Window/Renderer functions.

`SDL2.Timer(interval, callback)` registers `SDL_AddTimer`; the callback runs on
SDL's timer thread through the `PySDL_ThreadEnter` trampoline and returns the
next interval (`None` = same, `0` or falsy = stop). `Timer.Remove()` — and
dropping the wrapper, which calls it from `tp_dealloc` — cancels the timer.
`SDL_RemoveTimer` does **not** wait for a callback already under way (it only
marks the timer cancelled), so SDL's `param` is an integer token, never a
Python object: the callback looks the callable up in `_timers` (token →
callable) under the GIL, and `Remove()` deletes the entry first. A callback
still pending then finds nothing and stops. Any future callback whose SDL
removal doesn't synchronise with in-flight calls needs the same treatment.

`PySDL_Surface` carries a `shouldFree` flag: surfaces it owns (loaded images,
`CreateRGBSurface`) are `SDL_FreeSurface`d on dealloc; a borrowed surface like
`Window.GetWindowSurface()` sets `shouldFree = 0` (and is tracked, see above). It also holds a `Py_buffer
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

The same holds for SDL's **event-watcher lock**: the event filter and event
watchers run under it, on whichever thread pushed the event, and then take the
GIL. So any call that pushes or pumps events or (un)registers a filter / watch
(`PollEvent`, `PumpEvents`, `PushEvent`, `QuitRequested`, `ResetKeyboard`,
`SetEventFilter`, `Add/DelEventWatch`) drops the GIL around the SDL call;
otherwise a watcher fired from an SDL thread (audio, hotplug, timer) deadlocks
against a Python thread waiting on that lock. SDL functions that push events
only as a side effect (most window calls) still hold the GIL; that residual
risk needs a filter/watch installed *and* a concurrent push from another thread.

**SDL_mixer** is the extreme case of the audio rule: its channel-finished,
music-finished, post-mix, music-hook and effect callbacks all run on the audio
thread with the device locked, and almost every `Mix_*` call takes that lock.
`pysdl_Mixer.c` wraps each such call in `UNLOCKED(...)` (GIL released; touch no
Python object inside). Callbacks get audio as `bytes` and may return
replacement `bytes` of the same length. Custom effects: SDL_mixer can only
unregister an effect by its C function pointer, so each channel with Python
effects has *one* trampoline registration that runs that channel's list
(`_effects`: channel → `[(effect, done), …]`); SDL_mixer drops a channel's
effects when it stops, and the done trampoline empties the list.
`tests/test_mixer.py` has a subprocess stress test (re-entrant callbacks while
the main thread hammers `Mix_*`) whose timeout catches a regression.

New off-main-thread callbacks (timers, event filters) must follow the same
`PySDL_ThreadEnter` / `PySDL_ThreadLeave` pattern, and the trampoline takes its
own reference to the callable for the duration of the call, since a callback
may unregister itself. Registries keep what SDL's `void *userdata` points at
alive:
- event filter: `_py_event_filter`, passed as the SDL userdata, released only
  after `SDL_SetEventFilter` (which waits out an in-flight call) returns;
- event watches: `_py_event_watches` list (`pysdl_Events.c`);
- hint callbacks: `_hint_callbacks` list of `(name, callable)` tuples
  (`pysdl_Video.c`), emptied by `ClearHints` because `SDL_ClearHints` frees
  every SDL-side callback;
- `JoystickAttachVirtualEx`: a module dict keyed by instance id until
  `JoystickDetachVirtual`.

### Conventions

- Errors: set `pysdl_Error` (exposed as `SDL2.error`) with `SDL_GetError()` /
  `IMG_GetError()` and return `NULL`. `pysdl_Renderer.c` / `pysdl_Texture.c` /
  `pysdl_Surface.c` have a local `_raise()` helper that does exactly this.
- Create wrapper objects with `PySDL_New(&PySDL_X_Type)`; `Py_DECREF` the wrapper
  on the SDL-failure path (it owns nothing yet, but the wrapper itself leaks).
- Method names drop `SDL_`. `Renderer` keeps the rest verbatim
  (`RenderSetViewport`, `GetRendererInfo`) except the core draw calls, which
  also drop `Render` (`Clear`, `Present`, `Copy*`, `Draw*`, `Fill*`);
  `Texture` and `Surface` also drop the type word (`SDL_UpdateTexture` →
  `Update`, `SDL_SetSurfaceBlendMode` → `SetBlendMode`, `SDL_BlitSurface` →
  `Blit`) — except the pre-existing `Surface.LockSurface` / `UnlockSurface` /
  `SaveBMP`, kept for compatibility.
- Take a `PySDL_<Type> *` argument with `O!` + `&PySDL_<Type>_Type`, or check
  with `PyObject_TypeCheck` before casting — never cast an unchecked `O`.
- Rect/point/colour args go through `PyToRect` / `PyToPoint` / `PyToColor` /
  `PyToFRect` / `PyToFPoint` (tuple or list; a 2-item rect leaves `w`/`h` as
  `-1`, a 3-item colour sets `a = 255`). Check the return — they raise and
  return 0 on bad input. Pass `None` for optional rect args; guard with
  `if (arg && arg != Py_None)` since an omitted optional stays `NULL`.
- Anything that reads a file (`LoadBMP`, `LoadImage`, `Renderer.LoadTexture`,
  `IMG_*`) takes a path or the file's bytes through `PySDL_RWFromObject`;
  anything that writes one (`Surface.SavePNG` / `SaveJPG`) returns `bytes` when
  no path is given. SDL_image's own new functions keep the `IMG_` prefix.
- `PyToPixel(obj, format, &Uint32)` accepts an already-mapped int verbatim or
  maps an `(r,g,b[,a])` sequence through `format` — used for fill/colour-key.
- Renderer draw primitives (`DrawPoint(s)`, `DrawLine(s)`, `DrawRect(s)`,
  `FillRect(s)`) call the SDL `*F` float functions internally and accept int or
  float coordinates. `Copy`/`CopyEx` take integer rects; `CopyF`/`CopyExF` take a
  float dst rect for sub-pixel placement.
- C99 designated initializers for `PyTypeObject`; `PY_SSIZE_T_CLEAN` is set.
