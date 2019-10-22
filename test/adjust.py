#!/usr/bin/python3

# For LCD monitors using a VGA connection run this program then "auto-adjust" the monitor

import SDL2

# Initialize SDL2
SDL2.Init(SDL2.INIT_VIDEO)

# Get the current resolution
params = SDL2.GetDisplayMode(0)
w = params[1]
h = params[2]

# Create the window
window = SDL2.Window("Adjust", size=(w,h), flags=SDL2.WINDOW_FULLSCREEN)

# Hide the cursor
SDL2.ShowCursor(SDL2.DISABLE)

surface = window.GetWindowSurface()

white = b'\xff\xff\xff\x00'
black = b'\x00\x00\x00\x00'

# blit alternating black and white pixels to the screen
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
