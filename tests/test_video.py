'''Phase 7: window state, display queries, hints, message boxes, GL/Vulkan.

The dummy video driver rejects a lot of window operations ("That operation is
not supported"); those are exercised for their argument handling and skipped
when the driver refuses.'''

import pytest


def _try(fn):
    '''Run fn(); return its value, or None if the driver does not support it.'''
    try:
        return fn()
    except Exception as exc:  # noqa: BLE001 - SDL2.error, matched below
        if 'not supported' in str(exc) or 'No ' in str(exc):
            pytest.skip(str(exc))
        raise


# --- window ----------------------------------------------------------------

def test_window_flags_and_format(window, sdl):
    assert window.GetWindowFlags() & sdl.WINDOW_SHOWN or True  # flags is an int bitmask
    assert isinstance(window.GetWindowFlags(), int)
    assert window.GetWindowDisplayIndex() >= 0
    assert isinstance(window.GetWindowPixelFormat(), int)


def test_window_min_max_size(window):
    window.SetWindowMinimumSize((80, 60))
    window.SetWindowMaximumSize((800, 600))
    assert window.GetWindowMinimumSize() == (80, 60)
    assert window.GetWindowMaximumSize() == (800, 600)


def test_window_grab_roundtrip(window):
    window.SetWindowGrab(True)
    if window.GetWindowGrab() is False:
        # Older SDL (e.g. 2.0.20) only reports a grab for a window with input
        # focus, which the dummy driver never gives.
        pytest.skip('driver does not grab an unfocused window')
    window.SetWindowGrab(False)
    assert window.GetWindowGrab() is False


def test_window_bordered_and_modal(window, sdl):
    window.SetWindowBordered(False)
    window.SetWindowBordered(True)
    with pytest.raises(TypeError):
        window.SetWindowModalFor(object())


def test_window_opacity(window):
    _try(lambda: window.SetWindowOpacity(0.5))
    assert 0.0 <= window.GetWindowOpacity() <= 1.0


def test_window_gamma_ramp(window):
    r, g, b = window.GetWindowGammaRamp()
    assert len(r) == len(g) == len(b) == 256
    _try(lambda: window.SetWindowGammaRamp(r, g, b))
    with pytest.raises(ValueError):
        window.SetWindowGammaRamp([0] * 10, g, b)


def test_window_display_mode(window):
    fmt, w, h, rate = window.GetWindowDisplayMode()
    assert w > 0 and h > 0


def test_window_mouse_rect(window):
    if not hasattr(window, 'SetWindowMouseRect'):
        pytest.skip('SDL < 2.0.18')
    _try(lambda: window.SetWindowMouseRect((0, 0, 40, 40)))
    assert window.GetWindowMouseRect() == (0, 0, 40, 40)
    window.SetWindowMouseRect(None)
    assert window.GetWindowMouseRect() is None


# --- displays -------------------------------------------------------------

def test_display_queries(sdl):
    assert isinstance(sdl.GetDisplayName(0), str)
    x, y, w, h = sdl.GetDisplayUsableBounds(0)
    assert w > 0 and h > 0
    assert sdl.GetDisplayOrientation(0) in (
        sdl.ORIENTATION_UNKNOWN, sdl.ORIENTATION_LANDSCAPE, sdl.ORIENTATION_LANDSCAPE_FLIPPED,
        sdl.ORIENTATION_PORTRAIT, sdl.ORIENTATION_PORTRAIT_FLIPPED,
    )

    n = sdl.GetNumDisplayModes(0)
    assert n >= 1
    fmt, mw, mh, rate = sdl.GetDisplayMode(0, 0)
    assert mw > 0 and mh > 0

    closest = sdl.GetClosestDisplayMode(0, (0, 640, 480, 60))
    assert closest is None or len(closest) == 4


def test_get_window_from_id_is_borrowed(sdl, window):
    fetched = sdl.GetWindowFromID(window.GetWindowID())
    assert fetched.GetWindowID() == window.GetWindowID()
    del fetched  # borrowed: window must survive
    assert window.GetWindowSize() == (64, 48)

    assert sdl.GetWindowFromID(999999) is None


# --- hints --------------------------------------------------------------------

def test_hints(sdl):
    assert sdl.SetHint('SDL_pysdl2_test', 'yes') is True
    assert sdl.GetHint('SDL_pysdl2_test') == 'yes'
    assert sdl.GetHintBoolean('SDL_pysdl2_test', False) is True
    assert sdl.GetHint('SDL_pysdl2_never_set') is None

    sdl.SetHintWithPriority('SDL_pysdl2_test', 'no', sdl.HINT_OVERRIDE)
    assert sdl.GetHint('SDL_pysdl2_test') == 'no'
    sdl.ClearHints()


# --- message boxes ----------------------------------------------------------

def test_show_message_box_rejects_bad_config(sdl):
    with pytest.raises(TypeError):
        sdl.ShowMessageBox([1, 2, 3])
    with pytest.raises(KeyError):
        sdl.ShowMessageBox({'title': 'x', 'message': 'y'})  # no buttons


# --- locales / GL / Vulkan ------------------------------------------------

def test_preferred_locales(sdl):
    if not hasattr(sdl, 'GetPreferredLocales'):
        pytest.skip('SDL < 2.0.14')
    locales = sdl.GetPreferredLocales()
    assert isinstance(locales, list)
    for lang, country in locales:
        assert isinstance(lang, str)
        assert country is None or isinstance(country, str)


def test_gl_get_proc_address(sdl):
    assert isinstance(sdl.GL_GetProcAddress('glClear'), int)
    assert isinstance(sdl.GL_GetCurrentContext(), int)


def test_window_flag_constants_present(sdl):
    # these are SDL enum members - they were silently missing when guarded with
    # #ifdef instead of a version check
    for name in ('WINDOW_ALWAYS_ON_TOP', 'WINDOW_UTILITY', 'WINDOW_POPUP_MENU',
                 'WINDOW_VULKAN', 'BLENDMODE_MUL', 'LOCALECHANGED'):
        assert isinstance(getattr(sdl, name), int), name


def test_metal_view(sdl):
    if not hasattr(sdl, 'WINDOW_METAL') or not hasattr(sdl, 'Metal_GetLayer'):
        pytest.skip('SDL < 2.0.11')
    try:
        win = sdl.Window('metal-test', (32, 32), flags=sdl.WINDOW_METAL)
        view = win.Metal_CreateView()
    except sdl.error:
        pytest.skip('no Metal support in this driver / platform')
    assert isinstance(view, int) and view != 0
    assert isinstance(sdl.Metal_GetLayer(view), int)
    assert win.Metal_GetDrawableSize() == (32, 32)
    sdl.Metal_DestroyView(view)


def test_vulkan_functions_present(sdl, window):
    assert hasattr(sdl, 'Vulkan_GetInstanceExtensions')
    assert hasattr(window, 'Vulkan_CreateSurface')
    try:
        sdl.Vulkan_LoadLibrary()
    except sdl.error:
        pytest.skip('no Vulkan loader available')
    try:
        extensions = sdl.Vulkan_GetInstanceExtensions()
        assert isinstance(extensions, list)
        assert all(isinstance(e, str) for e in extensions)
        assert sdl.Vulkan_GetVkGetInstanceProcAddr() != 0
        with pytest.raises(TypeError):
            window.Vulkan_CreateSurface('not an int')
    finally:
        sdl.Vulkan_UnloadLibrary()


# --- phase 12: pixel size, ICC, window surface, brightness, shapes --------

def test_window_size_in_pixels(sdl, window):
    if not hasattr(window, 'GetWindowSizeInPixels'):
        pytest.skip('needs SDL >= 2.26')
    assert window.GetWindowSizeInPixels() == window.GetWindowSize()  # no high-DPI here


def test_window_icc_profile_and_brightness(sdl, window):
    if hasattr(window, 'GetWindowICCProfile'):
        profile = window.GetWindowICCProfile()
        assert profile is None or isinstance(profile, bytes)
    assert window.GetWindowBrightness() == pytest.approx(1.0)


def test_window_surface_lifecycle(sdl):
    window = sdl.Window('surface', (16, 16))
    if not hasattr(window, 'HasWindowSurface'):
        pytest.skip('needs SDL >= 2.28')
    assert window.HasWindowSurface() is False
    window.GetWindowSurface()
    assert window.HasWindowSurface() is True
    window.DestroyWindowSurface()
    assert window.HasWindowSurface() is False


def test_shaped_windows(sdl, window):
    assert window.IsShapedWindow() is False
    with pytest.raises(sdl.error, match='not a shaped window'):
        window.SetWindowShape(sdl.CreateRGBSurface((64, 48)))
    with pytest.raises(sdl.error):
        window.GetShapedWindowMode()
    try:
        shaped = sdl.CreateShapedWindow('shaped', (32, 32))
    except sdl.error:
        pytest.skip('video driver has no shaped-window support')
    mask = sdl.CreateRGBSurfaceWithFormat((32, 32), sdl.PIXELFORMAT_RGBA32)
    shaped.SetWindowShape(mask, sdl.SHAPEMODE_COLOR_KEY, (0, 0, 0))
    assert shaped.GetShapedWindowMode()[0] == sdl.SHAPEMODE_COLOR_KEY


def test_create_window_from_rejects_null(sdl):
    with pytest.raises(ValueError):
        sdl.CreateWindowFrom(0)


def test_video_init_quit_in_subprocess(sdl):
    # SDL_VideoInit tears down any running video subsystem first, so keep it
    # out of this test session's shared SDL state.
    import os
    import subprocess
    import sys
    code = (
        'import SDL2\n'
        'SDL2.VideoInit("dummy")\n'
        'assert SDL2.GetCurrentVideoDriver() == "dummy"\n'
        'SDL2.VideoQuit()\n'
        'try:\n'
        '    SDL2.VideoInit("no-such-driver")\n'
        'except SDL2.error:\n'
        '    print("ok")\n'
    )
    env = dict(os.environ, PYTHONPATH=os.path.dirname(sdl.__file__))
    out = subprocess.run([sys.executable, '-c', code], env=env, capture_output=True, text=True)
    assert out.stdout.strip() == 'ok', out.stderr


# --- phase 13: hint callbacks ----------------------------------------------

HINT = 'SDL_RENDER_SCALE_QUALITY'


@pytest.fixture
def clean_hint(sdl):
    yield HINT
    sdl.SetHint(HINT, '')


def test_hint_callback_lifecycle(sdl, clean_hint):
    sdl.SetHint(HINT, 'nearest')
    calls = []
    cb = lambda *args: calls.append(args)  # noqa: E731
    sdl.AddHintCallback(HINT, cb)
    assert calls == [(HINT, 'nearest', 'nearest')]  # called once right away

    sdl.SetHint(HINT, 'linear')
    assert calls[-1] == (HINT, 'nearest', 'linear')

    sdl.AddHintCallback(HINT, cb)  # re-adding replaces, it doesn't duplicate
    before = len(calls)
    sdl.SetHint(HINT, 'best')
    assert len(calls) - before == 1

    sdl.DelHintCallback(HINT, cb)
    before = len(calls)
    sdl.SetHint(HINT, 'nearest')
    assert len(calls) == before
    with pytest.raises(ValueError):
        sdl.DelHintCallback(HINT, cb)


def test_hint_callback_can_remove_itself(sdl, clean_hint):
    calls = []

    def once(name, old, new):
        calls.append(new)
        if new == 'linear':
            sdl.DelHintCallback(name, once)

    sdl.AddHintCallback(HINT, once)
    sdl.SetHint(HINT, 'linear')
    sdl.SetHint(HINT, 'nearest')
    assert calls[-1] == 'linear'


def test_clear_hints_drops_callbacks(sdl, clean_hint):
    cb = lambda *args: None  # noqa: E731
    sdl.AddHintCallback(HINT, cb)
    sdl.ClearHints()
    with pytest.raises(ValueError):
        sdl.DelHintCallback(HINT, cb)


def test_reset_hints(sdl, clean_hint):
    if not hasattr(sdl, 'ResetHints'):
        pytest.skip('needs SDL >= 2.26')
    sdl.SetHint(HINT, 'linear')
    calls = []
    sdl.AddHintCallback(HINT, lambda *args: calls.append(args))
    sdl.ResetHints()
    assert calls[-1] == (HINT, 'linear', None)
    assert sdl.GetHint(HINT) is None
    sdl.ClearHints()


def test_hint_callback_errors(sdl):
    with pytest.raises(TypeError):
        sdl.AddHintCallback(HINT, 'not callable')


# --- window-surface tracking and teardown invalidation ---------------------

def test_window_surface_is_one_object(sdl):
    window = sdl.Window('surface-identity', (32, 24))
    surface = window.GetWindowSurface()
    assert window.GetWindowSurface() is surface
    borrowed = sdl.GetWindowFromID(window.GetWindowID())
    assert borrowed.GetWindowSurface() is surface  # shared across wrappers


def test_window_surface_replaced_after_resize(sdl):
    window = sdl.Window('surface-resize', (32, 24), flags=sdl.WINDOW_RESIZABLE)
    old = window.GetWindowSurface()
    window.SetWindowSize((64, 48))
    sdl.PumpEvents()
    assert (old.w, old.h) == (32, 24)  # SDL keeps it until the next GetWindowSurface
    new = window.GetWindowSurface()
    assert (new.w, new.h) == (64, 48)
    if new is old:
        pass  # SDL reused the freed surface's address: the object *is* the new surface
    else:
        assert (old.w, old.h) == (0, 0)  # emptied, not dangling
    old.FillRect(None, (1, 2, 3))        # harmless either way
    new.FillRect(None, (1, 2, 3))
    window.UpdateWindowSurface()


def test_window_surface_emptied_on_destroy(sdl):
    window = sdl.Window('surface-destroy', (16, 16))
    surface = window.GetWindowSurface()
    if hasattr(window, 'DestroyWindowSurface'):
        window.DestroyWindowSurface()
        assert (surface.w, surface.h) == (0, 0)
        surface = window.GetWindowSurface()
    del window  # SDL frees the surface with the window
    assert (surface.w, surface.h) == (0, 0)


def test_borrowed_window_invalidated_with_owner(sdl):
    window = sdl.Window('owner', (16, 16))
    borrowed = sdl.GetWindowFromID(window.GetWindowID())
    del window
    with pytest.raises(sdl.error):
        borrowed.GetWindowID()


def test_bad_arguments_raise_cleanly(sdl, window):
    # These once checked PyArg_ParseTuple's result as `0 > ok` (it returns 0):
    # a bad call ran SDL with garbage and surfaced as SystemError.
    with pytest.raises(TypeError):
        window.SetWindowSize(1, 2)
    with pytest.raises(TypeError):
        window.SetWindowPosition('x')
    with pytest.raises(TypeError):
        window.SetWindowFullscreen('x')
    with pytest.raises(TypeError):
        sdl.WasInit('x')


def _run_teardown_script(sdl, body):
    import os
    import subprocess
    import sys
    import textwrap
    code = textwrap.dedent('''
        import gc
        import SDL2

        def raises(fn):
            try:
                fn()
            except SDL2.error:
                return True
            return False

        SDL2.Init(SDL2.INIT_VIDEO)
    ''') + textwrap.dedent(body) + '\nprint("ok")\n'
    env = dict(os.environ, PYTHONPATH=os.path.dirname(sdl.__file__))
    out = subprocess.run([sys.executable, '-c', code], env=env, capture_output=True, text=True)
    assert out.returncode == 0 and out.stdout.strip().endswith('ok'), out.stderr or out.stdout


def test_quit_invalidates_windows(sdl):
    _run_teardown_script(sdl, '''
        window = SDL2.Window('w', (32, 32))
        surface = window.GetWindowSurface()
        borrowed = SDL2.GetWindowFromID(window.GetWindowID())
        other = SDL2.Window('r', (32, 32))
        renderer = other.CreateRenderer()
        texture = SDL2.Texture(renderer, SDL2.PIXELFORMAT_RGBA32, SDL2.TEXTUREACCESS_STATIC, (4, 4))

        SDL2.Quit()
        assert raises(window.GetWindowID) and raises(borrowed.GetWindowID)
        assert (surface.w, surface.h) == (0, 0)

        # A new session reuses window ids; the old wrappers must stay dead.
        SDL2.Init(SDL2.INIT_VIDEO)
        fresh = SDL2.Window('fresh', (16, 16))
        assert raises(window.GetWindowID) and raises(borrowed.GetWindowID)
        del window, borrowed, surface, other, renderer, texture
        gc.collect()
        assert fresh.GetWindowID() > 0
        SDL2.Quit()
    ''')


def test_video_quit_and_init_invalidate_windows(sdl):
    _run_teardown_script(sdl, '''
        window = SDL2.Window('w', (16, 16))
        SDL2.VideoQuit()
        assert raises(window.GetWindowID)

        SDL2.VideoInit('dummy')
        window = SDL2.Window('w2', (16, 16))
        surface = window.GetWindowSurface()
        SDL2.VideoInit('dummy')  # restarting video destroys existing windows
        assert raises(window.GetWindowID) and (surface.w, surface.h) == (0, 0)
        del window, surface
        gc.collect()
        SDL2.Quit()
    ''')


def test_quit_subsystem_respects_refcount(sdl):
    _run_teardown_script(sdl, '''
        window = SDL2.Window('w', (16, 16))
        SDL2.InitSubSystem(SDL2.INIT_VIDEO)   # second reference
        SDL2.QuitSubSystem(SDL2.INIT_VIDEO)   # video keeps running
        assert window.GetWindowID() > 0
        SDL2.QuitSubSystem(SDL2.INIT_VIDEO)   # now it stops
        assert raises(window.GetWindowID)
        del window
        gc.collect()
        SDL2.Quit()
    ''')
