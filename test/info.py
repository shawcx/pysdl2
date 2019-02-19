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

print('Number of displays:', SDL2.GetNumVideoDisplays())
print('Current Driver:', SDL2.GetCurrentVideoDriver())
print('Available Drivers:')
for driver in SDL2.GetVideoDrivers():
    print('  ', driver)


SDL2.Quit()
