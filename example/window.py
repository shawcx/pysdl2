#!/usr/bin/env python3

# Window-state playground. Keys:
#   b  toggle border        g  toggle mouse grab      o  cycle opacity
#   t  toggle always-on-top f  flash the window       m  message box
#   i  print display / Vulkan info
# Esc or close to quit.

import SDL2

SDL2.Init(SDL2.INIT_VIDEO)

window = SDL2.Window('window flags', (480, 320), flags=SDL2.WINDOW_RESIZABLE)
renderer = window.CreateRenderer()

bordered = True
grabbed = False
on_top = False
opacity_steps = [1.0, 0.75, 0.5]
opacity = 0


def safe(label, fn):
    try:
        fn()
        print(f'  {label}: ok')
    except SDL2.error as exc:
        print(f'  {label}: {exc}')


def show_info():
    d = window.GetWindowDisplayIndex()
    print(f'display {d}: {SDL2.GetDisplayName(d)!r}')
    print(f'  bounds        {SDL2.GetDisplayBounds(d)}')
    print(f'  usable bounds {SDL2.GetDisplayUsableBounds(d)}')
    print(f'  orientation   {SDL2.GetDisplayOrientation(d)}')
    print(f'  modes         {SDL2.GetNumDisplayModes(d)}  (current {SDL2.GetCurrentDisplayMode(d)})')
    print(f'  window flags  {window.GetWindowFlags():#010x}')
    print(f'  borders       {window.GetWindowBordersSize()}')
    locales = getattr(SDL2, 'GetPreferredLocales', list)()
    print(f'  locales       {locales}')
    try:
        SDL2.Vulkan_LoadLibrary()
        print(f'  vulkan exts   {SDL2.Vulkan_GetInstanceExtensions()}')
        SDL2.Vulkan_UnloadLibrary()
    except SDL2.error as exc:
        print(f'  vulkan        unavailable ({exc})')


def redraw():
    renderer.SetRenderDrawColor(30, 34, 48, 255)
    renderer.Clear()
    renderer.SetRenderDrawColor(90, 160, 240, 255)
    renderer.DrawRect((20, 20, *[c - 40 for c in window.GetWindowSize()]))
    renderer.Present()


redraw()
running = True
while running:
    event = SDL2.WaitEvent()
    if event is None:
        continue
    kind, data = event

    if kind == SDL2.QUIT:
        running = False

    elif kind == SDL2.WINDOWEVENT:
        redraw()

    elif kind == SDL2.KEYDOWN:
        sym = data[2]
        if sym == SDL2.K_ESCAPE:
            running = False
        elif sym == SDL2.K_b:
            bordered = not bordered
            window.SetWindowBordered(bordered)
        elif sym == SDL2.K_g:
            grabbed = not grabbed
            window.SetWindowGrab(grabbed)
            print(f'grab -> {window.GetWindowGrab()}')
        elif sym == SDL2.K_o:
            opacity = (opacity + 1) % len(opacity_steps)
            safe(f'opacity {opacity_steps[opacity]}',
                 lambda: window.SetWindowOpacity(opacity_steps[opacity]))
        elif sym == SDL2.K_t and hasattr(window, 'SetWindowAlwaysOnTop'):
            on_top = not on_top
            window.SetWindowAlwaysOnTop(on_top)
            print(f'always-on-top -> {on_top}')
        elif sym == SDL2.K_f and hasattr(window, 'FlashWindow'):
            safe('flash', lambda: window.FlashWindow(SDL2.FLASH_BRIEFLY))
        elif sym == SDL2.K_m:
            choice = SDL2.ShowMessageBox({
                'window': window,
                'flags': SDL2.MESSAGEBOX_INFORMATION,
                'title': 'pysdl2',
                'message': 'Keep going?',
                'buttons': [
                    (0, 'Quit', SDL2.MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT),
                    (1, 'Continue', SDL2.MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT),
                ],
            })
            print(f'message box -> {choice}')
            if choice == 0:
                running = False
        elif sym == SDL2.K_i:
            show_info()

SDL2.Quit()
