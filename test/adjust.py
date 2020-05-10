#!/usr/bin/python3

# For LCD monitors using a VGA connection run this program then "auto-adjust" the monitor

import SDL2

# Initialize SDL2
SDL2.Init(SDL2.INIT_VIDEO)

numberOfDisplays = SDL2.GetNumVideoDisplays()

windows = []
for i in range(numberOfDisplays):
    x,y,w,h = SDL2.GetDisplayBounds(i)
    window = SDL2.Window("Adjust", size=(w,h), position=(x,y), flags=SDL2.WINDOW_FULLSCREEN)
    windows.append((window, w, h))

# Hide the cursor
SDL2.ShowCursor(SDL2.DISABLE)

white = b'\xff\xff\xff\x00'
black = b'\x00\x00\x00\x00'

for window,w,h in windows:
    surface = window.GetWindowSurface()
    surface.LockSurface()
    surface.pixels = ((white + black) * int(w/2) + (black + white) * int(w/2)) * int(h/2)
    surface.UnlockSurface()
    window.UpdateWindowSurface()

while True:
    event = SDL2.WaitEvent()
    if event:
        if SDL2.QUIT == event[0]:
            break
        if SDL2.KEYUP == event[0]:
            break

# Show the cursor
SDL2.ShowCursor(SDL2.ENABLE)
SDL2.Quit()
