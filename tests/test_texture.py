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


# --- phase 12: LockToSurface, UpdateNV ------------------------------------

def test_lock_to_surface(sdl, renderer):
    if not hasattr(sdl.Texture, 'LockToSurface'):
        pytest.skip('needs SDL >= 2.0.12')
    tex = sdl.Texture(renderer, sdl.PIXELFORMAT_RGBA32, sdl.TEXTUREACCESS_STREAMING, (8, 4))
    surface = tex.LockToSurface()
    assert (surface.w, surface.h) == (8, 4)
    surface.FillRect(None, (10, 20, 30, 255))
    with pytest.raises(sdl.error):
        tex.LockToSurface()  # one locked surface at a time
    tex.Unlock()

    # SDL freed the locked surface; ours is now an empty stand-in, not dangling.
    assert (surface.w, surface.h) == (0, 0)
    surface.FillRect(None, (0, 0, 0, 255))

    part = tex.LockToSurface((2, 1, 4, 2))
    assert (part.w, part.h) == (4, 2)
    del tex  # dealloc while locked also detaches the surface
    assert (part.w, part.h) == (0, 0)


def test_lock_to_surface_needs_streaming(sdl, renderer):
    if not hasattr(sdl.Texture, 'LockToSurface'):
        pytest.skip('needs SDL >= 2.0.12')
    tex = sdl.Texture(renderer, sdl.PIXELFORMAT_RGBA32, sdl.TEXTUREACCESS_STATIC, (4, 4))
    with pytest.raises(sdl.error):
        tex.LockToSurface()


def test_update_nv(sdl, renderer):
    if not hasattr(sdl.Texture, 'UpdateNV'):
        pytest.skip('needs SDL >= 2.0.16')
    tex = sdl.Texture(renderer, sdl.PIXELFORMAT_NV12, sdl.TEXTUREACCESS_STREAMING, (4, 4))
    tex.UpdateNV(b'\x80' * 16, 4, b'\x80' * 8, 4)
    tex.UpdateNV(b'\x80' * 8, 4, b'\x80' * 4, 4, rect=(0, 0, 4, 2))
    with pytest.raises(ValueError):
        tex.UpdateNV(b'\x80' * 15, 4, b'\x80' * 8, 4)
    with pytest.raises(ValueError):
        tex.UpdateNV(b'\x80' * 16, 4, b'\x80' * 7, 4)


def test_update_yuv_checks_plane_sizes(sdl, renderer):
    tex = sdl.Texture(renderer, sdl.PIXELFORMAT_IYUV, sdl.TEXTUREACCESS_STREAMING, (4, 4))
    y, u, v = b'\x80' * 16, b'\x80' * 4, b'\x80' * 4  # 4x4 luma, 2x2 chroma
    tex.UpdateYUV(y, 4, u, 2, v, 2)
    tex.UpdateYUV(y[:8], 4, u[:2], 2, v[:2], 2, rect=(0, 0, 4, 2))
    with pytest.raises(ValueError):
        tex.UpdateYUV(y[:15], 4, u, 2, v, 2)
    with pytest.raises(ValueError):
        tex.UpdateYUV(y, 4, u, 2, v[:3], 2)
    with pytest.raises(ValueError):
        tex.UpdateYUV(y, 0, u, 2, v, 2)


# --- renderer teardown invalidates its textures ----------------------------

def _software_renderer(sdl):
    return sdl.CreateSoftwareRenderer(sdl.CreateRGBSurface((8, 8)))


def test_textures_invalidated_with_renderer(sdl):
    import gc
    renderer = _software_renderer(sdl)
    made = [
        sdl.Texture(renderer, sdl.PIXELFORMAT_RGBA32, sdl.TEXTUREACCESS_STATIC, (4, 4)),
        renderer.CreateTextureFromSurface(sdl.CreateRGBSurface((4, 4))),
        renderer.LoadTexture(sdl.CreateRGBSurface((4, 4)).SavePNG()),
    ]
    del renderer
    gc.collect()
    for texture in made:
        with pytest.raises(sdl.error):
            texture.Query()
    del made
    gc.collect()  # freeing invalidated textures must not touch the freed ones


def test_invalidated_texture_rejected_by_other_renderer(sdl):
    import gc
    renderer = _software_renderer(sdl)
    texture = sdl.Texture(renderer, sdl.PIXELFORMAT_RGBA32, sdl.TEXTUREACCESS_TARGET, (4, 4))
    other = _software_renderer(sdl)
    survivor = sdl.Texture(other, sdl.PIXELFORMAT_RGBA32, sdl.TEXTUREACCESS_STATIC, (2, 2))
    del renderer
    gc.collect()
    # A NULL texture means "none" to these SDL calls: they must raise instead.
    with pytest.raises(sdl.error, match='Renderer was destroyed'):
        other.SetRenderTarget(texture)
    with pytest.raises(sdl.error, match='Renderer was destroyed'):
        other.RenderGeometry(texture, [((0, 0), (255, 255, 255, 255), (0, 0))])
    with pytest.raises(sdl.error):
        other.Copy(texture)
    assert survivor.Query()[2:] == (2, 2)  # other renderers' textures untouched


def test_locked_surface_emptied_with_renderer(sdl):
    import gc
    if not hasattr(sdl.Texture, 'LockToSurface'):
        pytest.skip('needs SDL >= 2.0.12')
    renderer = _software_renderer(sdl)
    texture = sdl.Texture(renderer, sdl.PIXELFORMAT_RGBA32, sdl.TEXTUREACCESS_STREAMING, (4, 4))
    surface = texture.LockToSurface()
    del renderer
    gc.collect()
    assert (surface.w, surface.h) == (0, 0)


def test_borrowed_renderer_invalidated_with_owner(sdl):
    import gc
    window = sdl.Window('renderer-owner', (16, 16))
    renderer = window.CreateRenderer()
    borrowed = window.GetRenderer()
    del renderer
    gc.collect()
    with pytest.raises(sdl.error):
        borrowed.Clear()


def test_module_teardown_order_is_safe(sdl):
    # At interpreter exit module globals are cleared in no particular order;
    # a renderer freed before its textures must not leave them dangling.
    import os
    import subprocess
    import sys
    code = (
        'import SDL2\n'
        'SDL2.Init(SDL2.INIT_VIDEO)\n'
        'renderer = SDL2.CreateSoftwareRenderer(SDL2.CreateRGBSurface((8, 8)))\n'
        'textures = [SDL2.Texture(renderer, SDL2.PIXELFORMAT_RGBA32, SDL2.TEXTUREACCESS_STATIC, (2, 2))\n'
        '            for _ in range(100)]\n'
        'del renderer\n'
        'print("ok")\n'
    )
    env = dict(os.environ, PYTHONPATH=os.path.dirname(sdl.__file__))
    out = subprocess.run([sys.executable, '-c', code], env=env, capture_output=True, text=True)
    assert out.returncode == 0 and out.stdout.strip() == 'ok', out.stderr
