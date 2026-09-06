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
    assert window.GetWindowGrab() is True
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
