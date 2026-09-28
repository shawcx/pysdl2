'''Phase 6: SDL2.Timer (callback fires on SDL's timer thread).'''

import threading
import time

import pytest


def test_timer_fires_and_stops_on_zero(sdl):
    hits = []

    def callback(interval):
        hits.append(interval)
        return 0 if len(hits) >= 3 else interval

    timer = sdl.Timer(20, callback)
    assert timer.id != 0
    time.sleep(0.25)

    assert len(hits) == 3
    assert hits[0] == 20
    assert timer.id != 0  # object still valid, timer just self-stopped


def test_timer_none_return_repeats_until_removed(sdl):
    count = [0]

    def callback(interval):
        count[0] += 1
        return None

    timer = sdl.Timer(10, callback)
    time.sleep(0.1)
    fired = count[0]
    assert fired > 3

    timer.Remove()
    assert timer.id == 0
    time.sleep(0.05)
    assert count[0] == fired  # no more callbacks after Remove


def test_timer_remove_is_idempotent(sdl):
    timer = sdl.Timer(1000, lambda i: 0)
    timer.Remove()
    timer.Remove()
    assert timer.id == 0


def test_timer_requires_callable(sdl):
    with pytest.raises(TypeError):
        sdl.Timer(10, 123)


def test_timer_callback_runs_on_another_thread(sdl):
    main = threading.get_ident()
    thread_ids = []

    def callback(interval):
        thread_ids.append(threading.get_ident())
        return 0

    timer = sdl.Timer(10, callback)
    time.sleep(0.1)
    assert timer.id is not None  # keep the timer alive until it fires
    assert thread_ids and thread_ids[0] != main


def test_empty_timer_has_no_id(sdl):
    assert sdl.Timer().id == 0


def test_dropping_timer_object_removes_it(sdl):
    import gc

    count = [0]
    timer = sdl.Timer(10, lambda i: count.__setitem__(0, count[0] + 1) or None)
    time.sleep(0.05)
    assert count[0] > 0

    del timer
    gc.collect()
    fired = count[0]
    time.sleep(0.05)
    assert count[0] == fired  # the timer stopped when its wrapper was collected


def test_dropping_timers_mid_callback_is_safe(sdl):
    # SDL_RemoveTimer doesn't wait for a callback already under way. Timers used
    # to hand SDL the Python callable itself, so dropping one while its callback
    # waited for the GIL called a freed object (a segfault in ~40% of runs of
    # this loop). Run in a subprocess: the failure mode is a crash.
    import os
    import subprocess
    import sys
    code = (
        'import gc, SDL2\n'
        'SDL2.Init(SDL2.INIT_TIMER)\n'
        'for _ in range(1500):\n'
        '    hits = []\n'
        '    timers = [SDL2.Timer(1, lambda i, h=hits: h.append(i) or None) for _ in range(4)]\n'
        '    for _ in range(2000):\n'
        '        pass\n'
        '    del timers\n'
        '    gc.collect()\n'
        'print("ok")\n'
    )
    env = dict(os.environ, PYTHONPATH=os.path.dirname(sdl.__file__))
    out = subprocess.run([sys.executable, '-c', code], env=env, capture_output=True,
                         text=True, timeout=60)
    assert out.returncode == 0 and out.stdout.strip() == 'ok', (out.returncode, out.stderr)


def test_timer_can_remove_itself(sdl):
    fired = []
    holder = {}

    def once(interval):
        fired.append(interval)
        holder['timer'].Remove()  # from inside its own callback
        return interval

    holder['timer'] = sdl.Timer(5, once)
    time.sleep(0.1)
    assert len(fired) == 1
    assert holder['timer'].id == 0
