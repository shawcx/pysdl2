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
  `example/info.py` ported to `tests/test_smoke.py`. 50 tests.
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
`tests/test_texture.py`; example: `example/draw.py`.

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

## Phase 2 - Surfaces & pixels  — DONE

Tests: `tests/test_surface.py`, `tests/test_pixelformat.py`; example:
`example/surface.py`.

- **`Surface`** methods (short names, like `Texture`): `Blit`, `BlitScaled`,
  `SoftStretch`, `FillRect` / `FillRects` (colour is an int or `(r,g,b[,a])`),
  `SetColorKey` / `GetColorKey`, `SetBlendMode` / `GetBlendMode`,
  `SetColorMod` / `GetColorMod`, `SetAlphaMod` / `GetAlphaMod`,
  `SetClipRect` / `GetClipRect`, `SetRLE`, `SetPalette`, `Convert` (enum int
  *or* `PixelFormat`), `Duplicate`, `MapRGB` / `MapRGBA`, `GetPixelFormat`,
  `SavePNG`, `SaveJPG`. New `pitch` attribute.
- New **`SDL2.PixelFormat(format_enum)`** (`src/pysdl_PixelFormat.c`):
  `MapRGB` / `MapRGBA` / `GetRGB` / `GetRGBA`, `SetPalette`; attrs `format`,
  `bpp`, `bytes`, `R/G/B/Amask`.
- New **`SDL2.Palette(ncolors)`** (`src/pysdl_Palette.c`): `SetColors(colors,
  first=0)`, `GetColors()`, `ncolors`.
- Module: `CreateRGBSurfaceWithFormat` / `…WithFormatFrom`, `GetPixelFormatName`,
  `PixelFormatEnumToMasks`, `MasksToPixelFormatEnum`, `IMG_Init` / `IMG_Quit`.
- `LoadBMP` / `LoadImage` now also take the file's `bytes` (RWops built
  internally); `…SurfaceFrom` keeps the caller's buffer alive via
  `Surface.pixels` (`Py_buffer`) — fixes a pre-existing dangling-pointer bug.
- New converter `PyToPixel`. Constants: `PIXELFORMAT_{RGBA,ARGB,BGRA,ABGR}32`,
  `PIXELFORMAT_X{RGB,BGR}8888`, `SWSURFACE`/`PREALLOC`/`RLEACCEL`/`DONTFREE`,
  `IMG_INIT_*`.

## Phase 3 - Keyboard, mouse, text input  — DONE

New `src/pysdl_Input.c` holds the module-level keyboard/mouse/text functions
(its `PyMethodDef` array is merged in `PyInit_SDL2` via `PyModule_AddFunctions`).
Tests: `tests/test_input.py`, `tests/test_cursor.py`; example:
`example/keyboard.py`.

- **Keyboard:** `GetKeyName`, `GetKeyFromName`, `GetScancodeName`,
  `GetScancodeFromName`, `GetKeyFromScancode`, `GetScancodeFromKey`,
  `SetModState`, `GetKeyboardFocus` (borrowed `Window`),
  `HasScreenKeyboardSupport`, `IsScreenKeyboardShown`. Constant `K_UNKNOWN`.
- **Text input:** `StartTextInput`, `StopTextInput`, `IsTextInputActive`,
  `SetTextInputRect`. `_event()` decodes `TEXTINPUT` (data = the `str`) and
  `TEXTEDITING` (`(text, start, length)`); the `KEYDOWN`/`KEYUP` tuple gained a
  5th element `repeat` (bool).
- **Mouse:** `GetMouseState` / `GetGlobalMouseState` / `GetRelativeMouseState`
  (each `(buttons, x, y)`), `WarpMouseInWindow`, `WarpMouseGlobal`,
  `SetRelativeMouseMode` / `GetRelativeMouseMode`, `CaptureMouse`,
  `GetMouseFocus` (borrowed `Window`). Constants `PRESSED`/`RELEASED`,
  `BUTTON_*` / `BUTTON_*MASK`, `MOUSEWHEEL_NORMAL`/`FLIPPED`.
- New **`SDL2.Cursor(system_cursor_id)`** (`src/pysdl_Cursor.c`) + `Set()`;
  module `CreateColorCursor(surface, hx, hy)`, `CreateCursor(data, mask, size,
  hot)`, `GetCursor` / `GetDefaultCursor` (borrowed), `SetCursor`. Constants
  `SYSTEM_CURSOR_*`.
- `Window.tp_init` now takes `title` as optional (borrowed-window path);
  `PySDL_Window` gained `shouldFree`. New helper `PySDL_WrapWindow()`.

## Phase 4 - Game controllers & joysticks  — DONE

Tests: `tests/test_joystick.py` (driven by a virtual joystick); example:
`example/gamepad.py`.

- New **`SDL2.Joystick(device_index)`** (`src/pysdl_Joystick.c`): `Name`,
  `GetGUID`, `InstanceID`, `Attached`, `NumAxes` / `NumButtons` / `NumHats` /
  `NumBalls`, `GetAxis` / `GetButton` / `GetHat` / `GetBall`, `Rumble`,
  `RumbleTriggers` / `SetLED` / `HasLED` (≥2.0.14), `CurrentPowerLevel`,
  `SetVirtualAxis` / `SetVirtualButton` / `SetVirtualHat` (≥2.0.14), `Close`.
  Module: `NumJoysticks`, `JoystickNameForIndex`, `JoystickUpdate`,
  `JoystickEventState`, `JoystickAttachVirtual` / `JoystickDetachVirtual` /
  `JoystickIsVirtual` (≥2.0.14).
- New **`SDL2.GameController(device_index)`** (`src/pysdl_GameController.c`):
  `Name`, `Attached`, `GetAxis`, `GetButton`, `Mapping`, `GetJoystick`
  (borrowed), `Rumble`, `RumbleTriggers` / `SetLED` (≥2.0.14), `GetType`
  (≥2.0.12), `Close`. Module: `IsGameController`, `GameControllerNameForIndex`,
  `GameControllerAddMapping` / `…AddMappingsFromFile`, `GameControllerUpdate`,
  `GameControllerEventState`.
- `_event()`: `JOYBALLMOTION`, `JOYHATMOTION`, `JOYDEVICEADDED/REMOVED`,
  `CONTROLLERAXISMOTION`, `CONTROLLERBUTTONDOWN/UP`,
  `CONTROLLERDEVICEADDED/REMOVED/REMAPPED`.
- Module: `InitSubSystem` / `QuitSubSystem` (were missing). Constants:
  `HAT_*`, `JOYSTICK_POWER_*`, `JOYSTICK_TYPE_*`, `CONTROLLER_AXIS_*`,
  `CONTROLLER_BUTTON_*`, `CONTROLLER_TYPE_*`, the joystick/controller event
  types.

## Phase 5 - Events subsystem completeness  — DONE

`_event()` and all the queue functions moved to the new `src/pysdl_Events.c`
(`pysdl_events_methods` merged via `PyModule_AddFunctions`). `GetKeyState` /
`GetModState` moved from `pysdl.c` to `pysdl_Input.c`. Tests:
`tests/test_events.py`; example: `example/events.py`.

- Module: `PumpEvents`, `PushEvent(type, code=0, windowID=0)`, `PeepEvents(count,
  action=PEEKEVENT, minType, maxType)` -> list, `FlushEvent` / `FlushEvents`,
  `HasEvent` / `HasEvents`, `WaitEventTimeout(ms)`, `RegisterEvents(n)`,
  `EventState(type, state)` / `GetEventState(type)`, `QuitRequested`.
- `_event()` now decodes `JOYDEVICE*` / `CONTROLLER*` (from phase 4) plus
  `DROPFILE` / `DROPTEXT` (frees `drop.file`) / `DROPBEGIN` / `DROPCOMPLETE`,
  `AUDIODEVICEADDED` / `REMOVED`, `FINGERDOWN` / `UP` / `MOTION`, `MULTIGESTURE`,
  `DOLLARGESTURE` / `DOLLARRECORD`, `DISPLAYEVENT`, `SENSORUPDATE` (≥2.0.9);
  any type `>= USEREVENT` yields `(code, windowID)`.
- Event filter (advanced): `SetEventFilter(callable | None)` / `GetEventFilter`
  (one global filter; callback returns truthy to keep the event) and
  `FilterEvents(callable)` (run once over the queue) — both through the
  `PySDL_ThreadEnter` trampoline.
- Constants: `DROP*`, `FINGER*`, `MULTIGESTURE`, `DOLLAR*`, `CLIPBOARDUPDATE`,
  `SENSORUPDATE`, `RENDER_TARGETS_RESET` / `RENDER_DEVICE_RESET`, `FIRSTEVENT` /
  `LASTEVENT` / `ADDEVENT` / `PEEKEVENT` / `GETEVENT`.

## Phase 6 - Timers, haptics, sensors, touch, power  — DONE

Tests: `tests/test_timer.py`, `tests/test_system.py`; example: `example/timer.py`.

- New **`SDL2.Timer(interval_ms, callback)`** (`src/pysdl_Timer.c`): `Remove()`,
  `id`. Callback fires on SDL's timer thread via the `PySDL_ThreadEnter`
  trampoline; its return value is the next interval (`None` = same, `0`/falsy =
  stop). Dropping the wrapper cancels the timer (GIL dropped around
  `SDL_RemoveTimer`).
- New **`SDL2.Haptic(device_index)`** (`src/pysdl_Haptic.c`): `Query`, `NumAxes`,
  `NumEffects` / `NumEffectsPlaying`, `RumbleSupported` / `RumbleInit` /
  `RumblePlay(strength, ms)` / `RumbleStop`, `SetGain` / `SetAutocenter`,
  `Pause` / `Unpause` / `StopAll`, `EffectSupported` / `NewEffect` /
  `UpdateEffect` / `RunEffect` / `StopEffect` / `DestroyEffect` /
  `GetEffectStatus` (effects passed as a dict with `type` = LEFTRIGHT / CONSTANT
  / SINE / TRIANGLE / SAWTOOTH*). Module: `NumHaptics`, `HapticName`,
  `MouseIsHaptic`, `HapticOpenFromMouse`, `JoystickIsHaptic`,
  `HapticOpenFromJoystick`.
- New **`SDL2.Sensor(device_index)`** (`src/pysdl_Sensor.c`): `GetName`,
  `GetType`, `GetNonPortableType`, `GetInstanceID`, `GetData(count=6)`, `Close`.
  Module: `NumSensors`, `SensorGetDeviceName` / `…Type` / `…InstanceID`,
  `SensorUpdate`.
- Touch (module, in `pysdl_Input.c`): `GetNumTouchDevices`, `GetTouchDevice`,
  `GetTouchDeviceType`, `GetTouchName` (≥2.0.22), `GetNumTouchFingers`,
  `GetTouchFinger` -> `(id, x, y, pressure)`, `RecordGesture`,
  `LoadDollarTemplates` / `SaveDollarTemplate` / `SaveAllDollarTemplates` (path).
- Power: `GetPowerInfo` -> `(state, seconds, percent)`. CPU extras: `GetSystemRAM`,
  `HasNEON`, `HasAVX512F`, `HasARMSIMD` (≥2.0.12), `HasLSX` / `HasLASX` (≥2.24),
  `SIMDGetAlignment` (≥2.0.10).
- Constants: `INIT_SENSOR`, `HAPTIC_*`, `SENSOR_*`, `TOUCH_DEVICE_*`,
  `TOUCH_MOUSEID` / `MOUSE_TOUCHID`, `POWERSTATE_*`.

## Phase 7 - Video/window completeness & system integration  — DONE

New `src/pysdl_Video.c` for the module-level video functions. Tests:
`tests/test_video.py`; example: `example/window.py`.

- **Window** methods added (`pysdl_Window.c`): `GetWindowFlags`,
  `GetWindowDisplayIndex`, `GetWindowPixelFormat`, `Set/GetWindowMinimumSize`,
  `Set/GetWindowMaximumSize`, `SetWindowBordered`, `SetWindowInputFocus`,
  `SetWindowModalFor`, `Set/GetWindowGrab`, `Set/GetWindowOpacity`,
  `GetWindowBordersSize`, `UpdateWindowSurfaceRects`, `Set/GetWindowGammaRamp`
  (256-value channels), `Set/GetWindowDisplayMode`; `SetWindowAlwaysOnTop`,
  `Set/GetWindowKeyboardGrab`, `Set/GetWindowMouseGrab`, `FlashWindow` (≥2.0.16);
  `Set/GetWindowMouseRect` (≥2.0.18). (`SetWindowHitTest` skipped — a hit-test
  callback per pixel is a poor fit here.)
- **Display** (module): `GetDisplayName`, `GetDisplayUsableBounds`,
  `GetDisplayOrientation`, `GetNumDisplayModes`, `GetClosestDisplayMode`;
  `GetPointDisplayIndex` / `GetRectDisplayIndex` (≥2.24). `GetDisplayMode(display,
  mode=0)` now takes the mode index.
- **Message box**: `ShowSimpleMessageBox(title, message, flags=INFORMATION,
  window=None)`, `ShowMessageBox({title, message, buttons, flags, window})` ->
  selected button id (buttons are `(id, text[, flags])`).
- **Hints**: `SetHint`, `SetHintWithPriority`, `GetHint`, `GetHintBoolean`,
  `ResetHint` (≥2.24), `ClearHints`.
- **Misc**: `OpenURL`, `GetPreferredLocales` -> `[(lang, country_or_None), …]`
  (≥2.0.14).
- **Borrowed windows** (module): `GetWindowFromID`, `GetGrabbedWindow` (≥2.0.16),
  `GL_GetCurrentWindow`.
- **GL loaders** (module): `GL_LoadLibrary`, `GL_UnloadLibrary`,
  `GL_GetProcAddress` -> address int, `GL_GetCurrentContext` -> address int.
- **Vulkan**: module `Vulkan_LoadLibrary` / `Vulkan_UnloadLibrary` /
  `Vulkan_GetInstanceExtensions(window=None)` -> `[str]` /
  `Vulkan_GetVkGetInstanceProcAddr()` -> address int; window
  `Vulkan_GetDrawableSize()` -> `(w, h)` and `Vulkan_CreateSurface(vk_instance:int)`
  -> `vk_surface:int` (handles as ints, for interop with a Python Vulkan binding).
- Constants: `WINDOW_VULKAN` / `WINDOW_METAL` / `WINDOW_*_GRABBED`,
  `WINDOWPOS_UNDEFINED`, `MESSAGEBOX_*`, `HINT_DEFAULT` / `NORMAL` / `OVERRIDE`,
  `ORIENTATION_*`, `FLASH_*` (≥2.0.16).

## Phase 8 - Audio completeness  — DONE

Tests: `tests/test_audio.py` (extended); example: `example/wav.py`.

- Module (`pysdl_Audio.c`, `pysdl_audio_methods`): `GetNumAudioDrivers`,
  `GetAudioDriver`, `GetCurrentAudioDriver`, `AudioInit` / `AudioQuit`,
  `LoadWAV(path | bytes)` -> `((freq, format, channels, samples), bytes)`,
  `MixAudioFormat(dst, src, format, volume=MIX_MAXVOLUME)` -> mixed bytes,
  `GetAudioDeviceSpec(index, iscapture=False)` (≥2.0.16),
  `GetDefaultAudioInfo(iscapture=False)` (≥2.24).
- `Audio` methods: `ClearQueued`, `GetStatus` (-> `AUDIO_STOPPED` / `PLAYING` /
  `PAUSED`). `Queue` now accepts any bytes-like; `Dequeue` returns exactly the
  bytes read (both hardened). `Open()`'s `deviceName` is now optional — omit it
  for the system default device.
- New **`SDL2.AudioStream(src_format, src_channels, src_rate, dst_format,
  dst_channels, dst_rate)`** (`src/pysdl_AudioStream.c`): `Put(data)`,
  `Get(max_len)` -> bytes, `Available()`, `Flush()`, `Clear()`; freed on dealloc.
- Constants: `AUDIO_STOPPED` / `PLAYING` / `PAUSED`, `MIX_MAXVOLUME`.

## Phase 9 - Loose ends  — DONE

Tests: `tests/test_rect.py`; example: `example/rects.py`.

- New `src/pysdl_Rect.c` (`pysdl_rect_methods`): `HasIntersection`,
  `IntersectRect` (-> rect or `None`), `UnionRect`, `EnclosePoints(points,
  clip=None)`, `IntersectRectAndLine(rect, (x1,y1,x2,y2))` (-> clipped line or
  `None`), `PointInRect`, `RectEmpty`, `RectEquals`; float variants
  `HasIntersectionF` / `IntersectFRect` / `UnionFRect` / `PointInFRect` /
  `FRectEmpty` / `FRectEquals` (≥2.0.22). (No `SDL2/__init__` shim — the binding
  is a single extension module; the helpers are in C over the same tuple rects.)
- **Metal** (rounding out the GL / Vulkan / Metal trio, ≥2.0.11): window
  `Metal_CreateView()` -> view int, `Metal_GetDrawableSize()`; module
  `Metal_GetLayer(view)` -> `CAMetalLayer` pointer int, `Metal_DestroyView(view)`.
- **Bug fix**: ~14 constants (`WINDOW_ALWAYS_ON_TOP`, `WINDOW_VULKAN`,
  `WINDOW_METAL`, `WINDOW_*_GRABBED`, `BLENDMODE_MUL`, `LOCALECHANGED`,
  `PIXELFORMAT_XRGB8888`, …) were guarded with `#ifdef` on an enum member and so
  never registered; switched to `SDL_VERSION_ATLEAST`.
- Filesystem / RWops: covered by the "accept bytes / take a path" decision
  throughout (`LoadBMP` / `LoadImage` / `LoadWAV`, the `…SurfaceFrom` keepalive,
  dollar-template save/load).

## Phase 10 - Joystick & game-controller completeness  — DONE

Tests: `tests/test_joystick.py` (virtual pads, incl. `AttachVirtualEx`
callbacks); example: `example/gamepad.py` (device info, binds, sensors,
touchpad/sensor/battery events, `--virtual` pad reporting rumble/LED).

- **Joystick** methods: `GetVendor` / `GetProduct` / `GetProductVersion` /
  `GetType` / `GetAxisInitialState` (-> value or `None`), `GetPlayerIndex`
  (≥2.0.9), `SetPlayerIndex` (≥2.0.12), `GetSerial` (≥2.0.14), `SendEffect(bytes)`
  (≥2.0.16), `HasRumble` / `HasRumbleTriggers` (≥2.0.18), `GetFirmwareVersion` /
  `Path` (≥2.24). Module: `JoystickGetDevice{GUID,InstanceID,Vendor,Product,
  ProductVersion,Type}`, `JoystickGetDevicePlayerIndex` (≥2.0.9),
  `JoystickPathForIndex` (≥2.24), `JoystickFromInstanceID`,
  `JoystickFromPlayerIndex` (≥2.0.12), `Lock/UnlockJoysticks` (≥2.0.7).
- **GUIDs** stay 32-char hex strings; anything taking a GUID also accepts the raw
  16 bytes (`PyToGUID` / `GUIDToPy` in `pysdl_Joystick.c`).
  `JoystickGetGUIDFromString` / `GUIDFromString` (≥2.24) -> 16 bytes,
  `JoystickGetGUIDString` / `GUIDToString` (≥2.24) -> str, `GetJoystickGUIDInfo`
  (≥2.26) -> `(vendor, product, version, crc16)`.
- **`JoystickAttachVirtualEx(type, naxes, nbuttons, nhats, *, vendor_id,
  product_id, button_mask, axis_mask, name, update, set_player_index, rumble,
  rumble_triggers, set_led, send_effect)`** (≥2.24). Callbacks go through
  `PySDL_ThreadEnter`; returning `None`/truthy reports success, falsy or an
  exception reports failure. Only the given callbacks are wired, so e.g.
  `HasRumble()` reflects whether `rumble=` was passed. The callback tuple is kept
  in a module dict keyed by instance id and released by `JoystickDetachVirtual`.
- **GameController** methods: `GetBindForAxis` / `GetBindForButton` (-> `None`,
  `(BINDTYPE_BUTTON|AXIS, n)` or `(BINDTYPE_HAT, hat, mask)`), `GetVendor` /
  `GetProduct` / `GetProductVersion`, `GetPlayerIndex` (≥2.0.9),
  `SetPlayerIndex` (≥2.0.12); ≥2.0.14: `HasLED`, `HasAxis`, `HasButton`,
  `GetSerial`, `HasSensor`, `SetSensorEnabled`, `IsSensorEnabled`,
  `GetSensorData(type, count=3)`, `GetNumTouchpads`, `GetNumTouchpadFingers`,
  `GetTouchpadFinger` (-> `(state, x, y, pressure)`); ≥2.0.16:
  `GetSensorDataRate`, `SendEffect`; ≥2.0.18: `HasRumble`, `HasRumbleTriggers`,
  `GetAppleSFSymbolsNameFor{Button,Axis}` (`None` off Apple); ≥2.24:
  `GetFirmwareVersion`, `Path`; `GetSensorDataWithTimestamp` (≥2.26) ->
  `(timestamp_us, values)`; `GetSteamHandle` (≥2.30).
- GameController module: `GameControllerFromInstanceID`,
  `GameControllerFromPlayerIndex` / `…TypeForIndex` (≥2.0.12),
  `GameControllerNumMappings` / `…MappingForIndex` / `…MappingForGUID`,
  `…MappingForDeviceIndex` (≥2.0.9), `…PathForIndex` (≥2.24),
  `GameControllerGet{Axis,Button}FromString`, `GameControllerGetStringFor{Axis,Button}`.
  `GameControllerAddMappingsFromFile` now also takes the file's bytes (covers
  `AddMappingsFromRW`).
- `*FromInstanceID` / `*FromPlayerIndex` return a new **owned** wrapper: they
  re-open the device by index, which bumps SDL's refcount, so closing one handle
  never invalidates another. `None` when nothing matching is open.
- `_event()`: `CONTROLLERTOUCHPADDOWN/MOTION/UP` -> `(which, touchpad, finger, x,
  y, pressure)`, `CONTROLLERSENSORUPDATE` -> `(which, sensor, (x, y, z))`
  (≥2.0.14); `JOYBATTERYUPDATED` -> `(which, level)` (≥2.24);
  `CONTROLLERSTEAMHANDLEUPDATED` -> `(which,)` (≥2.30). Constants: those event
  types, `CONTROLLER_BINDTYPE_*`, the remaining `CONTROLLER_TYPE_*`,
  `JOYSTICK_AXIS_MIN/MAX`.

## Phase 11 - SDL_image completeness  — DONE

Tests: `tests/test_image.py` (hand-built GIF / TGA / SVG / XPM fixtures);
example: `example/image.py`.

- New `src/pysdl_Image.c` (`pysdl_image_methods`); `IMG_Init` / `IMG_Quit` and
  `LoadImage` moved there from `pysdl.c`. New module functions keep the `IMG_`
  prefix: `IMG_Linked_Version`, `IMG_is{BMP,CUR,GIF,ICO,JPG,LBM,PCX,PNG,PNM,TIF,
  WEBP,XCF,XPM,XV}`, `IMG_isSVG` (≥2.0.2), `IMG_is{AVIF,JXL,QOI}` (≥2.6),
  `IMG_ReadXPMFromArray(lines)`, `IMG_ReadXPMFromArrayToRGB888` /
  `IMG_LoadSizedSVG(src, w, h)` (≥2.6).
- `IMG_LoadAnimation(src, type=None)` (≥2.6) -> `(w, h, [(Surface, delay_ms),
  …])`. Frames are moved out of the `IMG_Animation` into owned Surfaces before
  `IMG_FreeAnimation`, so there is no animation type to manage.
- `LoadImage(src, type=None)` and `Renderer.LoadTexture(src, type=None)` take a
  path, os.PathLike or bytes. SDL_image detects every format with a magic number
  from the data, so `type` only matters for magic-less TGA; that makes the
  per-format `IMG_Load<FMT>_RW` / `IMG_LoadGIFAnimation_RW` /
  `IMG_LoadWEBPAnimation_RW` equivalent and they are not exposed separately.
- `Surface.SavePNG(path=None)` / `SaveJPG(path=None, quality=90)`: with no path
  they return the encoded `bytes` (via `IMG_Save*_RW` into a growable in-memory
  RWops, `PySDL_RWBuffer` in `pysdl_util.c`).
- Shared `PySDL_RWFromObject` (path or buffer -> RWops) now also backs `LoadBMP`,
  which gains os.PathLike support.

## Phase 12 - Render, surface & window completeness  — DONE

Tests: phase-12 sections in `tests/test_renderer.py`, `test_texture.py`,
`test_surface.py`, `test_video.py`; example: `example/logical.py`.

- **Renderer**: `RenderSetVSync`, `RenderWindowToLogical((x, y))` -> floats /
  `RenderLogicalToWindow((fx, fy))` -> ints (≥2.0.18), `RenderGetWindow`
  (borrowed, ≥2.0.22), `RenderGetMetalLayer` / `RenderGetMetalCommandEncoder`
  (pointer int or `None`, ≥2.0.8), and **`RenderGeometryRaw(texture, xy, color,
  uv=None, indices=None, *, num_vertices, xy_stride=8, color_stride=4,
  uv_stride=8, index_size=4)`** (≥2.0.18): zero-copy from any buffer (bytes,
  `array`, numpy); sizes, strides and float alignment are validated before SDL
  sees them, and `color_stride=0` gives every vertex one colour. `Renderer`
  gained `shouldFree` so `Window.GetRenderer()` can return a borrowed wrapper.
- Module: `CreateWindowAndRenderer(size, flags=0)` -> `(Window, Renderer)`,
  `CreateWindowFrom(native_handle)`, `CreateShapedWindow(title, size,
  position=…, flags=0)`, `VideoInit(driver=None)` / `VideoQuit`.
- **Texture**: `LockToSurface(rect=None)` (≥2.0.12) -> Surface over the locked
  region; `Unlock` swaps it for an empty 0x0 surface so it never dangles.
  `UpdateNV(yplane, ypitch, uvplane, uvpitch, rect=None)` (≥2.0.16), with plane
  sizes checked against the pitches and height.
- **Surface**: `HasColorKey` (≥2.0.9), `HasRLE` (≥2.0.14), `SoftStretchLinear`
  (≥2.0.16), `SaveBMP(path=None)` -> bytes with no path (like `SavePNG` /
  `SaveJPG`). The `w` / `h` / `pitch` / … getters now raise on an uninitialised
  Surface instead of dereferencing NULL.
- Pixels (module): `ConvertPixels(size, src_format, src, dst_format,
  src_pitch=0, dst_pitch=0)` / `PremultiplyAlpha(…)` (≥2.0.18) -> bytes (pitch 0
  = tight; planar and packed YUV destinations sized correctly),
  `CalculateGammaRamp(gamma)` -> 256 ints for `SetWindowGammaRamp`,
  `Set/GetYUVConversionMode`, `GetYUVConversionModeForResolution` (≥2.0.8).
- **Window**: `GetWindowSizeInPixels` (≥2.26), `GetWindowICCProfile` -> bytes or
  `None` (≥2.0.18), `HasWindowSurface` / `DestroyWindowSurface` (≥2.28),
  `GetWindowBrightness`, `GetRenderer`, `IsShapedWindow`, `SetWindowShape(shape,
  mode=SHAPEMODE_DEFAULT, param=None)`, `GetShapedWindowMode` -> `(mode,
  param)`. `GetWindowSurface` no longer leaks its wrapper on failure.
- Constants: `YUV_CONVERSION_*`, `SHAPEMODE_*`.
- **Skipped on purpose**: `Get/SetWindowData` and `Get/SetTextureUserData`
  (opaque `void*` slots; keep a Python dict keyed by the wrapper or window id),
  `SDL_LowerBlit` / `SDL_LowerBlitScaled` (unclipped fast paths that write out of
  bounds on bad rects; `Blit` / `BlitScaled` cover them safely).

## Phase 13 - Loose ends II  — DONE

Tests: phase-13 sections in `tests/test_events.py` (watchers, incl. a
cross-thread no-deadlock test), `test_video.py` (hint callbacks),
`test_rect.py`, `test_input.py`, `test_system.py`; example: `example/events.py`
(watcher + hint callback).

- **Event watchers**: `AddEventWatch(callback)` / `DelEventWatch(callback)`
  (by identity). Watchers run after the filter, only for events it keeps, on
  the pushing thread; return value ignored. A watcher may remove itself.
- **GIL rule for the event queue**: `PollEvent`, `PumpEvents`, `PushEvent`,
  `QuitRequested`, `SetEventFilter`, `Add/DelEventWatch` (and `ResetKeyboard`)
  now drop the GIL, since SDL calls filters/watchers under its watcher lock.
  `SetEventFilter` passes the callable as SDL userdata, which also fixes a race
  where a replaced filter could be freed while a call on another thread was
  about to use it.
- **Hint callbacks**: `AddHintCallback(name, callback)` -> `callback(name, old,
  new)` (called once immediately; re-adding replaces), `DelHintCallback(name,
  callback)`, `ResetHints` (≥2.26). `ClearHints` also drops the registrations.
- Clipboard: `Get/Set/HasPrimarySelectionText` (≥2.26).
- Keyboard: `ClearComposition`, `IsTextInputShown` (≥2.0.22), `ResetKeyboard`
  (≥2.24).
- Rect: `EncloseFPoints(points, clip=None)`, `IntersectFRectAndLine(frect, line)`
  (≥2.0.22). SDL 2 keeps integer edge semantics here (result is +1 wide/tall,
  a clip's far edge is `x + w - 1`).
- Sensors: `SensorFromInstanceID` (owned re-open, like the joystick lookups),
  `SensorGetDeviceNonPortableType` (≥2.0.9), `Lock/UnlockSensors` (≥2.0.14),
  `Sensor.GetDataWithTimestamp(count=6)` (≥2.26). Haptic: `HapticOpened`.
- Misc: `HasRDTSC`, `GetErrorMsg` (≥2.0.14).
- **Not wrapped, deliberately**: the legacy single-device audio API
  (`SDL_OpenAudio`, `SDL_PauseAudio`, `SDL_LockAudio`, `SDL_MixAudio`,
  `SDL_BuildAudioCVT` / `SDL_ConvertAudio`, …: superseded by `SDL2.Audio`,
  `MixAudioFormat` and `AudioStream`), `SDL_GetRevisionNumber` (deprecated,
  always 0), `SDL_SIMDAlloc` / `SIMDRealloc` / `SIMDFree` (raw C allocation),
  `SDL_Error` (internal).

## Phase 14 - SDL2_ttf (optional)  — DONE

Tests: `tests/test_ttf.py` (skips without SDL2_ttf or a system font); example:
`example/text.py`.

- **Optional dependency**: `setup.py` probes `pkg-config SDL2_ttf` (then the
  header); when found it links `SDL2_ttf` and defines `PYSDL_HAVE_TTF`.
  `PYSDL_TTF=0` builds without it. Without it `SDL2.Font` / `TTF_*` are simply
  absent. Added `SDL_TTF_VERSION_ATLEAST` / `SDL_IMAGE_VERSION_ATLEAST`
  fallbacks in `pysdl.h` for releases that predate those macros.
- New `src/pysdl_Font.c`. Module (keeps the `TTF_` prefix like `IMG_`):
  `TTF_Init`, `TTF_Quit`, `TTF_WasInit`, `TTF_Linked_Version`,
  `TTF_GetFreeTypeVersion` / `TTF_GetHarfBuzzVersion` (≥2.0.18).
- **`SDL2.Font(src, ptsize=12, index=0, *, hdpi=0, vdpi=0)`**: `src` is a path,
  os.PathLike or the font file's bytes (pinned for the font's lifetime). Methods
  drop `TTF_` and `Font`: `Close`, `Get/SetStyle`, `Get/SetOutline`,
  `Get/SetHinting`, `Get/SetKerning`, `Height`, `Ascent`, `Descent`,
  `LineSkip`, `Faces`, `FaceIsFixedWidth`, `FaceFamilyName`, `FaceStyleName`,
  `GlyphIsProvided(ch)`, `GlyphMetrics(ch)` -> `(minx, maxx, miny, maxy,
  advance)`, `Size(text)` -> `(w, h)`, `GetKerningSize(a, b)` (≥2.0.14);
  ≥2.0.18: `SetSize`, `SetSizeDPI`, `Get/SetSDF`, `Measure(text, width)` ->
  `(extent, count)`; ≥2.20: `Get/SetWrappedAlign`, `SetDirection`,
  `SetScriptName`.
- Rendering -> owned `Surface`: `RenderSolid(text, fg, wrap_length=None)`,
  `RenderShaded(text, fg, bg, wrap_length=None)`, `RenderBlended(text, fg,
  wrap_length=None)`, `RenderLCD(text, fg, bg, wrap_length=None)` (≥2.20); a
  `wrap_length` selects the `*_Wrapped` variant (≥2.0.18; 0 = newlines only).
  `RenderGlyph{Solid,Shaded,Blended,LCD}(ch, fg[, bg])`. `ch` is a 1-char str or
  an int codepoint (32-bit functions from 2.0.18).
- Text is a Python `str` passed as UTF-8, so the Latin-1 (`TTF_*Text*`) and
  UCS-2 (`TTF_*UNICODE*`, `TTF_ByteSwappedUNICODE`) variants and the 16-bit
  glyph functions are not exposed; nor are the deprecated global
  `TTF_SetDirection` / `TTF_SetScript` (use the per-font setters).
- **Lifetime**: a font used after the final `TTF_Quit` raises instead of
  touching freed FreeType state, and is never passed to `TTF_CloseFont` (see
  the session counter in `pysdl_Font.c`). The GIL stays held: a `TTF_Font` is
  not thread-safe.
- Constants: `TTF_STYLE_*`, `TTF_HINTING_*`, `TTF_WRAPPED_ALIGN_*`,
  `TTF_DIRECTION_*`.
- **Bug fix**: `Texture.UpdateYUV` now checks each plane buffer against its
  pitch and the update height (as `UpdateNV` does) instead of letting SDL read
  past the end of a short buffer.

With this phase every in-scope SDL2 header is covered; what remains unwrapped is
listed in the phase notes above ("skipped on purpose" / "not wrapped,
deliberately") or below.

## Phase 15 - Wrapper invalidation  — DONE

Tests: teardown / tracking section of `tests/test_video.py` (the `Quit` cases
run in a subprocess).

- **Window surfaces no longer dangle.** `GetWindowSurface()` returns the same
  Surface object while SDL keeps the same surface (across all wrappers of that
  window); when SDL replaces it after a resize, destroys it
  (`DestroyWindowSurface`) or destroys the window, the old Surface is emptied to
  0x0 instead of pointing at freed memory.
- **Teardown invalidation.** `Quit`, `VideoQuit`, `VideoInit` and a
  `QuitSubSystem` that really stops video null every `Window` wrapper and empty
  every window Surface, so wrappers kept past a shutdown raise instead of
  touching freed windows — also after re-initialising, when window ids are
  reused. Freeing the owning `Window` likewise invalidates borrowed wrappers of
  it (`GetWindowFromID`, `GetKeyboardFocus`, …).
- Shared pieces: `PySDL_Registry` (`pysdl_util.c`), `PySDL_SurfaceDetach`
  (`pysdl_Surface.c`, now also used by `Texture.Unlock`), `Surface.window_id`.
- **Bug fix**: `WasInit`, `Window.SetWindowFullscreen`, `SetWindowPosition` and
  `SetWindowSize` tested `PyArg_ParseTuple`'s result as `0 > ok` (it returns 0
  on failure), so bad arguments ran SDL on uninitialised values and surfaced as
  `SystemError`; they now raise `TypeError`.
- **Textures outliving their renderer.** `SDL_DestroyRenderer` frees every
  texture of that renderer, so a `Texture` kept past its `Renderer` (`del
  renderer`, or module teardown order at interpreter exit) used to dangle and
  its dealloc called `SDL_DestroyTexture` on freed memory. Texture wrappers now
  record their renderer (`PySDL_TextureTrack`); the owning Renderer's dealloc
  invalidates them (methods raise "Invalid texture", a `LockToSurface` Surface
  is emptied) and its borrowed wrappers (`Window.GetRenderer()`) before
  destroying. `SetRenderTarget` / `RenderGeometry` / `RenderGeometryRaw` raise
  on an invalidated texture rather than treating it as "no texture".
- **Cursors.** `SDL_FreeCursor` and video shutdown free cursors (shutdown frees
  *every* cursor, including ones we own). Cursor wrappers are now registered:
  an owner's dealloc invalidates borrowed wrappers of its cursor, video
  teardown invalidates all of them (owned ones are then not freed twice), and
  `GetCursor()` returns the owning `Cursor` object for our own cursors.
  `SetCursor` / `Cursor.Set` raise on an invalid cursor instead of passing NULL
  (which SDL reads as "redraw").
- **`GameController.GetJoystick()`** now returns an owned `Joystick` (re-opened
  by index, refcounted) instead of a borrowed pointer that dangled once the
  controller was closed; `None` if the device is gone. Nothing borrows a
  joystick any more, so `Joystick.shouldFree` was removed.

Every borrowed pointer the binding hands out is now either owned (joysticks,
controllers, sensors via re-open), tracked and invalidated (windows, window
surfaces, renderers, textures, locked texture surfaces, cursors), or SDL's own
never-freed object (the default cursor).

## Phase 16 - SDL2_mixer (optional)  — DONE

Tests: `tests/test_mixer.py` (dummy audio driver; skips without SDL2_mixer);
example: `example/mixer.py`.

- `setup.py`'s optional-library probe is now a table (`OPTIONAL`): SDL2_mixer
  joins SDL2_ttf (`PYSDL_HAVE_MIXER`, opt out with `PYSDL_MIXER=0`).
- New `src/pysdl_Mixer.c`: **`SDL2.Chunk(src)`** (path or bytes, decoded to the
  device format; `Volume(volume=-1)`, `Free`) and **`SDL2.Music(src,
  type=MUS_NONE)`** (path or pinned bytes; `Play(loops=0)`, `FadeIn(ms, loops=0,
  position=0.0)`, `GetType`, `Free`; ≥2.6: `GetTitle`, `GetTitleTag`,
  `GetArtistTag`, `GetAlbumTag`, `GetCopyrightTag`, `Duration`, `GetPosition`,
  `GetLoopStartTime` / `EndTime` / `LengthTime`, `GetVolume`; ≥2.8:
  `StartTrack`, `GetNumTracks`).
- `Mix_*` module functions (prefix kept, like `IMG_` / `TTF_`): `Init`, `Quit`,
  `Linked_Version`, `OpenAudio(frequency, format, channels, chunksize, device,
  allowed_changes)` (`Mix_OpenAudioDevice`), `CloseAudio`, `QuerySpec`,
  `PauseAudio` (≥2.8), `AllocateChannels`, `ReserveChannels`,
  `QuickLoad_RAW(data)` (pinned); decoders (`GetNum/Get/HasChunkDecoder`,
  `…MusicDecoder`); channels (`PlayChannel(channel, chunk, loops=0,
  ticks=-1)`, `FadeInChannel`, `GetChunk`, `Volume`, `HaltChannel`,
  `ExpireChannel`, `FadeOutChannel`, `FadingChannel`, `Pause`, `Resume`,
  `Paused`, `Playing`); groups (`GroupChannel(s)`, `GroupAvailable`,
  `GroupCount`, `GroupOldest`, `GroupNewer`, `HaltGroup`, `FadeOutGroup`);
  positional effects (`SetPanning`, `SetPosition`, `SetDistance`,
  `SetReverseStereo`); custom effects (`RegisterEffect(channel, effect,
  done=None)`, `UnregisterEffect`, `UnregisterAllEffects`); music
  (`VolumeMusic`, `MasterVolume` (≥2.6), `HaltMusic`, `FadeOutMusic`,
  `FadingMusic`, `Pause/Resume/Rewind/PausedMusic`, `PlayingMusic`,
  `SetMusicPosition`, `ModMusicJumpToOrder` (≥2.6), `SetMusicCMD`,
  `Set/GetSynchroValue`); MIDI (`Set/GetSoundFonts`, `EachSoundFont`,
  `Set/GetTimidityCfg` (≥2.6)); callbacks (`ChannelFinished`,
  `HookMusicFinished`, `SetPostMix`, `HookMusic`, `GetMusicHookData`).
- **Threading**: every call that can lock the audio device releases the GIL;
  callbacks run through `PySDL_ThreadEnter`, hold their own reference to the
  callable, and exchange audio as `bytes`. Verified by a subprocess stress test
  (3000 rapid calls against re-entrant callbacks, under a timeout).
- **Lifetimes**: playing chunks / music are kept alive by the binding until
  their channel is reused or audio closes; bytes-backed music and
  `QuickLoad_RAW` chunks pin their buffers; `Mix_Quit` invalidates older `Music`
  (session counter, as for fonts).
- **Not wrapped, deliberately**: `Mix_QuickLoad_WAV` (no length parameter: a
  short buffer is read out of bounds; `SDL2.Chunk(bytes)` is the safe
  equivalent); `Mix_LoadWAV` / `Mix_LoadMUS_RW` / `Mix_LoadMUSType_RW` /
  `Mix_FadeInMusic` / `Mix_PlayChannel` / `Mix_FadeInChannel` macro and
  non-`Timed` / non-`Pos` variants (covered by the path-or-bytes constructors
  and the optional `ticks` / `position` arguments).
- Constants: `MIX_INIT_*`, `MIX_CHANNELS`, `MIX_DEFAULT_*`, `MIX_MAX_VOLUME`,
  `MIX_CHANNEL_POST`, `MIX_EFFECTSMAXSPEED` (a hint name), `MIX_NO_FADING` /
  `MIX_FADING_OUT` / `MIX_FADING_IN`, `MUS_*`.

## Explicitly out of scope

Threads / mutexes / semaphores / condition vars / atomics (use Python's),
`SDL_Log*`, assertions, `SDL_main` / main callbacks, platform-specific APIs
(Android / iOS / WinRT), `GetWindowWMInfo`, stdinc shims, `SDL_hid_*` (HIDAPI:
use a Python `hid` package), `SDL_LoadObject` / `SDL_LoadFunction` /
`SDL_UnloadObject` (use `ctypes`). The other satellite
libraries (`SDL2_net`, `SDL2_gfx`) remain a separate effort; if added, follow
the Phase 14 / 16 pattern (optional, probed by `setup.py`, one
`#ifdef PYSDL_HAVE_<LIB>` file).

## Per-phase checklist

Each phase ships: new constants, `.tp_doc` + terse method docstrings matching the
existing style, one `example/` script, one headless `tests/test_<area>.py`,
and a README coverage note.
