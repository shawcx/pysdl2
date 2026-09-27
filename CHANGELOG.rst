Changelog
=========

0.3.0 (2026-09-27)
------------------

Coverage grows from a hand-picked subset to essentially the whole SDL2 API,
plus SDL2_image and the optional SDL2_ttf and SDL2_mixer. ``docs/ROADMAP.md``
records each phase in detail and every deliberate omission with its reason.

Requirements
^^^^^^^^^^^^

* Python 3.10 or newer.
* SDL2 and SDL2_image as before; SDL2_ttf and SDL2_mixer are optional and
  built in automatically when found (``PYSDL_TTF=0`` / ``PYSDL_MIXER=0`` to
  leave one out). ``setup.py`` now locates SDL through ``sdl2-config`` /
  ``pkg-config``.

New
^^^

* Rendering: renderer / texture construction, all primitives (int or float),
  geometry (including zero-copy ``RenderGeometryRaw``), render targets,
  logical size and window/logical coordinate mapping, vsync, texture locking
  (``Lock``, ``LockToSurface``), YUV / NV12 uploads.
* Surfaces and pixels: creation from buffers, blits, fills, conversion, colour
  keys, palettes, pixel formats, ``ConvertPixels``, ``PremultiplyAlpha``;
  ``SaveBMP`` / ``SavePNG`` / ``SaveJPG`` return ``bytes`` when no path is given.
* Input: keyboard, mouse, touch, text input / IME, cursors; joysticks and game
  controllers in full (virtual joysticks with Python callbacks, sensors,
  touchpads, rumble, LEDs, mappings, GUIDs); haptics; sensors.
* Events: the whole queue API, event filters and watchers, custom events,
  every structured event type decoded to a tuple.
* Video: window state and display queries, message boxes, hints (with change
  callbacks), shaped windows, high-DPI sizes, ICC profiles, GL / Vulkan / Metal
  loaders.
* Audio: devices, queueing, ``LoadWAV``, ``AudioStream`` resampling, mixing.
* Timers, power, clipboard (incl. primary selection), locales, rect geometry.
* SDL2_image: format probes, typed loading, animations, sized SVG, XPM.
* SDL2_ttf (optional): ``SDL2.Font`` with styles, metrics, wrapped text and the
  four render modes.
* SDL2_mixer (optional): ``SDL2.Chunk`` / ``SDL2.Music`` and the ``Mix_*``
  functions, including channel / music callbacks, post-mix hooks and custom
  effects.
* Loaders accept a path, an ``os.PathLike`` or the file's ``bytes``.

Behaviour changes
^^^^^^^^^^^^^^^^^

* ``Window.CreateRenderer(flags=0, index=-1)`` no longer forces
  ``RENDERER_ACCELERATED | RENDERER_PRESENTVSYNC``; pass the flags you want.
* ``GetDisplayMode(display, mode=0)`` takes a mode index.
* ``Texture.GL_Bind()`` returns the texture's ``(w, h)`` scale.
* ``GameController.GetJoystick()`` returns an independently owned
  ``Joystick`` (``None`` if the device is gone).
* Objects SDL has destroyed no longer point at freed memory: window surfaces,
  windows, renderers, textures and cursors are invalidated (their methods raise
  ``SDL2.error``) when a resize, ``Quit``, ``VideoQuit``, or the destruction of
  their owner frees them. A window surface replaced after a resize becomes an
  empty 0x0 surface (or, if SDL reuses its address, simply is the new surface).
* Invalid arguments to ``WasInit``, ``SetWindowFullscreen``,
  ``SetWindowPosition`` and ``SetWindowSize`` raise ``TypeError`` (they used to
  run SDL on uninitialised values and surface as ``SystemError``).
* Event filter / watcher callbacks and every call that can run them release the
  GIL, fixing deadlocks with events pushed from SDL's own threads.

Fixed
^^^^^

* Builds against every SDL release from 2.0.20 up: version guards that named
  the wrong release are corrected (the Metal functions and ``WINDOW_METAL``
  need 2.0.14, ``CONTROLLER_TYPE_AMAZON_LUNA`` / ``GOOGLE_STADIA`` 2.0.16,
  ``CONTROLLER_TYPE_MAX`` 2.30), and ``POLLSENTINEL`` is available from 2.0.18.
  ``tools/check_version_guards.py`` checks every guard against the real
  headers, in CI.
* ``Texture.UpdateYUV`` / ``UpdateNV`` check plane buffer sizes instead of
  letting SDL read past a short buffer.

Tested with
^^^^^^^^^^^

SDL 2.0.20 / SDL_image 2.0.5 / SDL_ttf 2.0.18 / SDL_mixer 2.0.4 (Ubuntu 22.04,
Python 3.10), SDL 2.26 / 2.6 / 2.20 / 2.6 (Debian 12, Python 3.13) and SDL 2.30
/ 2.8 / 2.22 / 2.8 (Ubuntu 24.04, Python 3.12).

0.2.1 (2020-05-11)
------------------

Previous release.
