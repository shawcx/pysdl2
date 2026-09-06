'''Phase 2: PixelFormat and Palette.'''

import pytest


def test_pixelformat_attributes(sdl):
    pf = sdl.PixelFormat(sdl.PIXELFORMAT_RGBA8888)
    assert pf.format == sdl.PIXELFORMAT_RGBA8888
    assert pf.bpp == 32
    assert pf.bytes == 4
    assert pf.Rmask == 0xFF000000
    assert pf.Amask == 0x000000FF


def test_pixelformat_map_and_unmap(sdl):
    pf = sdl.PixelFormat(sdl.PIXELFORMAT_RGBA8888)
    pixel = pf.MapRGBA((10, 20, 30, 200))
    assert isinstance(pixel, int)
    assert pf.GetRGBA(pixel) == (10, 20, 30, 200)
    assert pf.GetRGB(pixel) == (10, 20, 30)


def test_pixelformat_uninitialised_is_guarded(sdl):
    with pytest.raises(sdl.error):
        sdl.PixelFormat().MapRGB((0, 0, 0))
    with pytest.raises(sdl.error):
        _ = sdl.PixelFormat().format


def test_palette_roundtrip(sdl):
    pal = sdl.Palette(4)
    assert pal.ncolors == 4
    pal.SetColors([(0, 0, 0), (255, 0, 0), (0, 255, 0, 128), (0, 0, 255)])
    assert pal.GetColors() == [
        (0, 0, 0, 255), (255, 0, 0, 255), (0, 255, 0, 128), (0, 0, 255, 255),
    ]


def test_palette_set_colors_offset(sdl):
    pal = sdl.Palette(8)
    pal.SetColors([(1, 1, 1), (2, 2, 2)], first=6)
    assert pal.GetColors()[6:] == [(1, 1, 1, 255), (2, 2, 2, 255)]


def test_palette_applied_to_indexed_surface(sdl):
    surface = sdl.CreateRGBSurfaceWithFormat((8, 8), sdl.PIXELFORMAT_INDEX8)
    pal = sdl.Palette(256)
    pal.SetColors([(i, i, i) for i in range(256)])
    surface.SetPalette(pal)


def test_module_pixel_format_helpers(sdl):
    assert 'RGBA8888' in sdl.GetPixelFormatName(sdl.PIXELFORMAT_RGBA8888)

    bpp, r, g, b, a = sdl.PixelFormatEnumToMasks(sdl.PIXELFORMAT_RGBA8888)
    assert bpp == 32
    assert sdl.MasksToPixelFormatEnum(bpp, r, g, b, a) == sdl.PIXELFORMAT_RGBA8888
