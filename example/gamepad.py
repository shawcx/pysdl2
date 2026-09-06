#!/usr/bin/env python3

# Game-controller / joystick monitor. Plug in a pad and it opens automatically;
# move sticks and press buttons to see live events. Ctrl-C to quit.
#
# With no real device, pass --virtual to attach a virtual joystick and drive it
# with the number keys 0-5 (needs SDL >= 2.0.14).

import sys
import SDL2

SDL2.Init(SDL2.INIT_VIDEO | SDL2.INIT_JOYSTICK | SDL2.INIT_GAMECONTROLLER)

AXIS_NAMES = {
    SDL2.CONTROLLER_AXIS_LEFTX: 'LeftX', SDL2.CONTROLLER_AXIS_LEFTY: 'LeftY',
    SDL2.CONTROLLER_AXIS_RIGHTX: 'RightX', SDL2.CONTROLLER_AXIS_RIGHTY: 'RightY',
    SDL2.CONTROLLER_AXIS_TRIGGERLEFT: 'TrigL', SDL2.CONTROLLER_AXIS_TRIGGERRIGHT: 'TrigR',
}

window = SDL2.Window('gamepad', (360, 120))

controllers = {}   # instance id -> GameController
joysticks = {}     # instance id -> Joystick


def add_device(device_index):
    if SDL2.IsGameController(device_index):
        gc = SDL2.GameController(device_index)
        iid = gc.GetJoystick().InstanceID()
        if iid in controllers:
            return
        controllers[iid] = gc
        print(f'+ controller {iid}: {gc.Name()!r}')
    else:
        js = SDL2.Joystick(device_index)
        if js.InstanceID() in joysticks:
            return
        joysticks[js.InstanceID()] = js
        print(f'+ joystick {js.InstanceID()}: {js.Name()!r}  '
              f'{js.NumAxes()} axes, {js.NumButtons()} buttons, {js.NumHats()} hats')


virtual_index = None
if '--virtual' in sys.argv and hasattr(SDL2, 'JoystickAttachVirtual'):
    virtual_index = SDL2.JoystickAttachVirtual(SDL2.JOYSTICK_TYPE_GAMECONTROLLER, 6, 8, 1)
    vjs = SDL2.Joystick(virtual_index)
    print('attached virtual joystick; press number keys 0-5 to nudge its axes')

for i in range(SDL2.NumJoysticks()):
    add_device(i)

running = True
while running:
    event = SDL2.WaitEvent()
    if event is None:
        continue
    kind, data = event

    if kind == SDL2.QUIT:
        running = False

    elif kind in (SDL2.CONTROLLERDEVICEADDED, SDL2.JOYDEVICEADDED):
        add_device(data[0])
    elif kind in (SDL2.CONTROLLERDEVICEREMOVED, SDL2.JOYDEVICEREMOVED):
        controllers.pop(data[0], None)
        joysticks.pop(data[0], None)
        print(f'- device {data[0]} removed')

    elif kind == SDL2.CONTROLLERAXISMOTION:
        which, axis, value = data
        if abs(value) > 8000:
            print(f'  [{which}] {AXIS_NAMES.get(axis, axis):>6} = {value:+6d}')
    elif kind in (SDL2.CONTROLLERBUTTONDOWN, SDL2.CONTROLLERBUTTONUP):
        which, button, state = data
        gc = controllers.get(which)
        if gc:
            gc.Rumble(0x8000, 0x8000, 120) if state else None
        print(f'  [{which}] button {button} {"down" if state else "up"}')

    elif kind == SDL2.JOYHATMOTION:
        print(f'  [{data[0]}] hat {data[1]} -> {data[2]}')
    elif kind in (SDL2.JOYBUTTONDOWN, SDL2.JOYBUTTONUP):
        print(f'  [{data[0]}] joy button {data[1]} {"down" if data[2] else "up"}')

    elif kind == SDL2.KEYDOWN and virtual_index is not None:
        sym = data[2]
        if SDL2.K_0 <= sym <= SDL2.K_5:
            axis = sym - SDL2.K_0
            vjs.SetVirtualAxis(axis, 20000)
        elif sym == SDL2.K_ESCAPE:
            running = False

if virtual_index is not None:
    SDL2.JoystickDetachVirtual(virtual_index)
SDL2.Quit()
