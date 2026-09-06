#!/usr/bin/env python3

import sys
import time
import SDL2

SDL2.Init()

print('Platform:',  SDL2.GetPlatform())
print('SDL2 Version: %d.%d.%d' % SDL2.Version())
print()
print('CPU Count:  ', SDL2.GetCPUCount())
print('Cache Line: ', SDL2.GetCPUCacheLineSize())
print('Has3DNow:   ', SDL2.Has3DNow())
print('HasAVX:     ', SDL2.HasAVX())
print('HasAVX2:    ', SDL2.HasAVX2())
print('HasAltiVec: ', SDL2.HasAltiVec())
print('HasMMX:     ', SDL2.HasMMX())
print('HasSSE:     ', SDL2.HasSSE())
print('HasSSE2:    ', SDL2.HasSSE2())
print('HasSSE3:    ', SDL2.HasSSE3())
print('HasSSE41:   ', SDL2.HasSSE41())
print('HasSSE42:   ', SDL2.HasSSE42())
print()

drivers = SDL2.GetVideoDrivers()
current = SDL2.GetCurrentVideoDriver()
print('Number of drivers:', len(drivers))
for driver in drivers:
    print('-', driver, '(current)' if current == driver else '')
print()

numberOfDisplays = SDL2.GetNumVideoDisplays()
print('Number of displays:', numberOfDisplays)
for idx in range(numberOfDisplays):
    flags,width,height,rate = SDL2.GetDisplayMode(idx)
    print('-', f'Display Mode: {width}x{height} @ {rate}Hz')
    flags,width,height,rate = SDL2.GetDesktopDisplayMode(idx)
    print('-', f'Desktop Display Mode: {width}x{height} @ {rate}Hz')
    ddpi,hdpi,vdpi = SDL2.GetDisplayDPI(idx)
    print('-', f'DPI: {hdpi} H, {vdpi} V, {ddpi} D')
print()

numberOfRenderers = SDL2.GetNumRenderDrivers()
print('Number of renderers:', numberOfRenderers)
for idx in range(numberOfRenderers):
    name,flags,fmts,max_width,max_height = SDL2.GetRenderDriverInfo(idx)
    print('-', name, hex(flags))
print()

SDL2.Quit()
