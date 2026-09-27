'''Phase 11: SDL_image - typed loading, format probes, save-to-bytes, SVG, XPM,
animations.'''

import pathlib
import struct

import pytest


def _rgba(surface, x, y):
    '''(r, g, b, a) at (x, y), whatever the surface's own format.'''
    import SDL2
    rgba = surface.Convert(SDL2.PIXELFORMAT_RGBA32)
    off = y * rgba.pitch + x * 4
    return tuple(rgba.pixels[off:off + 4])


def _gif(frames, delay_cs=10):
    '''A minimal 1x1 animated GIF; frames is a list of palette indexes (0 = red,
    1 = blue). The LZW stream is clear, <index>, end at code size 2.'''
    lzw = {0: b'\x02\x02\x44\x01\x00', 1: b'\x02\x02\x4c\x01\x00'}
    out = bytearray(b'GIF89a' + struct.pack('<HHBBB', 1, 1, 0x80, 0, 0))
    out += b'\xff\x00\x00' + b'\x00\x00\xff'  # 2-entry global palette
    out += b'\x21\xff\x0bNETSCAPE2.0\x03\x01\x00\x00\x00'  # loop forever
    for index in frames:
        out += b'\x21\xf9\x04\x04' + struct.pack('<H', delay_cs) + b'\x00\x00'
        out += b'\x2c' + struct.pack('<HHHHB', 0, 0, 1, 1, 0) + lzw[index]
    out += b'\x3b'
    return bytes(out)


SVG = (b'<svg xmlns="http://www.w3.org/2000/svg" width="10" height="20">'
       b'<rect width="10" height="20" fill="#00ff00"/></svg>')

XPM = [
    '2 2 2 1',
    'r c #FF0000',
    'b c #0000FF',
    'rb',
    'br',
]


@pytest.fixture
def red_green(sdl):
    '''4x2 surface: left half red, right half green.'''
    surface = sdl.CreateRGBSurfaceWithFormat((4, 2), sdl.PIXELFORMAT_RGBA32)
    surface.FillRect((0, 0, 2, 2), (255, 0, 0, 255))
    surface.FillRect((2, 0, 2, 2), (0, 255, 0, 255))
    return surface


def test_linked_version(sdl):
    major, minor, patch = sdl.IMG_Linked_Version()
    assert major == 2 and minor >= 0 and patch >= 0


def test_save_png_to_bytes_round_trips(sdl, red_green):
    data = red_green.SavePNG()
    assert data[:4] == b'\x89PNG'
    loaded = sdl.LoadImage(data)
    assert (loaded.w, loaded.h) == (4, 2)
    assert _rgba(loaded, 0, 0) == (255, 0, 0, 255)
    assert _rgba(loaded, 3, 1) == (0, 255, 0, 255)


def test_save_jpg_to_bytes(sdl, red_green):
    small = red_green.SaveJPG(quality=10)
    big = red_green.SaveJPG(quality=100)
    assert small[:2] == big[:2] == b'\xff\xd8'
    assert sdl.IMG_isJPG(big) is True
    loaded = sdl.LoadImage(big)
    assert (loaded.w, loaded.h) == (4, 2)


def test_save_to_path_still_works(sdl, red_green, tmp_path):
    path = tmp_path / 'x.png'
    assert red_green.SavePNG(str(path)) is None
    assert path.read_bytes() == red_green.SavePNG()
    assert red_green.SaveJPG(str(tmp_path / 'x.jpg'), quality=50) is None
    with pytest.raises(sdl.error):
        red_green.SavePNG(str(tmp_path / 'missing-dir' / 'x.png'))


def test_load_image_sources(sdl, red_green, tmp_path):
    path = tmp_path / 'img.png'
    red_green.SavePNG(str(path))
    for src in (str(path), path, path.read_bytes(), bytearray(path.read_bytes()),
                memoryview(path.read_bytes())):
        assert sdl.LoadImage(src).w == 4
    with pytest.raises(sdl.error):
        sdl.LoadImage(str(tmp_path / 'nope.png'))
    with pytest.raises(TypeError):
        sdl.LoadImage(42)


def _tga():
    '''A 2x1 uncompressed 24-bit TGA (blue, red); TGA has no magic number.'''
    header = struct.pack('<BBBHHBHHHHBB', 0, 0, 2, 0, 0, 0, 0, 0, 2, 1, 24, 0x20)
    return header + b'\xff\x00\x00' + b'\x00\x00\xff'


def test_load_image_typed(sdl, red_green):
    data = red_green.SavePNG()
    assert sdl.LoadImage(data, type='PNG').w == 4
    # Formats with a magic number are detected from the data whatever `type` says.
    assert sdl.LoadImage(data, 'GIF').h == 2

    tga = _tga()
    with pytest.raises(sdl.error):
        sdl.LoadImage(tga)  # magic-less: undetectable without a type
    surface = sdl.LoadImage(tga, type='TGA')
    assert (surface.w, surface.h) == (2, 1)
    assert _rgba(surface, 0, 0)[:3] == (0, 0, 255)
    assert _rgba(surface, 1, 0)[:3] == (255, 0, 0)


def test_format_probes(sdl, red_green, tmp_path):
    png = red_green.SavePNG()
    bmp_path = tmp_path / 'p.bmp'
    red_green.SaveBMP(str(bmp_path))
    bmp = bmp_path.read_bytes()

    assert sdl.IMG_isPNG(png) is True
    assert sdl.IMG_isBMP(png) is False
    assert sdl.IMG_isBMP(bmp) is True
    assert sdl.IMG_isBMP(str(bmp_path)) is True  # probes accept paths too
    assert sdl.IMG_isGIF(_gif([0])) is True
    assert sdl.IMG_isSVG(SVG) is True
    assert sdl.IMG_isXPM(b'/* XPM */\nstatic char *x[] = {};') is True
    for name in ('BMP', 'CUR', 'GIF', 'ICO', 'JPG', 'LBM', 'PCX', 'PNG', 'PNM',
                 'TIF', 'WEBP', 'XCF', 'XPM', 'XV', 'SVG', 'AVIF', 'JXL', 'QOI'):
        probe = getattr(sdl, 'IMG_is' + name, None)
        if probe is not None:
            assert probe(b'not an image at all') is False, name


def test_probe_does_not_consume_bytes(sdl, red_green):
    data = red_green.SavePNG()
    assert sdl.IMG_isPNG(data) and sdl.IMG_isPNG(data)
    assert sdl.LoadImage(data).w == 4


def test_load_sized_svg(sdl):
    if not hasattr(sdl, 'IMG_LoadSizedSVG'):
        pytest.skip('needs SDL_image >= 2.6')
    natural = sdl.LoadImage(SVG)
    assert (natural.w, natural.h) == (10, 20)
    sized = sdl.IMG_LoadSizedSVG(SVG, 30, 60)
    assert (sized.w, sized.h) == (30, 60)
    assert _rgba(sized, 15, 30)[:3] == (0, 255, 0)


def test_read_xpm_from_array(sdl):
    surface = sdl.IMG_ReadXPMFromArray(XPM)
    assert (surface.w, surface.h) == (2, 2)
    assert _rgba(surface, 0, 0)[:3] == (255, 0, 0)
    assert _rgba(surface, 1, 0)[:3] == (0, 0, 255)
    if hasattr(sdl, 'IMG_ReadXPMFromArrayToRGB888'):
        rgb = sdl.IMG_ReadXPMFromArrayToRGB888(XPM)
        assert rgb.bpp == 32  # never paletted (exact 32-bit layout varies by version)
        assert _rgba(rgb, 0, 1)[:3] == (0, 0, 255)
    with pytest.raises(sdl.error):
        sdl.IMG_ReadXPMFromArray(['garbage'])
    with pytest.raises(TypeError):
        sdl.IMG_ReadXPMFromArray([1, 2])


def test_load_animation(sdl, tmp_path):
    if not hasattr(sdl, 'IMG_LoadAnimation'):
        pytest.skip('needs SDL_image >= 2.6')
    gif = _gif([0, 1, 0], delay_cs=7)
    w, h, frames = sdl.IMG_LoadAnimation(gif)
    assert (w, h) == (1, 1)
    assert [delay for _, delay in frames] == [70, 70, 70]
    colours = [_rgba(frame, 0, 0)[:3] for frame, _ in frames]
    assert colours == [(255, 0, 0), (0, 0, 255), (255, 0, 0)]

    path = tmp_path / 'anim.gif'
    path.write_bytes(gif)
    assert len(sdl.IMG_LoadAnimation(str(path))[2]) == 3
    assert len(sdl.IMG_LoadAnimation(path.read_bytes(), type='GIF')[2]) == 3


def test_animation_frames_outlive_each_other(sdl):
    if not hasattr(sdl, 'IMG_LoadAnimation'):
        pytest.skip('needs SDL_image >= 2.6')
    _, _, frames = sdl.IMG_LoadAnimation(_gif([0, 1]))
    second = frames[1][0]
    del frames  # frees the first frame; the second is independently owned
    assert _rgba(second, 0, 0)[:3] == (0, 0, 255)


def test_load_animation_rejects_garbage(sdl):
    if not hasattr(sdl, 'IMG_LoadAnimation'):
        pytest.skip('needs SDL_image >= 2.6')
    with pytest.raises(sdl.error):
        sdl.IMG_LoadAnimation(b'nope')


def test_renderer_load_texture_sources(sdl, renderer, red_green, tmp_path):
    data = red_green.SavePNG()
    path = tmp_path / 't.png'
    path.write_bytes(data)
    for texture in (renderer.LoadTexture(str(path)), renderer.LoadTexture(data),
                    renderer.LoadTexture(data, type='PNG'), renderer.LoadTexture(path)):
        assert texture.Query()[2:] == (4, 2)
    assert renderer.LoadTexture(_tga(), type='TGA').Query()[2:] == (2, 1)
    with pytest.raises(sdl.error):
        renderer.LoadTexture(b'garbage')


def test_load_bmp_accepts_pathlike(sdl, red_green, tmp_path):
    path = tmp_path / 'p.bmp'
    red_green.SaveBMP(str(path))
    assert sdl.LoadBMP(path).w == 4
    assert sdl.LoadBMP(pathlib.Path(str(path))).h == 2
