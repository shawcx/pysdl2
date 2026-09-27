'''Phase 3: SDL2.Cursor. The dummy video driver has no cursor support, so most
of these skip there; they still exercise the argument checking and error paths.'''

import pytest


@pytest.fixture
def system_cursor(sdl):
    try:
        return sdl.Cursor(sdl.SYSTEM_CURSOR_HAND)
    except sdl.error:
        pytest.skip('cursors unsupported under this driver')


def test_system_cursor_set(system_cursor):
    system_cursor.Set()


def test_uninitialised_cursor_set_is_guarded(sdl):
    with pytest.raises(sdl.error):
        sdl.Cursor().Set()


def test_color_cursor(sdl):
    surface = sdl.CreateRGBSurface((16, 16))
    surface.FillRect(None, (255, 255, 255, 255))
    try:
        cursor = sdl.CreateColorCursor(surface, 8, 8)
    except sdl.error:
        pytest.skip('cursors unsupported under this driver')
    cursor.Set()

    with pytest.raises(TypeError):
        sdl.CreateColorCursor(object())


def test_create_cursor_validates_buffer_size(sdl):
    with pytest.raises(ValueError):
        sdl.CreateCursor(b'\x00', b'\x00', (16, 16), (0, 0))


def test_create_cursor_from_bitmap(sdl):
    data = bytes((16 + 7) // 8 * 16)
    mask = bytes((16 + 7) // 8 * 16)
    try:
        cursor = sdl.CreateCursor(data, mask, (16, 16), (0, 0))
    except sdl.error:
        pytest.skip('cursors unsupported under this driver')
    cursor.Set()


def test_get_cursor_borrowed(sdl):
    default = sdl.GetDefaultCursor()
    active = sdl.GetCursor()
    for cur in (default, active):
        assert cur is None or type(cur).__name__ == 'Cursor'
    # deallocating a borrowed cursor must not free SDL's copy
    del default, active
    sdl.SetCursor(None)


def test_set_cursor_rejects_non_cursor(sdl):
    with pytest.raises(TypeError):
        sdl.SetCursor(object())


# --- cursor lifetime ---------------------------------------------------------

def _color_cursor(sdl):
    surface = sdl.CreateRGBSurface((8, 8))
    try:
        return sdl.CreateColorCursor(surface, 0, 0)
    except sdl.error:
        pytest.skip('cursors unsupported under this driver')


def test_get_cursor_returns_the_owning_object(sdl):
    cursor = _color_cursor(sdl)
    cursor.Set()
    assert sdl.GetCursor() is cursor
    sdl.SetCursor(None)


def test_freeing_the_current_cursor(sdl):
    # GetCursor() hands back the owning object for our cursors, so no borrowed
    # wrapper of one can exist; freeing it while current makes SDL fall back
    # to its default cursor, which GetCursor() then reports (None under dummy).
    import gc
    cursor = _color_cursor(sdl)
    cursor.Set()
    assert sdl.GetCursor() is cursor
    del cursor
    gc.collect()
    current = sdl.GetCursor()
    assert current is None or type(current).__name__ == 'Cursor'
    if current is not None:
        current.Set()  # SDL's default: still valid


def test_freed_cursor_rejected_by_set_cursor(sdl):
    import gc
    cursor = _color_cursor(sdl)
    zombie = type(cursor)()  # an uninitialised Cursor stands in for a freed one
    with pytest.raises(sdl.error, match='not valid'):
        sdl.SetCursor(zombie)
    with pytest.raises(sdl.error, match='not valid'):
        zombie.Set()
    del cursor
    gc.collect()


def test_quit_invalidates_cursors(sdl):
    # SDL_Quit frees every cursor, including ones we own: they must neither
    # dangle nor be freed a second time. Run in a subprocess (it tears SDL down).
    import os
    import subprocess
    import sys
    code = (
        'import gc, SDL2\n'
        'SDL2.Init(SDL2.INIT_VIDEO)\n'
        'try:\n'
        '    cursor = SDL2.CreateColorCursor(SDL2.CreateRGBSurface((8, 8)), 0, 0)\n'
        'except SDL2.error:\n'
        '    print("skip"); raise SystemExit\n'
        'cursor.Set()\n'
        'current = SDL2.GetCursor()\n'
        'SDL2.Quit()\n'
        'for c in (cursor, current):\n'
        '    try:\n'
        '        c.Set(); raise SystemExit("Set() on a freed cursor did not raise")\n'
        '    except SDL2.error:\n'
        '        pass\n'
        'SDL2.Init(SDL2.INIT_VIDEO)\n'
        'fresh = SDL2.CreateColorCursor(SDL2.CreateRGBSurface((8, 8)), 0, 0)\n'
        'del cursor, current\n'
        'gc.collect()\n'
        'fresh.Set()\n'
        'SDL2.Quit()\n'
        'print("ok")\n'
    )
    env = dict(os.environ, PYTHONPATH=os.path.dirname(sdl.__file__))
    out = subprocess.run([sys.executable, '-c', code], env=env, capture_output=True, text=True)
    if out.stdout.strip() == 'skip':
        pytest.skip('cursors unsupported under this driver')
    assert out.returncode == 0 and out.stdout.strip() == 'ok', out.stderr or out.stdout
