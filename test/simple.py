#!/usr/bin/env python3

import sys
import time
import SDL2

SDL2.Init()

print('Version: %d.%d.%d' % SDL2.Version())


print("Has3DNow",   SDL2.Has3DNow())
print("HasAVX",     SDL2.HasAVX())
print("HasAVX2",    SDL2.HasAVX2())
print("HasAltiVec", SDL2.HasAltiVec())
print("HasMMX",     SDL2.HasMMX())
print("HasSSE",     SDL2.HasSSE())
print("HasSSE2",    SDL2.HasSSE2())
print("HasSSE3",    SDL2.HasSSE3())
print("HasSSE41",   SDL2.HasSSE41())
print("HasSSE42",   SDL2.HasSSE42())


surface = SDL2.LoadBMP(sys.argv[1])

print('%s: %dx%d @ %d' % (sys.argv[1], surface.w, surface.h, surface.bpp))

window = SDL2.Window(sys.argv[1], (640,480))
#window = SDL2.Window(sys.argv[1], flags=SDL2.WINDOW_FULLSCREEN_DESKTOP)

print(window.GetWindowSize())
print(SDL2.GetVideoDrivers())

print(SDL2.GetCurrentVideoDriver())

renderer = window.CreateRenderer()

#tex2 = renderer.LoadTexture(sys.argv[2])
#for x in range(0,200,10):
#    renderer.SetRenderDrawColor(128,128,x)
#    renderer.RenderClear()
#    renderer.RenderPresent()
#    time.sleep(0.1)

texture = renderer.CreateTextureFromSurface(surface)
texture.QueryTexture()

renderer.RenderClear()
renderer.RenderCopy(texture)
renderer.RenderPresent()

quit = False
while not quit:
    while True:
        result = SDL2.PollEvent()
        if result is None:
            break

        event,data = result

        if SDL2.QUIT == event:
            quit = True
            break

        if SDL2.KEYDOWN == event:
            # quit on escape
            if SDL2.K_ESCAPE == data[2]:
                quit = True
                break
#            # take a screenshot on c
#            if SDL2.K_c == data[2]:
#                self.Capture()
#            # display info on i
#            if SDL2.K_i == data[2]:
#                self.fnInfo()

del texture
del surface
del renderer
del window

SDL2.Quit()
