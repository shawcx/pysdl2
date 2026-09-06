'''Phase 0: the tuple/list -> SDL_Rect converters, via Renderer.Copy.'''

import pytest


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


def test_copy_rejects_non_texture(renderer):
    with pytest.raises(TypeError):
        renderer.Copy(object())
