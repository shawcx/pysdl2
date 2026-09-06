'''Renderer draw-colour getter and the tuple/list -> SDL_Rect converters,
exercised through Renderer.Copy (the only public path to them in phase 0).'''

import pytest


@pytest.fixture
def renderer(sdl):
    window = sdl.Window('render-test', (64, 48))
    try:
        yield window.CreateRenderer()
    except sdl.error:
        pytest.skip('no renderer available under this driver')


@pytest.fixture
def texture(sdl, renderer):
    return renderer.CreateTextureFromSurface(sdl.CreateRGBSurface((8, 8)))


def test_draw_color_roundtrip(renderer):
    renderer.SetRenderDrawColor(10, 20, 30, 40)
    assert renderer.GetRenderDrawColor() == (10, 20, 30, 40)


def test_copy_without_rects(renderer, texture):
    renderer.Clear()
    renderer.Copy(texture)
    renderer.Copy(texture, None, None)
    renderer.Present()


def test_copy_accepts_tuple_and_list_rects(renderer, texture):
    renderer.Copy(texture, (0, 0, 4, 4), [0, 0, 8, 8])


def test_copy_rejects_malformed_rect(renderer, texture):
    with pytest.raises(ValueError):
        renderer.Copy(texture, (1, 2, 3))
    with pytest.raises(TypeError):
        renderer.Copy(texture, 5)
