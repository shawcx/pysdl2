#!/usr/bin/env python3

# A fixed-rate animation driven by SDL2.Timer. The timer callback runs on SDL's
# timer thread, so it does not touch the renderer - it just pushes a REDRAW
# event that the main loop handles. The title shows battery / power state.
# Esc or close to quit.

import math
import SDL2

SDL2.Init(SDL2.INIT_VIDEO | SDL2.INIT_TIMER | SDL2.INIT_EVENTS)

W, H = 400, 400
window = SDL2.Window('timer', (W, H))
renderer = window.CreateRenderer(flags=SDL2.RENDERER_ACCELERATED)

REDRAW = SDL2.RegisterEvents(1)

frame = 0


def on_tick(interval):
    # 60 Hz; return the interval to keep going, 0 to stop.
    SDL2.PushEvent(REDRAW)
    return interval


timer = SDL2.Timer(1000 // 60, on_tick)

POWER = {SDL2.POWERSTATE_UNKNOWN: '?', SDL2.POWERSTATE_ON_BATTERY: 'battery',
         SDL2.POWERSTATE_NO_BATTERY: 'AC', SDL2.POWERSTATE_CHARGING: 'charging',
         SDL2.POWERSTATE_CHARGED: 'charged'}


def render():
    global frame
    frame += 1
    t = frame / 60.0

    renderer.SetRenderDrawColor(18, 18, 26, 255)
    renderer.Clear()

    cx, cy = W / 2, H / 2
    for i in range(12):
        a = t + i * math.tau / 12
        r = 150 + 30 * math.sin(t * 2 + i)
        x, y = cx + r * math.cos(a), cy + r * math.sin(a)
        shade = int(128 + 127 * math.sin(t + i))
        renderer.SetRenderDrawColor(shade, 200 - shade // 2, 255 - shade, 255)
        renderer.FillRect((x - 8, y - 8, 16, 16))

    # clock hand
    renderer.SetRenderDrawColor(240, 240, 240, 255)
    renderer.DrawLine(cx, cy, cx + 120 * math.cos(t), cy + 120 * math.sin(t))

    renderer.Present()

    if frame % 30 == 0:
        state, secs, pct = SDL2.GetPowerInfo()
        window.SetWindowTitle(f'timer  |  {frame} frames  |  {POWER[state]} {pct}%')


running = True
while running:
    event = SDL2.WaitEvent()
    if event is None:
        continue
    kind, data = event

    if kind == REDRAW:
        render()
    elif kind == SDL2.QUIT:
        running = False
    elif kind == SDL2.KEYDOWN and data[2] == SDL2.K_ESCAPE:
        running = False

timer.Remove()
SDL2.Quit()
