'''Phase 5: the event queue - push/poll/peep/flush, filters, custom events.'''

import time

import pytest


@pytest.fixture
def drained(sdl):
    '''A quiet event queue and a freshly registered custom event type.'''
    sdl.PumpEvents()
    while sdl.PollEvent() is not None:
        pass
    return sdl.RegisterEvents(2)


def test_push_poll_roundtrip(sdl, drained):
    mytype = drained
    assert sdl.PushEvent(mytype, 42, 7) is True
    assert sdl.PollEvent() == (mytype, (42, 7))
    assert sdl.PollEvent() is None


def test_has_event_and_flush(sdl, drained):
    a, b = drained, drained + 1
    sdl.PushEvent(a)
    sdl.PushEvent(b)
    assert sdl.HasEvent(a) is True
    assert sdl.HasEvents(a, b) is True

    sdl.FlushEvent(a)
    assert sdl.HasEvent(a) is False
    assert sdl.HasEvent(b) is True

    sdl.FlushEvents(a, b)
    assert sdl.HasEvents(a, b) is False


def test_peep_events_peek_then_get(sdl, drained):
    mytype = drained
    for code in (10, 20, 30):
        sdl.PushEvent(mytype, code)

    peeked = sdl.PeepEvents(10, sdl.PEEKEVENT, mytype, mytype)
    assert [data[0] for _, data in peeked] == [10, 20, 30]
    assert sdl.HasEvent(mytype) is True  # peek does not remove

    got = sdl.PeepEvents(10, sdl.GETEVENT, mytype, mytype)
    assert [data[0] for _, data in got] == [10, 20, 30]
    assert sdl.HasEvent(mytype) is False

    with pytest.raises(ValueError):
        sdl.PeepEvents(1, sdl.ADDEVENT)


def test_event_state(sdl):
    previous = sdl.EventState(sdl.MOUSEMOTION, sdl.DISABLE)
    try:
        assert sdl.GetEventState(sdl.MOUSEMOTION) == sdl.DISABLE
    finally:
        sdl.EventState(sdl.MOUSEMOTION, previous)


def test_wait_event_timeout_returns_none_when_idle(sdl, drained):
    start = time.monotonic()
    assert sdl.WaitEventTimeout(30) is None
    assert (time.monotonic() - start) * 1000 >= 15


def test_wait_event_timeout_delivers_pushed_event(sdl, drained):
    sdl.PushEvent(drained, 99)
    assert sdl.WaitEventTimeout(100) == (drained, (99, 0))


def test_quit_requested(sdl, drained):
    assert sdl.QuitRequested() is False
    sdl.PushEvent(sdl.QUIT)
    assert sdl.QuitRequested() is True
    sdl.FlushEvent(sdl.QUIT)


def test_event_filter_drops_and_keeps(sdl, drained):
    a, b = drained, drained + 1
    seen = []

    def keep_only_a(event):
        # SDL >= 2.0.18 also filters its own POLLSENTINEL marker; ignore it.
        if event[0] == getattr(sdl, 'POLLSENTINEL', None):
            return True
        seen.append(event[0])
        return event[0] == a

    sdl.SetEventFilter(keep_only_a)
    assert sdl.GetEventFilter() is keep_only_a
    try:
        sdl.PushEvent(a, 1)
        sdl.PushEvent(b, 2)
        delivered = []
        while (event := sdl.PollEvent()) is not None:
            delivered.append(event[0])
    finally:
        sdl.SetEventFilter(None)

    assert set(seen) == {a, b}
    assert delivered == [a]
    assert sdl.GetEventFilter() is None


def test_filter_events_over_queue(sdl, drained):
    mytype = drained
    for code in (1, 2, 3):
        sdl.PushEvent(mytype, code)
    sdl.FilterEvents(lambda event: event[1][0] != 2)
    remaining = [data[0] for _, data in sdl.PeepEvents(10, sdl.GETEVENT, mytype, mytype)]
    assert remaining == [1, 3]


def test_set_event_filter_rejects_non_callable(sdl):
    with pytest.raises(TypeError):
        sdl.SetEventFilter(42)


# --- phase 13: event watchers ----------------------------------------------

def test_event_watch_sees_pushed_events(sdl, drained):
    seen = []

    def watch(event):
        if event[0] == drained:  # PollEvent also pumps SDL's own events
            seen.append(event)

    sdl.AddEventWatch(watch)
    try:
        sdl.PushEvent(drained, 7)
        assert seen == [(drained, (7, 0))]
        assert sdl.PollEvent() == (drained, (7, 0))  # still queued; watching is passive
    finally:
        sdl.DelEventWatch(watch)
    sdl.PushEvent(drained, 8)
    assert len(seen) == 1
    sdl.FlushEvent(drained)


def test_event_watch_runs_after_filter(sdl, drained):
    seen = []

    def watch(event):
        if event[0] == drained:
            seen.append(event[1][0])

    sdl.AddEventWatch(watch)
    sdl.SetEventFilter(lambda event: event[1][0] != 2)
    try:
        sdl.PushEvent(drained, 1)
        sdl.PushEvent(drained, 2)  # dropped by the filter: watchers never see it
    finally:
        sdl.SetEventFilter(None)
        sdl.DelEventWatch(watch)
    assert seen == [1]
    sdl.FlushEvent(drained)


def test_event_watch_registration_errors(sdl):
    with pytest.raises(TypeError):
        sdl.AddEventWatch(42)
    with pytest.raises(ValueError):
        sdl.DelEventWatch(lambda event: None)


def test_event_watch_can_remove_itself(sdl, drained):
    calls = []

    def once(event):
        if event[0] == drained:
            calls.append(event[1][0])
            sdl.DelEventWatch(once)

    sdl.AddEventWatch(once)
    sdl.PushEvent(drained, 1)
    sdl.PushEvent(drained, 2)
    assert calls == [1]
    sdl.FlushEvent(drained)


def test_event_watch_releases_callable(sdl):
    import gc
    import weakref

    class Watch:
        def __call__(self, event):
            pass

    watch = Watch()
    ref = weakref.ref(watch)
    sdl.AddEventWatch(watch)
    del watch
    gc.collect()
    assert ref() is not None
    sdl.DelEventWatch(ref())
    gc.collect()
    assert ref() is None


def test_event_watch_cross_thread_no_deadlock(sdl, drained):
    # An SDL timer thread pushes events (firing the watcher, which needs the
    # GIL) while this thread polls; before the GIL rule this could deadlock.
    import threading
    import time

    threads = set()

    def watch(event):
        if event[0] == drained:
            threads.add(threading.get_ident())

    def tick(interval):
        sdl.PushEvent(drained, 1)
        return interval

    sdl.AddEventWatch(watch)
    timer = sdl.Timer(1, tick)
    try:
        deadline = time.monotonic() + 0.5
        while time.monotonic() < deadline:
            sdl.PollEvent()
    finally:
        timer.Remove()
        sdl.DelEventWatch(watch)
    sdl.FlushEvent(drained)
    assert threads - {threading.get_ident()}  # the watcher ran on the timer thread
