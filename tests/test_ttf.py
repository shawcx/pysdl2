'''SDL2_ttf: SDL2.Font and the TTF_* functions. Skipped when the extension was
built without SDL2_ttf or no TrueType font can be found on this machine.'''

import gc
import glob
import shutil
import subprocess

import pytest

CANDIDATES = [
    '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',
    '/usr/share/fonts/TTF/DejaVuSans.ttf',
    '/Library/Fonts/Arial.ttf',
    '/System/Library/Fonts/Supplemental/Arial.ttf',
    'C:/Windows/Fonts/arial.ttf',
]


def _find_font():
    for path in CANDIDATES:
        if glob.glob(path):
            return path
    if shutil.which('fc-match'):
        out = subprocess.run(['fc-match', '-f', '%{file}', 'sans:fontformat=TrueType'],
                             capture_output=True, text=True).stdout.strip()
        if out.lower().endswith(('.ttf', '.otf')):
            return out
    return None


FONT_PATH = _find_font()
WHITE = (255, 255, 255)


@pytest.fixture
def ttf(sdl):
    if not hasattr(sdl, 'Font'):
        pytest.skip('built without SDL2_ttf')
    sdl.TTF_Init()
    yield sdl
    sdl.TTF_Quit()


@pytest.fixture
def font(ttf):
    if FONT_PATH is None:
        pytest.skip('no TrueType font found')
    f = ttf.Font(FONT_PATH, 16)
    yield f
    f.Close()


def test_init_and_versions(ttf):
    assert ttf.TTF_WasInit() >= 1
    assert ttf.TTF_Linked_Version()[0] == 2
    if hasattr(ttf, 'TTF_GetFreeTypeVersion'):
        assert ttf.TTF_GetFreeTypeVersion()[0] >= 2
        assert len(ttf.TTF_GetHarfBuzzVersion()) == 3


def test_open_requires_init(sdl):
    if not hasattr(sdl, 'Font'):
        pytest.skip('built without SDL2_ttf')
    assert sdl.TTF_WasInit() == 0
    with pytest.raises(sdl.error, match='TTF_Init'):
        sdl.Font(FONT_PATH or 'x.ttf', 12)


def test_open_errors(ttf, tmp_path):
    with pytest.raises(ttf.error):
        ttf.Font(str(tmp_path / 'missing.ttf'), 12)
    with pytest.raises(ttf.error):
        ttf.Font(b'definitely not a font', 12)
    with pytest.raises(ttf.error, match='not open'):
        ttf.Font().Height()


def test_metrics(font):
    assert font.Height() > 0 and font.Ascent() > 0 and font.Descent() <= 0
    assert font.LineSkip() >= font.Height() - 1
    assert font.Faces() >= 1
    assert isinstance(font.FaceIsFixedWidth(), bool)
    assert isinstance(font.FaceFamilyName(), str)
    name = font.FaceStyleName()
    assert name is None or isinstance(name, str)


def test_size_and_measure(font):
    w, h = font.Size('Hello')
    assert w > 0 and h == font.Height()
    assert font.Size('Hello Hello')[0] > w
    if hasattr(font, 'Measure'):
        extent, count = font.Measure('Hello world', w)
        assert count == 5 and extent <= w
    with pytest.raises(ValueError):
        font.Size('a\0b')
    with pytest.raises(TypeError):
        font.Size(b'bytes are not text')


def test_glyphs(font):
    assert font.GlyphIsProvided('A') is True
    assert font.GlyphIsProvided(ord('A')) is True
    assert font.GlyphIsProvided(0x10FFFD) is False  # private-use plane
    minx, maxx, miny, maxy, advance = font.GlyphMetrics('g')
    assert advance > 0 and maxx > minx and miny < 0  # 'g' has a descender
    assert isinstance(font.GetKerningSize('A', 'V'), int)
    with pytest.raises(ValueError):
        font.GlyphIsProvided('AB')


def test_render_modes(ttf, font):
    w, h = font.Size('Hello')
    blended = font.RenderBlended('Hello', WHITE)
    assert (blended.w, blended.h) == (w, h) and blended.bpp == 32
    solid = font.RenderSolid('Hello', WHITE)
    assert (solid.w, solid.h) == (w, h) and solid.bpp == 8
    shaded = font.RenderShaded('Hello', WHITE, (0, 0, 64))
    assert (shaded.w, shaded.h) == (w, h)
    if hasattr(font, 'RenderLCD'):
        assert font.RenderLCD('Hello', WHITE, (0, 0, 0)).w == w
    with pytest.raises(ttf.error):
        font.RenderBlended('', WHITE)  # SDL_ttf: "Text has zero width"


def test_rendered_pixels(ttf, font):
    surface = font.RenderShaded('I', (255, 0, 0), (0, 0, 255)).Convert(ttf.PIXELFORMAT_RGBA32)
    data = surface.pixels
    pixels = {tuple(data[i:i + 3]) for i in range(0, len(data), 4)}
    assert (0, 0, 255) in pixels     # background
    assert (255, 0, 0) in pixels     # fully covered glyph pixels


def test_render_wrapped(font):
    if not hasattr(font, 'Measure'):
        pytest.skip('wrapping needs SDL_ttf >= 2.0.18')
    text = 'the quick brown fox jumps over the lazy dog'
    one_line = font.RenderBlended(text, WHITE)
    wrapped = font.RenderBlended(text, WHITE, wrap_length=100)
    assert wrapped.w <= 100 < one_line.w
    assert wrapped.h > one_line.h
    assert font.RenderSolid('a\nb', WHITE, wrap_length=0).h > one_line.h  # newline only
    with pytest.raises(ValueError):
        font.RenderBlended(text, WHITE, wrap_length=-1)


def test_render_glyphs(font):
    assert font.RenderGlyphBlended('Q', WHITE).h == font.Height()
    assert font.RenderGlyphSolid(ord('Q'), WHITE).w > 0
    assert font.RenderGlyphShaded('Q', WHITE, (0, 0, 0)).w > 0
    if hasattr(font, 'RenderGlyphLCD'):
        assert font.RenderGlyphLCD('Q', WHITE, (0, 0, 0)).w > 0


def test_style_outline_hinting_kerning(ttf, font):
    plain = font.Size('Hello')
    font.SetStyle(ttf.TTF_STYLE_BOLD | ttf.TTF_STYLE_UNDERLINE)
    assert font.GetStyle() == ttf.TTF_STYLE_BOLD | ttf.TTF_STYLE_UNDERLINE
    assert font.Size('Hello')[0] >= plain[0]
    font.SetStyle(ttf.TTF_STYLE_NORMAL)

    font.SetOutline(2)
    assert font.GetOutline() == 2
    assert font.Size('Hello')[1] > plain[1]
    font.SetOutline(0)

    font.SetHinting(ttf.TTF_HINTING_MONO)
    assert font.GetHinting() == ttf.TTF_HINTING_MONO
    font.SetKerning(False)
    assert font.GetKerning() is False


def test_resize_sdf_align_direction(ttf, font):
    if not hasattr(font, 'SetSize'):
        pytest.skip('needs SDL_ttf >= 2.0.18')
    small = font.Height()
    font.SetSize(32)
    assert font.Height() > small
    font.SetSizeDPI(16, 144, 144)  # 16pt at 2x DPI ~ 32pt at 72
    assert font.Height() > small
    font.SetSDF(True)
    assert font.GetSDF() is True
    font.SetSDF(False)
    if hasattr(font, 'SetWrappedAlign'):
        font.SetWrappedAlign(ttf.TTF_WRAPPED_ALIGN_CENTER)
        assert font.GetWrappedAlign() == ttf.TTF_WRAPPED_ALIGN_CENTER
        try:
            font.SetDirection(ttf.TTF_DIRECTION_LTR)
            font.SetScriptName('Latn')
        except ttf.error:
            pass  # SDL_ttf built without HarfBuzz


def test_font_from_bytes_outlives_source(ttf):
    if FONT_PATH is None:
        pytest.skip('no TrueType font found')
    with open(FONT_PATH, 'rb') as f:
        data = f.read()
    font = ttf.Font(data, 20)
    del data
    gc.collect()  # SDL_ttf reads the font data lazily: the wrapper must pin it
    assert font.RenderBlended('from bytes', WHITE).w > 0
    font.Close()
    font.Close()  # idempotent


def test_font_after_ttf_quit(ttf):
    if FONT_PATH is None:
        pytest.skip('no TrueType font found')
    font = ttf.Font(FONT_PATH, 12)
    ttf.TTF_Quit()
    try:
        with pytest.raises(ttf.error, match='TTF_Quit'):
            font.Height()
        del font  # must not TTF_CloseFont into freed FreeType state
        gc.collect()
    finally:
        ttf.TTF_Init()  # balance the fixture's TTF_Quit
    again = ttf.Font(FONT_PATH, 12)
    assert again.Height() > 0
    again.Close()
