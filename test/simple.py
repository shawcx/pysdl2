#!/usr/bin/env python3

import sys
import time
import SDL2

imagePath = sys.argv[1]

SDL2.Init()
surface = SDL2.LoadBMP(imagePath)
window = SDL2.Window(imagePath, (surface.w,surface.h))

print('%s: %dx%d @ %dbpp' % (imagePath, surface.w, surface.h, surface.bpp))

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
            if SDL2.K_h == data[2]:
                window.HideWindow()
                break
            if SDL2.K_m == data[2]:
                window.MinimizeWindow()
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
