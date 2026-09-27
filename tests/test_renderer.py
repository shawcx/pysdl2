'''Phase 1: Renderer construction, primitives, draw state, targets, info.'''

import pytest


def test_standalone_constructor(sdl, window):
    renderer = sdl.Renderer(window)
    assert type(renderer).__name__ == 'Renderer'
    assert sdl.Renderer in type(renderer).__mro__


def test_constructor_rejects_non_window(sdl):
    with pytest.raises(TypeError):
        sdl.Renderer(object())


def test_software_renderer_from_surface(sdl):
    surface = sdl.CreateRGBSurface((32, 32))
    renderer = sdl.CreateSoftwareRenderer(surface)
    renderer.SetRenderDrawColor(255, 255, 255, 255)
    renderer.Clear()
    with pytest.raises(TypeError):
        sdl.CreateSoftwareRenderer(object())


def test_draw_color_roundtrip(renderer):
    renderer.SetRenderDrawColor(10, 20, 30, 40)
    assert renderer.GetRenderDrawColor() == (10, 20, 30, 40)


def test_primitives_accept_ints_and_floats(renderer):
    renderer.Clear()
    renderer.DrawPoint(1, 2)
    renderer.DrawPoint(1.5, 2.5)
    renderer.DrawPoints([(0, 0), (1, 1), (2, 2)])
    renderer.DrawLine(0, 0, 10, 10)
    renderer.DrawLines([(0, 0), (5, 5), (10, 0)])
    renderer.DrawRect((1, 1, 5, 5))
    renderer.DrawRect(None)
    renderer.DrawRects([(0, 0, 4, 4), (5, 5, 4, 4)])
    renderer.FillRect((2, 2, 8, 8))
    renderer.FillRect(None)
    renderer.FillRects([(0, 0, 2, 2), (3, 3, 2, 2)])
    renderer.Present()


def test_draw_blend_mode_roundtrip(renderer, sdl):
    renderer.SetRenderDrawBlendMode(sdl.BLENDMODE_BLEND)
    assert renderer.GetRenderDrawBlendMode() == sdl.BLENDMODE_BLEND


def test_viewport_clip_scale_logical(renderer):
    renderer.RenderSetViewport((0, 0, 32, 24))
    assert renderer.RenderGetViewport() == (0, 0, 32, 24)
    renderer.RenderSetViewport(None)

    assert renderer.RenderIsClipEnabled() is False
    renderer.RenderSetClipRect((1, 1, 10, 10))
    assert renderer.RenderGetClipRect() == (1, 1, 10, 10)
    assert renderer.RenderIsClipEnabled() is True
    renderer.RenderSetClipRect(None)
    assert renderer.RenderIsClipEnabled() is False

    renderer.RenderSetScale(2.0, 3.0)
    assert renderer.RenderGetScale() == (2.0, 3.0)
    renderer.RenderSetScale(1.0, 1.0)

    renderer.RenderSetLogicalSize(100, 80)
    assert renderer.RenderGetLogicalSize() == (100, 80)
    renderer.RenderSetLogicalSize(0, 0)

    renderer.RenderSetIntegerScale(True)
    assert renderer.RenderGetIntegerScale() is True
    renderer.RenderSetIntegerScale(False)


def test_render_target_identity(renderer, sdl):
    assert renderer.GetRenderTarget() is None
    if not renderer.RenderTargetSupported():
        pytest.skip('render targets unsupported')

    target = sdl.Texture(renderer, sdl.PIXELFORMAT_RGBA8888, sdl.TEXTUREACCESS_TARGET, (16, 16))
    renderer.SetRenderTarget(target)
    assert renderer.GetRenderTarget() is target
    renderer.SetRenderTarget(None)
    assert renderer.GetRenderTarget() is None


def test_read_pixels_length_and_value(renderer):
    renderer.SetRenderDrawColor(50, 60, 70, 255)
    renderer.Clear()
    data = renderer.RenderReadPixels((0, 0, 2, 2))
    assert len(data) == 2 * 2 * 4
    # ARGB8888, little-endian in memory -> B, G, R, A
    assert data[0:4] == bytes((70, 60, 50, 255))


def test_info_and_output_size(renderer):
    name, flags, formats, max_w, max_h = renderer.GetRendererInfo()
    assert isinstance(name, str) and name
    assert isinstance(formats, list)
    assert renderer.GetRendererOutputSize() == (64, 48)


def test_render_geometry(renderer, sdl):
    if not hasattr(renderer, 'RenderGeometry'):
        pytest.skip('SDL_RenderGeometry needs SDL >= 2.0.18')
    verts = [
        ((0, 0), (255, 0, 0, 255)),
        ((16, 0), (0, 255, 0, 255)),
        ((0, 16), (0, 0, 255, 255)),
    ]
    renderer.RenderGeometry(None, verts)
    renderer.RenderGeometry(None, verts, [0, 1, 2])
    with pytest.raises(ValueError):
        renderer.RenderGeometry(None, [((0, 0),)])


def test_compose_custom_blend_mode(sdl):
    mode = sdl.ComposeCustomBlendMode(
        sdl.BLENDFACTOR_SRC_ALPHA, sdl.BLENDFACTOR_ONE, sdl.BLENDOPERATION_ADD,
        sdl.BLENDFACTOR_ONE, sdl.BLENDFACTOR_ONE, sdl.BLENDOPERATION_ADD)
    assert isinstance(mode, int)
    assert mode != sdl.BLENDMODE_INVALID


# --- phase 12: vsync, coordinate mapping, raw geometry, window links ------

import struct  # noqa: E402

TRIANGLE = struct.pack('6f', 0, 0, 10, 0, 0, 10)
RED = bytes([255, 0, 0, 255])


def _needs(obj, name):
    if not hasattr(obj, name):
        pytest.skip(f'{name} needs a newer SDL')


def test_create_window_and_renderer(sdl):
    window, renderer = sdl.CreateWindowAndRenderer((40, 30))
    assert type(window).__name__ == 'Window' and type(renderer).__name__ == 'Renderer'
    assert renderer.GetRendererOutputSize() == (40, 30)


def test_window_get_renderer_is_borrowed(sdl, window, renderer):
    assert sdl.Window('no renderer', (8, 8)).GetRenderer() is None
    borrowed = window.GetRenderer()
    assert borrowed.GetRendererOutputSize() == renderer.GetRendererOutputSize()
    del borrowed  # must not destroy the window's renderer
    renderer.Clear()
    renderer.Present()


def test_render_get_window(sdl, window, renderer):
    _needs(renderer, 'RenderGetWindow')
    assert renderer.RenderGetWindow().GetWindowID() == window.GetWindowID()
    software = sdl.CreateSoftwareRenderer(sdl.CreateRGBSurface((4, 4)))
    assert software.RenderGetWindow() is None


def test_vsync_and_metal(sdl, renderer):
    _needs(renderer, 'RenderSetVSync')
    try:
        renderer.RenderSetVSync(True)
        renderer.RenderSetVSync(False)
    except sdl.error:
        pass  # a renderer may not support toggling
    assert renderer.RenderGetMetalLayer() is None  # not a Metal renderer here
    assert renderer.RenderGetMetalCommandEncoder() is None


def test_window_logical_mapping(renderer):
    _needs(renderer, 'RenderWindowToLogical')
    w, h = renderer.GetRendererOutputSize()
    renderer.RenderSetLogicalSize(w // 2, h // 2)
    try:
        assert renderer.RenderWindowToLogical((w // 2, h // 2)) == (w // 4, h // 4)
        assert renderer.RenderLogicalToWindow((w / 4, h / 4)) == (w // 2, h // 2)
    finally:
        renderer.RenderSetLogicalSize(0, 0)


def test_render_geometry_raw(sdl, renderer):
    _needs(renderer, 'RenderGeometryRaw')
    renderer.RenderGeometryRaw(None, TRIANGLE, RED, color_stride=0)          # one colour
    renderer.RenderGeometryRaw(None, TRIANGLE, RED * 3)                      # per vertex
    renderer.RenderGeometryRaw(None, TRIANGLE, RED * 3, indices=bytes([0, 1, 2]), index_size=1)
    renderer.RenderGeometryRaw(None, TRIANGLE, RED * 3,
                               indices=struct.pack('3H', 2, 1, 0), index_size=2)
    renderer.RenderGeometryRaw(None, bytearray(TRIANGLE), memoryview(RED * 3), num_vertices=3)
    renderer.Present()


def test_render_geometry_raw_with_texture(sdl, renderer, texture):
    _needs(renderer, 'RenderGeometryRaw')
    uv = struct.pack('6f', 0, 0, 1, 0, 0, 1)
    renderer.RenderGeometryRaw(texture, TRIANGLE, RED * 3, uv)


def test_render_geometry_raw_validation(sdl, renderer):
    _needs(renderer, 'RenderGeometryRaw')
    with pytest.raises(ValueError):
        renderer.RenderGeometryRaw(None, TRIANGLE, RED)  # 3 vertices, 1 colour
    with pytest.raises(ValueError):
        renderer.RenderGeometryRaw(None, TRIANGLE, RED * 3, xy_stride=6)
    with pytest.raises(ValueError):
        renderer.RenderGeometryRaw(None, TRIANGLE, RED * 3, indices=b'\0', index_size=3)
    with pytest.raises(sdl.error):
        renderer.RenderGeometryRaw(None, TRIANGLE, RED * 3, indices=bytes([0, 1, 9]), index_size=1)
    with pytest.raises(TypeError):
        renderer.RenderGeometryRaw('nope', TRIANGLE, RED * 3)
