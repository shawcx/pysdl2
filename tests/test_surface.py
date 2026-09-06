'''Phase 2: Surface creation, blitting, fills, conversion, state.'''

import pytest


@pytest.fixture
def surface(sdl):
    return sdl.CreateRGBSurfaceWithFormat((16, 16), sdl.PIXELFORMAT_RGBA8888)


def _pixel(surface, x, y):
    data = surface.pixels
    off = y * surface.pitch + x * 4
    return tuple(data[off:off + 4])


def test_create_with_format(sdl, surface):
    assert (surface.w, surface.h) == (16, 16)
    assert surface.format == sdl.PIXELFORMAT_RGBA8888
    assert surface.pitch == 16 * 4


def test_create_from_buffer_keeps_data_alive(sdl):
    buf = bytearray(b'\xAB' * (4 * 4 * 4))
    surface = sdl.CreateRGBSurfaceFrom(buf, (4, 4))
    del buf
    assert surface.pixels[:4] == b'\xAB\xAB\xAB\xAB'


def test_fill_rect_int_and_tuple_colours(sdl, surface):
    surface.FillRect(None, (0, 0, 0, 255))
    surface.FillRect((0, 0, 8, 8), (255, 0, 0, 255))
    surface.FillRect((8, 8, 8, 8), surface.MapRGB((0, 255, 0)))
    # RGBA8888 little-endian in memory -> A, B, G, R
    assert _pixel(surface, 0, 0) == (255, 0, 0, 255)
    assert _pixel(surface, 15, 15) == (255, 0, 255, 0)
    assert _pixel(surface, 0, 15) == (255, 0, 0, 0)


def test_fill_rects(sdl, surface):
    surface.FillRect(None, 0)
    surface.FillRects([(0, 0, 2, 2), (4, 4, 2, 2)], (1, 2, 3, 4))


def test_blit_and_blit_scaled(sdl, surface):
    src = sdl.CreateRGBSurfaceWithFormat((4, 4), sdl.PIXELFORMAT_RGBA8888)
    src.FillRect(None, (10, 20, 30, 255))
    surface.FillRect(None, 0)
    surface.Blit(src, None, (2, 2))
    assert _pixel(surface, 3, 3) == (255, 30, 20, 10)
    surface.BlitScaled(src, None, (0, 0, 16, 16))
    assert _pixel(surface, 0, 0) == (255, 30, 20, 10)


def test_blit_rejects_non_surface(surface):
    with pytest.raises(TypeError):
        surface.Blit(object())


def test_convert(sdl, surface):
    surface.FillRect(None, (1, 2, 3, 255))

    by_enum = surface.Convert(sdl.PIXELFORMAT_RGB24)
    assert by_enum.format == sdl.PIXELFORMAT_RGB24

    by_format = surface.Convert(sdl.PixelFormat(sdl.PIXELFORMAT_BGRA8888))
    assert by_format.format == sdl.PIXELFORMAT_BGRA8888

    with pytest.raises(TypeError):
        surface.Convert('nope')


def test_duplicate(surface):
    surface.FillRect(None, (9, 9, 9, 255))
    clone = surface.Duplicate()
    assert (clone.w, clone.h) == (surface.w, surface.h)
    assert clone.pixels == surface.pixels


def test_colour_key_roundtrip(sdl, surface):
    assert surface.GetColorKey() is None
    surface.SetColorKey((255, 0, 255))
    assert surface.GetColorKey() == surface.MapRGBA((255, 0, 255, 255))
    surface.SetColorKey((255, 0, 255), enable=False)
    assert surface.GetColorKey() is None


def test_blend_colour_alpha_mod(sdl, surface):
    surface.SetBlendMode(sdl.BLENDMODE_BLEND)
    assert surface.GetBlendMode() == sdl.BLENDMODE_BLEND
    surface.SetColorMod(200, 100, 50)
    assert surface.GetColorMod() == (200, 100, 50)
    surface.SetAlphaMod(123)
    assert surface.GetAlphaMod() == 123


def test_clip_rect(surface):
    assert surface.SetClipRect((1, 1, 8, 8)) is True
    assert surface.GetClipRect() == (1, 1, 8, 8)
    surface.SetClipRect(None)
    assert surface.GetClipRect() == (0, 0, 16, 16)


def test_rle_and_get_pixel_format(sdl, surface):
    surface.SetRLE(True)
    surface.SetRLE(False)
    pf = surface.GetPixelFormat()
    assert pf.format == sdl.PIXELFORMAT_RGBA8888


def test_load_bmp_from_bytes(sdl, tmp_path):
    src = sdl.CreateRGBSurface((8, 6))
    src.FillRect(None, (1, 2, 3, 255))
    path = tmp_path / 's.bmp'
    src.SaveBMP(str(path))

    loaded = sdl.LoadBMP(path.read_bytes())
    assert (loaded.w, loaded.h) == (8, 6)


def test_save_png_jpg(sdl, surface, tmp_path):
    surface.FillRect(None, (40, 50, 60, 255))
    png = tmp_path / 'o.png'
    jpg = tmp_path / 'o.jpg'
    surface.SavePNG(str(png))
    surface.SaveJPG(str(jpg), quality=75)
    assert png.read_bytes()[:4] == b'\x89PNG'
    assert jpg.read_bytes()[:2] == b'\xff\xd8'
