#!/usr/bin/env python3

# Dumps every event with a readable name. A custom event is registered and
# pushed once a second from a background thread to show cross-thread PushEvent
# and RegisterEvents. An event filter throws away mouse-motion spam, and an
# event watcher counts what gets through. A hint callback reports changes to
# SDL_RENDER_SCALE_QUALITY (press H to cycle it). Esc or close to quit.

import threading
import time
import SDL2

SDL2.Init(SDL2.INIT_VIDEO | SDL2.INIT_EVENTS)

window = SDL2.Window('events', (400, 160))

TICK = SDL2.RegisterEvents(1)

# name lookup for the type ids we might see
NAMES = {getattr(SDL2, n): n for n in dir(SDL2)
         if n.isupper() and isinstance(getattr(SDL2, n), int)
         and (n in ('QUIT',) or n.endswith(('EVENT', 'DOWN', 'UP', 'MOTION', 'WHEEL',
                                            'ADDED', 'REMOVED', 'REMAPPED', 'INPUT',
                                            'EDITING', 'FILE', 'TEXT', 'BEGIN', 'COMPLETE',
                                            'GESTURE', 'RECORD', 'UPDATE', 'CHANGED')))}
NAMES[TICK] = 'TICK (custom)'


def drop_mouse_motion(event):
    return event[0] != SDL2.MOUSEMOTION


SDL2.SetEventFilter(drop_mouse_motion)

# Watchers see every event the filter keeps, as it is pushed (possibly from
# another thread), without consuming it.
counts = {}


def count_events(event):
    if event[0] != getattr(SDL2, 'POLLSENTINEL', None):  # SDL's per-poll marker
        counts[event[0]] = counts.get(event[0], 0) + 1


SDL2.AddEventWatch(count_events)


def hint_changed(name, old, new):
    print(f'  hint {name}: {old!r} -> {new!r}')


SDL2.AddHintCallback('SDL_RENDER_SCALE_QUALITY', hint_changed)  # fires once now
QUALITIES = ['nearest', 'linear', 'best']

stop = threading.Event()


def ticker():
    n = 0
    while not stop.wait(1.0):
        n += 1
        SDL2.PushEvent(TICK, code=n)


threading.Thread(target=ticker, daemon=True).start()

print('waiting for events (mouse-motion is filtered out)...')
running = True
while running:
    event = SDL2.WaitEvent()
    if event is None:
        continue
    kind, data = event
    print(f'{NAMES.get(kind, kind):>22}  {data}')

    if kind == SDL2.QUIT:
        running = False
    elif kind == SDL2.KEYDOWN and data[2] == SDL2.K_ESCAPE:
        running = False
    elif kind == SDL2.KEYDOWN and data[2] == SDL2.K_h:
        QUALITIES.append(QUALITIES.pop(0))
        SDL2.SetHint('SDL_RENDER_SCALE_QUALITY', QUALITIES[0])

stop.set()
SDL2.DelHintCallback('SDL_RENDER_SCALE_QUALITY', hint_changed)
SDL2.DelEventWatch(count_events)
SDL2.SetEventFilter(None)
print('events seen by the watcher:',
      ', '.join(f'{NAMES.get(k, k)} x{n}' for k, n in sorted(counts.items(), key=lambda kv: -kv[1])))
SDL2.Quit()
