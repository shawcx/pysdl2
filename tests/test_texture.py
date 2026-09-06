'''Phase 1: Texture construction, pixel upload, modulation, locking.'''

import pytest


@pytest.fixture
def static_texture(sdl, renderer):
    return sdl.Texture(renderer, sdl.PIXELFORMAT_RGBA8888, sdl.TEXTUREACCESS_STATIC, (4, 4))


def test_constructor_and_query(sdl, renderer):
    tex = sdl.Texture(renderer, sdl.PIXELFORMAT_RGBA8888, sdl.TEXTUREACCESS_STATIC, (8, 6))
    fmt, access, w, h = tex.Query()
    assert (w, h) == (8, 6)
    assert access == sdl.TEXTUREACCESS_STATIC


def test_constructor_rejects_non_renderer(sdl):
    with pytest.raises(TypeError):
        sdl.Texture(object())


def test_update_full_and_partial(static_texture):
    static_texture.Update(b'\xff' * (4 * 4 * 4))
    static_texture.Update(b'\x80' * (2 * 2 * 4), (1, 1, 2, 2))
    static_texture.Update(b'\x80' * (2 * 2 * 4), rect=(1, 1, 2, 2), pitch=8)


def test_color_alpha_blend_mod_roundtrip(sdl, static_texture):
    static_texture.SetColorMod(10, 20, 30)
    assert static_texture.GetColorMod() == (10, 20, 30)

    static_texture.SetAlphaMod(128)
    assert static_texture.GetAlphaMod() == 128

    static_texture.SetBlendMode(sdl.BLENDMODE_ADD)
    assert static_texture.GetBlendMode() == sdl.BLENDMODE_ADD


def test_scale_mode_roundtrip(sdl, static_texture):
    if not hasattr(static_texture, 'SetScaleMode'):
        pytest.skip('SDL_*TextureScaleMode needs SDL >= 2.0.12')
    static_texture.SetScaleMode(sdl.SCALEMODE_LINEAR)
    assert static_texture.GetScaleMode() == sdl.SCALEMODE_LINEAR


def test_lock_returns_writable_view(sdl, renderer):
    tex = sdl.Texture(renderer, sdl.PIXELFORMAT_RGBA8888, sdl.TEXTUREACCESS_STREAMING, (4, 4))
    view, pitch = tex.Lock()
    try:
        assert pitch >= 4 * 4
        assert len(view) == pitch * 4
        assert view.readonly is False
        view[0] = 123
    finally:
        tex.Unlock()


def test_lock_region(sdl, renderer):
    tex = sdl.Texture(renderer, sdl.PIXELFORMAT_RGBA8888, sdl.TEXTUREACCESS_STREAMING, (8, 8))
    view, pitch = tex.Lock((0, 0, 2, 2))
    try:
        assert len(view) == pitch * 2
    finally:
        tex.Unlock()
