# SDL2 binding roadmap

A phased plan to take this binding from its current partial coverage to broad
SDL2 (+ SDL2_image) coverage, keeping the established style: one wrapped type per
`src/pysdl_<Type>.c` file, the `SDL_` prefix dropped from names, errors raised
through `pysdl_Error`, blocking calls wrapped in `Py_BEGIN_ALLOW_THREADS`.

## Design decisions to settle first

These shape every later phase.

| Question | Decision | Rationale |
|---|---|---|
| Event representation | Keep `(type, data)` with positional tuples; extend the `switch` in `_event()`. Add sub-type constants (`WINDOWEVENT_*`, `CONTROLLERBUTTON*`) and a `SDL2.event_name(type)` helper. | Consistency with existing `PollEvent`/`WaitEvent`. Attribute objects would be cleaner but a full rewrite. |
| Feeding file data to loaders | Accept `bytes` directly in `LoadBMP` / `LoadImage` / `LoadWAV` and build an `SDL_RWops` internally. Do not expose an `RWops` type. | Pythonic; avoids a whole type surface. |
| `_constants.c` upkeep | Add `tools/gen_constants.py` that scans the installed `SDL_*.h` for `#define`/enum values and emits the file (or a supplement). | ~700 hand-maintained constants will not keep pace with new subsystems. |
| Borrowed vs owned pointers | Generalize the `Surface.shouldFree` idea into a documented convention plus a `PYSDL_WRAP_BORROWED(type, ptr)` helper in `pysdl.h`. | Several new APIs return pointers SDL still owns (`GetWindowFromID`, `GameController.GetJoystick`, joystick-from-event). |
| Off-thread Python callbacks | One shared trampoline helper (GIL ensure -> `Py_IsFinalizing` guard -> call -> print/clear on error). Reuse for audio, timers, event filters, hint callbacks. | The audio callback pattern is about to be copied 3-4 times. |
| `setup.py` flags | Derive from `sdl2-config --cflags --libs` / `pkg-config`, keep the hardcoded homebrew / `/usr/local` list as fallback. | Removes the "edit setup.py" friction as more libs get linked. |

## Phase 0 - Infrastructure  — DONE

- **Headless test harness**: `pytest` suite under `tests/` (`conftest.py` forces
  the dummy drivers, auto-builds, injects `sys.path`; `sdl` session fixture).
  `test/info.py` ported to `tests/test_smoke.py`. 50 tests.
- **Audio callback template fixed**: `PySDL_Audio.pycallback` now holds the
  `(callback, userdata)` tuple and is released in `Close`/dealloc;
  `_playback_callback` checks the return is `bytes` and long enough (else
  silence); both callbacks route through `PySDL_ThreadEnter`. Also found & fixed
  a GIL deadlock — `Close`/`Lock`/`Pause` now drop the GIL around the SDL call
  that joins the callback thread.
- **Helpers** in `src/pysdl_util.c`: `PySDL_New`, `PySDL_ThreadEnter` /
  `PySDL_ThreadLeave`. (The `PYSDL_WRAP_BORROWED` macro is deferred to the first
  phase that returns a borrowed pointer; `Surface.shouldFree` remains the
  documented pattern.)
- **Struct converters** moved to `pysdl_util.c` and hardened (raise on bad
  input): `PyToRect` / `PyToPoint` / `PyToColor` / `RectToPy` / `PointToPy`.
  `Renderer.Copy` / `CopyEx` now check the return (they previously crashed on
  `Copy(texture)` with no rect — omitted optionals are `NULL`, not `Py_None`).
- **Constants tool** `tools/gen_constants.py`: reports the ~580 unexposed
  symbols grouped by header; `--emit <substr>` prints pasteable lines.
- **`setup.py`** now probes `sdl2-config` / `pkg-config`, keeping the hardcoded
  prefixes as fallback.

**Quick wins landed**: `Delay`, `GetTicks64` (guarded ≥2.0.18),
`GetPerformanceCounter`, `GetPerformanceFrequency`, `GetVersion`, `GetRevision`,
`SetError`, `ClearError`, `GetBasePath`, `GetPrefPath`, `EnableScreenSaver` /
`DisableScreenSaver` / `IsScreenSaverEnabled`, `GetClipboardText` /
`SetClipboardText` / `HasClipboardText`, `Renderer.GetRenderDrawColor`, and
`Texture.GL_Bind` now returns `(w, h)`.

## Phase 1 - Rendering completeness  — DONE

`pysdl_Renderer.c` / `pysdl_Texture.c` rewritten; `Renderer` and `Texture` are
now in the module namespace and constructible. Tests: `tests/test_renderer.py`,
`tests/test_texture.py`; example: `test/draw.py`.

- **Construction**: `SDL2.Renderer(window, index=-1, flags=0)`,
  `SDL2.Texture(renderer, format, access, size)`,
  `SDL2.CreateSoftwareRenderer(surface)`. `Window.CreateRenderer(flags=0,
  index=-1)` now takes args — **behaviour change**: it no longer forces
  `ACCELERATED | PRESENTVSYNC`; pass the flags you want.
- **Primitives**: `DrawPoint(s)`, `DrawLine(s)`, `DrawRect(s)`, `FillRect(s)`
  (all call the SDL `*F` functions internally, accept int or float),
  `RenderGeometry(texture, vertices, indices=None)` (≥2.0.18), `CopyF` / `CopyExF`.
- **State**: `Get/SetRenderDrawBlendMode`, `RenderSet/GetViewport`,
  `RenderSet/GetClipRect`, `RenderIsClipEnabled`, `RenderSet/GetScale`,
  `RenderSet/GetLogicalSize`, `RenderSet/GetIntegerScale`.
- **Targets**: `SetRenderTarget` / `GetRenderTarget` (returns the same wrapper
  object via `Renderer.target`), `RenderTargetSupported`,
  `RenderReadPixels(rect=None, format=PIXELFORMAT_ARGB8888)` -> bytes.
- **Info**: `GetRendererInfo`, `GetRendererOutputSize`, `RenderFlush`.
- **Texture**: `Update(pixels, rect=None, pitch=0)`, `UpdateYUV(...)`,
  `Set/GetColorMod`, `Set/GetAlphaMod`, `Set/GetBlendMode`, `Set/GetScaleMode`
  (≥2.0.12), `Lock(rect=None)` -> `(writable memoryview, pitch)` valid until
  `Unlock()`.
- `Copy` / `CopyEx` gained `O!` texture type-checking (was an unchecked cast).
- Module: `SDL2.ComposeCustomBlendMode(...)`.
- New converters `PyToFRect` / `PyToFPoint` in `pysdl_util.c`.
- Constants: `TEXTUREACCESS_*`, `TEXTUREMODULATE_*`, `BLENDMODE_*`,
  `BLENDOPERATION_*`, `BLENDFACTOR_*`, `SCALEMODE_*`.

## Phase 2 - Surfaces & pixels

- `pysdl_Surface.c`: `BlitSurface`, `BlitScaled`, `FillRect(s)`,
  `Set/GetColorKey`, `SetSurfaceBlendMode` / `ColorMod` / `AlphaMod` (+ getters),
  `Set/GetClipRect`, `ConvertSurface`, `ConvertSurfaceFormat`,
  `DuplicateSurface`, `SoftStretch`, `SetSurfaceRLE`, `SetSurfacePalette`;
  constructors `CreateRGBSurfaceWithFormat[From]`, `LoadBMP` from bytes.
- New `src/pysdl_PixelFormat.c` -> `SDL2.PixelFormat`: `AllocFormat` /
  `FreeFormat`, `MapRGB(A)`, `GetRGB(A)`, name/masks; module helpers
  `GetPixelFormatName`, `PixelFormatEnumToMasks`, `MasksToPixelFormatEnum`.
- New `src/pysdl_Palette.c` -> `SDL2.Palette`: `AllocPalette`,
  `SetPaletteColors`, `FreePalette`.
- SDL2_image: `IMG_Init` / `IMG_Quit`, `Surface.SavePNG` / `SaveJPG`.

## Phase 3 - Keyboard, mouse, text input

- Keyboard: `GetKeyName`, `GetKeyFromName`, `GetScancodeName`,
  `GetScancodeFromName`, `GetKeyFromScancode`, `GetScancodeFromKey`,
  `SetModState`, `GetKeyboardFocus`, `HasScreenKeyboardSupport`,
  `IsScreenKeyboardShown`.
- Text input: `StartTextInput`, `StopTextInput`, `IsTextInputActive`,
  `SetTextInputRect`; handle `SDL_TEXTINPUT` / `SDL_TEXTEDITING` in `_event()`
  (string payload).
- Mouse: `GetMouseState`, `GetGlobalMouseState`, `GetRelativeMouseState`,
  `WarpMouseInWindow`, `WarpMouseGlobal`, `Set/GetRelativeMouseMode`,
  `CaptureMouse`, `GetMouseFocus`.
- New `src/pysdl_Cursor.c` -> `SDL2.Cursor`: `CreateCursor`,
  `CreateColorCursor`, `CreateSystemCursor`, `Set/GetCursor`,
  `GetDefaultCursor`, `FreeCursor`.

## Phase 4 - Game controllers & joysticks

- New `src/pysdl_Joystick.c` -> `SDL2.Joystick`: `NumJoysticks` (module), open by
  index, name/GUID/instance-id, `NumAxes/Buttons/Hats/Balls`,
  `GetAxis/Button/Hat/Ball`, `Rumble`, `RumbleTriggers`, `CurrentPowerLevel`,
  `SetLED`, `JoystickUpdate` / `JoystickEventState`.
- New `src/pysdl_GameController.c` -> `SDL2.GameController`: `IsGameController`
  (module), open, `Name`, `GetAxis`, `GetButton`, `Mapping`, `AddMapping` /
  `AddMappingsFromFile`, `Rumble`, `GetJoystick` (borrowed), `Update`, `SetLED`,
  `GetType`.
- `_event()`: `CONTROLLERAXISMOTION`, `CONTROLLERBUTTONDOWN/UP`,
  `CONTROLLERDEVICEADDED/REMOVED/REMAPPED`, `JOYDEVICEADDED/REMOVED`,
  `JOYHATMOTION`, `JOYBALLMOTION`.

## Phase 5 - Events subsystem completeness

- Module: `PushEvent`, `PumpEvents`, `PeepEvents`, `FlushEvent(s)`,
  `HasEvent(s)`, `WaitEventTimeout`, `RegisterEvents`, `EventState` /
  `GetEventState`, `QuitRequested`.
- Remaining `_event()` structs: `DROPFILE/DROPTEXT/DROPBEGIN/DROPCOMPLETE`,
  `AUDIODEVICEADDED/REMOVED`, `USEREVENT`, `DISPLAYEVENT`, `SENSORUPDATE`,
  `FINGERDOWN/UP/MOTION`, `MULTIGESTURE`, `DOLLARGESTURE`, `CLIPBOARDUPDATE`,
  `KEYMAPCHANGED`; window-event subtype decode.
- Optional/advanced: `SetEventFilter` / `AddEventWatch` via the trampoline.

## Phase 6 - Timers, haptics, sensors, touch, power

- New `src/pysdl_Timer.c` -> `SDL2.Timer`: `AddTimer` / `RemoveTimer`
  (trampoline, callback returns next interval).
- New `src/pysdl_Haptic.c` -> `SDL2.Haptic`: open from index / joystick / mouse,
  `RumbleInit` / `RumblePlay` / `RumbleStop`, effect lifecycle (`NewEffect` /
  `RunEffect` / `UpdateEffect` / `StopEffect` / `DestroyEffect`), `Query`,
  `NumEffects`.
- New `src/pysdl_Sensor.c` -> `SDL2.Sensor`: `NumSensors`, open, `GetData`,
  `GetType`, `GetName`.
- Touch (module): `GetNumTouchDevices`, `GetTouchDevice`, `GetNumTouchFingers`,
  `GetTouchFinger`, gesture record/load/save.
- Power: `GetPowerInfo`. CPU extras: `GetSystemRAM`, `HasNEON`, `HasARMSIMD`,
  `HasLSX`, `HasLASX`, `SIMDGetAlignment`.

## Phase 7 - Video/window completeness & system integration

- Window: `GetWindowFlags`, `GetWindowDisplayIndex`, `GetWindowPixelFormat`,
  min/max size, `SetWindowBordered`, `SetWindowAlwaysOnTop`,
  `SetWindowInputFocus`, `SetWindowModalFor`, `Set/GetWindowGrab` (+ keyboard /
  mouse grab), `Set/GetWindowOpacity`, `SetWindowMouseRect`, `FlashWindow`,
  `GetWindowBordersSize`, `UpdateWindowSurfaceRects`, gamma ramp,
  `Set/GetWindowDisplayMode`; module `GetWindowFromID` / `GetGrabbedWindow`
  (borrowed); `SetWindowHitTest` (trampoline, optional).
- Display: `GetDisplayName`, `GetDisplayUsableBounds`, `GetDisplayOrientation`,
  `GetNumDisplayModes`, `GetClosestDisplayMode`; add a mode-index arg to
  `GetDisplayMode` (currently hardcoded to 0).
- Message box: `ShowSimpleMessageBox`, `ShowMessageBox` (button list).
- Hints: `SetHint`, `SetHintWithPriority`, `GetHint`, `GetHintBoolean`,
  `ClearHints`.
- Misc: `OpenURL`, `GetPreferredLocales`.
- Optional/advanced: Vulkan (`Vulkan_LoadLibrary`,
  `Vulkan_GetInstanceExtensions`, `Vulkan_CreateSurface`), `GL_LoadLibrary` /
  `GL_GetCurrentContext`.

## Phase 8 - Audio completeness

- Module: `GetNumAudioDrivers`, `GetAudioDriver`, `GetCurrentAudioDriver`,
  `AudioInit` / `AudioQuit`, `GetAudioDeviceStatus`, `LoadWAV` (path + bytes),
  `MixAudioFormat`, `GetAudioDeviceSpec`.
- Device: `ClearQueuedAudio`.
- New `src/pysdl_AudioStream.c` -> `SDL2.AudioStream`: `NewAudioStream`, `Put`,
  `Get`, `Available`, `Flush`, `Clear`, `Free` (the modern resampling path).

## Phase 9 - Loose ends

- Rect math as pure-Python helpers in a thin `SDL2/__init__` shim, or in C:
  `HasIntersection`, `IntersectRect`, `UnionRect`, `EnclosePoints`,
  `PointInRect`, `RectEmpty`, `RectEquals`.
- Filesystem / RWops: covered implicitly by the "accept bytes" decision.

## Explicitly out of scope

Threads / mutexes / semaphores / condition vars / atomics (use Python's),
`SDL_Log*`, assertions, `SDL_main` / main callbacks, platform-specific APIs
(Android / iOS / WinRT), `GetWindowWMInfo`, stdinc shims. Satellite libraries
(`SDL2_ttf`, `SDL2_mixer`, `SDL2_net`, `SDL2_gfx`) are a separate effort - each
its own linked lib + `pysdl_ttf.c` etc., or separate packages.

## Per-phase checklist

Each phase ships: new constants, `.tp_doc` + terse method docstrings matching the
existing style, one `test/` example script, one headless `tests/test_<area>.py`,
and a README coverage note.
