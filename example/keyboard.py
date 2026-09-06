#!/usr/bin/env python3

# Keyboard / text-input / mouse event echo. Type to see TEXTINPUT, hold keys for
# repeat, move the mouse over the window, click to cycle the system cursor.
# Esc or close to quit.

import SDL2

SDL2.Init(SDL2.INIT_VIDEO)

window = SDL2.Window('keyboard', (480, 240))
renderer = window.CreateRenderer()
renderer.SetRenderDrawColor(20, 20, 28, 255)
renderer.Clear()
renderer.Present()

SDL2.StartTextInput()
SDL2.SetTextInputRect((8, 8, 200, 24))

CURSORS = [SDL2.SYSTEM_CURSOR_ARROW, SDL2.SYSTEM_CURSOR_HAND,
           SDL2.SYSTEM_CURSOR_CROSSHAIR, SDL2.SYSTEM_CURSOR_WAIT]
cursors = [SDL2.Cursor(c) for c in CURSORS]
which = 0
cursors[which].Set()

typed = ''
running = True
while running:
    event = SDL2.WaitEvent()
    if event is None:
        continue
    kind, data = event

    if kind == SDL2.QUIT:
        running = False

    elif kind == SDL2.KEYDOWN:
        state, scancode, sym, mod, repeat = data
        name = SDL2.GetKeyName(sym) or SDL2.GetScancodeName(scancode)
        print(f'KEYDOWN  {name!r:<14} sym={sym} scancode={scancode} '
              f'mod={SDL2.GetModState():#06x} repeat={bool(repeat)}')
        if sym == SDL2.K_ESCAPE:
            running = False
        elif sym == SDL2.K_BACKSPACE:
            typed = typed[:-1]
            print('  buffer:', repr(typed))

    elif kind == SDL2.TEXTINPUT:
        typed += data
        print(f'TEXTINPUT {data!r}   buffer: {typed!r}')

    elif kind == SDL2.TEXTEDITING:
        text, start, length = data
        print(f'TEXTEDITING text={text!r} start={start} length={length}')

    elif kind == SDL2.MOUSEMOTION:
        _state, x, y, xrel, yrel = data
        print(f'MOUSEMOTION ({x:>4},{y:>4}) rel ({xrel:+d},{yrel:+d})  '
              f'focus={SDL2.GetMouseFocus() is not None}')

    elif kind == SDL2.MOUSEBUTTONDOWN:
        which = (which + 1) % len(cursors)
        cursors[which].Set()
        print(f'cursor -> {CURSORS[which]}')

SDL2.StopTextInput()
SDL2.Quit()
