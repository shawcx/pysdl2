#!/usr/bin/env python3

# SDL_image tour, no window needed: probe a file's format, load it (animated
# GIF/WEBP frame by frame), and re-encode it to PNG / JPG in memory.
#
#   python3 example/image.py [image-file]
#
# With no file it builds a small test image and an SVG to work on.

import sys
import SDL2

SDL2.Init(SDL2.INIT_VIDEO)
print('SDL_image', '.'.join(map(str, SDL2.IMG_Linked_Version())))

FORMATS = [name[6:] for name in dir(SDL2) if name.startswith('IMG_is')]


def probe(data):
    return [fmt for fmt in FORMATS if getattr(SDL2, 'IMG_is' + fmt)(data)] or ['unknown']


def describe(label, data):
    print(f'\n{label}: {len(data)} bytes, looks like {", ".join(probe(data))}')

    if hasattr(SDL2, 'IMG_LoadAnimation') and probe(data)[0] in ('GIF', 'WEBP'):
        w, h, frames = SDL2.IMG_LoadAnimation(data)
        total = sum(delay for _, delay in frames)
        print(f'  animation {w}x{h}, {len(frames)} frames, {total} ms per loop')
        surface = frames[0][0]
    else:
        surface = SDL2.LoadImage(data)
        print(f'  image {surface.w}x{surface.h}, {SDL2.GetPixelFormatName(surface.format)}')

    png = surface.SavePNG()
    print(f'  re-encoded: PNG {len(png)} bytes', end='')
    for quality in (30, 90):
        print(f', JPG q{quality} {len(surface.SaveJPG(quality=quality))} bytes', end='')
    print()


if len(sys.argv) > 1:
    with open(sys.argv[1], 'rb') as f:
        describe(sys.argv[1], f.read())
else:
    # A gradient drawn into a surface, round-tripped through PNG bytes.
    surface = SDL2.CreateRGBSurfaceWithFormat((64, 32), SDL2.PIXELFORMAT_RGBA32)
    for x in range(64):
        surface.FillRect((x, 0, 1, 32), (x * 4, 128, 255 - x * 4, 255))
    describe('generated gradient', surface.SavePNG())

    svg = (b'<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16">'
           b'<circle cx="8" cy="8" r="7" fill="orange"/></svg>')
    describe('inline SVG', svg)
    if hasattr(SDL2, 'IMG_LoadSizedSVG'):
        big = SDL2.IMG_LoadSizedSVG(svg, 256, 256)
        print(f'  rasterised at {big.w}x{big.h} with IMG_LoadSizedSVG')

    icon = SDL2.IMG_ReadXPMFromArray([
        '4 4 2 1', '. c None', '# c #3060FF',
        '.##.', '####', '####', '.##.',
    ])
    print(f'\nXPM from array: {icon.w}x{icon.h}')

SDL2.Quit()
