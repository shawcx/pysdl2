#!/usr/bin/env python3

# Logical-resolution rendering: a fixed 160x120 canvas scaled to a resizable
# window. Shows mouse picking with RenderWindowToLogical, a RenderGeometryRaw
# colour wheel fed straight from an array, a streaming texture drawn through
# LockToSurface, and the window's pixel size (differs on high-DPI screens).
#
#   V toggles vsync, Esc or close quits. --frames N exits after N frames.

import array
import math
import sys
import SDL2

LOGICAL_W, LOGICAL_H = 160, 120
frames_left = int(sys.argv[sys.argv.index('--frames') + 1]) if '--frames' in sys.argv else None

SDL2.Init(SDL2.INIT_VIDEO)
window, renderer = SDL2.CreateWindowAndRenderer((640, 480), SDL2.WINDOW_RESIZABLE)
window.SetWindowTitle('logical')
renderer.RenderSetLogicalSize(LOGICAL_W, LOGICAL_H)

if hasattr(window, 'GetWindowSizeInPixels'):
    print('window size', window.GetWindowSize(), 'pixels', window.GetWindowSizeInPixels())

# Colour wheel: centre vertex plus a ring, drawn as an indexed triangle fan.
SEGMENTS = 24
xy = array.array('f', [40.0, 60.0])
rgba = array.array('B', [255, 255, 255, 255])
for i in range(SEGMENTS):
    angle = 2 * math.pi * i / SEGMENTS
    xy.extend([40 + 30 * math.cos(angle), 60 + 30 * math.sin(angle)])
    rgba.extend([int(127 + 127 * math.cos(angle + k * 2.094)) for k in range(3)] + [255])
indices = array.array('H')
for i in range(SEGMENTS):
    indices.extend([0, 1 + i, 1 + (i + 1) % SEGMENTS])

# A streaming texture we redraw every frame through a Surface.
stripes = SDL2.Texture(renderer, SDL2.PIXELFORMAT_RGBA32, SDL2.TEXTUREACCESS_STREAMING, (32, 32))

vsync = True
cursor = (0.0, 0.0)
frame = 0
running = True
while running:
    while (event := SDL2.PollEvent()) is not None:
        kind, data = event
        if kind == SDL2.QUIT:
            running = False
        elif kind == SDL2.KEYDOWN and data[2] == SDL2.K_ESCAPE:
            running = False
        elif kind == SDL2.KEYDOWN and data[2] == SDL2.K_v and hasattr(renderer, 'RenderSetVSync'):
            vsync = not vsync
            renderer.RenderSetVSync(vsync)
            print('vsync', vsync)
        elif kind == SDL2.MOUSEMOTION and hasattr(renderer, 'RenderWindowToLogical'):
            # Mouse events arrive in window coordinates; map them onto the canvas.
            cursor = renderer.RenderWindowToLogical((data[1], data[2]))

    surface = stripes.LockToSurface()
    for y in range(0, 32, 4):
        shade = (frame * 4 + y * 8) % 256
        surface.FillRect((0, y, 32, 4), (shade, 64, 255 - shade, 255))
    stripes.Unlock()

    renderer.SetRenderDrawColor(16, 16, 24, 255)
    renderer.Clear()
    renderer.RenderGeometryRaw(None, xy, rgba, indices=indices, index_size=2)
    renderer.Copy(stripes, None, (100, 44, 32, 32))

    renderer.SetRenderDrawColor(255, 255, 255, 255)
    cx, cy = cursor
    renderer.DrawLine(cx - 4, cy, cx + 4, cy)
    renderer.DrawLine(cx, cy - 4, cx, cy + 4)
    renderer.Present()

    frame += 1
    if frames_left is not None:
        frames_left -= 1
        running = running and frames_left > 0

SDL2.Quit()
