#include "pysdl.h"

// The event queue: the `_event()` converter (SDL_Event -> (type, data)), the
// pump/poll/wait/peep/push functions, and an optional Python event filter.
// Registered in PyInit_SDL2 via PyModule_AddFunctions(module, pysdl_events_methods).

//=========================================================
// SDL_Event -> (type, data)
//=========================================================

// `consume` frees the drop.file string SDL handed us; it must be 0 when the
// event still belongs to SDL (event filter, SDL_PEEKEVENT).
static PyObject * _event(SDL_Event *event, int consume) {
    PyObject *data;

    switch(event->type) {
    case SDL_KEYDOWN:
    case SDL_KEYUP: {
            SDL_KeyboardEvent *e = &event->key;
            data = Py_BuildValue("(iiiiO)", e->state, e->keysym.scancode,
                e->keysym.sym, e->keysym.mod, e->repeat ? Py_True : Py_False);
        }
        break;
    case SDL_TEXTINPUT:
        data = PyUnicode_FromString(event->text.text);
        break;
    case SDL_TEXTEDITING:
        data = Py_BuildValue("(sii)", event->edit.text, event->edit.start, event->edit.length);
        break;
    case SDL_MOUSEMOTION: {
            SDL_MouseMotionEvent *e = &event->motion;
            data = Py_BuildValue("(iiiii)", e->state, e->x, e->y, e->xrel, e->yrel);
        }
        break;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP: {
            SDL_MouseButtonEvent *e = &event->button;
            data = Py_BuildValue("(iiiii)", e->which, e->button, e->state, e->x, e->y);
        }
        break;
    case SDL_MOUSEWHEEL: {
            SDL_MouseWheelEvent *e = &event->wheel;
            data = Py_BuildValue("(iiii)", e->which, e->x, e->y, e->direction);
        }
        break;
    case SDL_JOYAXISMOTION:
        data = Py_BuildValue("(iii)", event->jaxis.which, event->jaxis.axis, event->jaxis.value);
        break;
    case SDL_JOYBALLMOTION:
        data = Py_BuildValue("(iiii)", event->jball.which, event->jball.ball,
            event->jball.xrel, event->jball.yrel);
        break;
    case SDL_JOYHATMOTION:
        data = Py_BuildValue("(iii)", event->jhat.which, event->jhat.hat, event->jhat.value);
        break;
    case SDL_JOYBUTTONDOWN:
    case SDL_JOYBUTTONUP:
        data = Py_BuildValue("(iii)", event->jbutton.which, event->jbutton.button, event->jbutton.state);
        break;
    case SDL_JOYDEVICEADDED:
    case SDL_JOYDEVICEREMOVED:
        data = Py_BuildValue("(i)", event->jdevice.which);
        break;
    case SDL_CONTROLLERAXISMOTION:
        data = Py_BuildValue("(iii)", event->caxis.which, event->caxis.axis, event->caxis.value);
        break;
    case SDL_CONTROLLERBUTTONDOWN:
    case SDL_CONTROLLERBUTTONUP:
        data = Py_BuildValue("(iii)", event->cbutton.which, event->cbutton.button, event->cbutton.state);
        break;
    case SDL_CONTROLLERDEVICEADDED:
    case SDL_CONTROLLERDEVICEREMOVED:
    case SDL_CONTROLLERDEVICEREMAPPED:
#if SDL_VERSION_ATLEAST(2,30,0)
    case SDL_CONTROLLERSTEAMHANDLEUPDATED:
#endif
        data = Py_BuildValue("(i)", event->cdevice.which);
        break;
#if SDL_VERSION_ATLEAST(2,0,14)
    case SDL_CONTROLLERTOUCHPADDOWN:
    case SDL_CONTROLLERTOUCHPADMOTION:
    case SDL_CONTROLLERTOUCHPADUP: {
            SDL_ControllerTouchpadEvent *e = &event->ctouchpad;
            data = Py_BuildValue("(iiifff)", e->which, e->touchpad, e->finger, e->x, e->y, e->pressure);
        }
        break;
    case SDL_CONTROLLERSENSORUPDATE: {
            SDL_ControllerSensorEvent *e = &event->csensor;
            data = Py_BuildValue("(ii(fff))", e->which, e->sensor, e->data[0], e->data[1], e->data[2]);
        }
        break;
#endif
#if SDL_VERSION_ATLEAST(2,24,0)
    case SDL_JOYBATTERYUPDATED:
        data = Py_BuildValue("(ii)", event->jbattery.which, event->jbattery.level);
        break;
#endif
    case SDL_WINDOWEVENT:
        data = Py_BuildValue("(iiiI)", event->window.event, event->window.data1,
            event->window.data2, event->window.windowID);
        break;
#if SDL_VERSION_ATLEAST(2,0,9)
    case SDL_DISPLAYEVENT:
        data = Py_BuildValue("(iiI)", event->display.event, event->display.data1, event->display.display);
        break;
    case SDL_SENSORUPDATE: {
            const float *v = event->sensor.data;
            data = Py_BuildValue("(i(ffffff))", event->sensor.which,
                v[0], v[1], v[2], v[3], v[4], v[5]);
        }
        break;
#endif
    case SDL_AUDIODEVICEADDED:
    case SDL_AUDIODEVICEREMOVED:
        data = Py_BuildValue("(Ii)", event->adevice.which, event->adevice.iscapture);
        break;
    case SDL_FINGERMOTION:
    case SDL_FINGERDOWN:
    case SDL_FINGERUP: {
            SDL_TouchFingerEvent *e = &event->tfinger;
            data = Py_BuildValue("(LLfffffI)", (long long)e->touchId, (long long)e->fingerId,
                e->x, e->y, e->dx, e->dy, e->pressure, e->windowID);
        }
        break;
    case SDL_MULTIGESTURE: {
            SDL_MultiGestureEvent *e = &event->mgesture;
            data = Py_BuildValue("(Lffffi)", (long long)e->touchId,
                e->dTheta, e->dDist, e->x, e->y, e->numFingers);
        }
        break;
    case SDL_DOLLARGESTURE:
    case SDL_DOLLARRECORD: {
            SDL_DollarGestureEvent *e = &event->dgesture;
            data = Py_BuildValue("(LLifff)", (long long)e->touchId, (long long)e->gestureId,
                e->numFingers, e->error, e->x, e->y);
        }
        break;
    case SDL_DROPFILE:
    case SDL_DROPTEXT:
        data = Py_BuildValue("(sI)", event->drop.file ? event->drop.file : "", event->drop.windowID);
        if(consume) {
            SDL_free(event->drop.file);
        }
        break;
    case SDL_DROPBEGIN:
    case SDL_DROPCOMPLETE:
        data = Py_BuildValue("(I)", event->drop.windowID);
        break;
    case SDL_QUIT:
        data = Py_NewRef(Py_None);
        break;
    default:
        // SDL_USEREVENT .. SDL_LASTEVENT-1 are application / RegisterEvents types.
        if(event->type >= SDL_USEREVENT) {
            data = Py_BuildValue("(iI)", event->user.code, event->user.windowID);
        } else {
            data = Py_NewRef(Py_None);
        }
    }

    if(NULL == data) {
        return NULL;
    }
    return Py_BuildValue("(IN)", event->type, data);
}

//=========================================================
// pump / poll / wait
//=========================================================

static PyObject * PySDL_PumpEvents(PyObject *self, PyObject *ign) {
    SDL_PumpEvents();
    Py_RETURN_NONE;
}

static PyObject * PySDL_PollEvent(PyObject *self, PyObject *ign) {
    SDL_Event event;
    if(0 == SDL_PollEvent(&event)) {
        Py_RETURN_NONE;
    }
    return _event(&event, 1);
}

static PyObject * PySDL_WaitEvent(PyObject *self, PyObject *ign) {
    SDL_Event event;
    int ok;
    Py_BEGIN_ALLOW_THREADS
        ok = SDL_WaitEvent(&event);
    Py_END_ALLOW_THREADS
    if(0 == ok) {
        Py_RETURN_NONE;
    }
    return _event(&event, 1);
}

static PyObject * PySDL_WaitEventTimeout(PyObject *self, PyObject *arg) {
    long ms = PyLong_AsLong(arg);
    if(-1 == ms && PyErr_Occurred()) {
        return NULL;
    }
    SDL_Event event;
    int ok;
    Py_BEGIN_ALLOW_THREADS
        ok = SDL_WaitEventTimeout(&event, (int)ms);
    Py_END_ALLOW_THREADS
    if(0 == ok) {
        Py_RETURN_NONE;
    }
    return _event(&event, 1);
}

//=========================================================
// queue manipulation
//=========================================================

static PyObject * PySDL_PushEvent(PyObject *self, PyObject *args, PyObject *kwds) {
    unsigned int type;
    int code = 0;
    unsigned int windowID = 0;

    static char *kwlist[] = {"type", "code", "windowID", NULL};
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "I|iI", kwlist, &type, &code, &windowID)) {
        return NULL;
    }

    SDL_Event event;
    SDL_memset(&event, 0, sizeof(event));
    event.type = type;
    event.user.code = code;
    event.user.windowID = windowID;

    int rc = SDL_PushEvent(&event);
    if(0 > rc) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }
    return PyBool_FromLong(rc);  // 1 pushed, 0 dropped by a filter
}

static PyObject * PySDL_PeepEvents(PyObject *self, PyObject *args) {
    int numevents;
    int action = SDL_PEEKEVENT;
    unsigned int minType = SDL_FIRSTEVENT;
    unsigned int maxType = SDL_LASTEVENT;

    if(!PyArg_ParseTuple(args, "i|iII", &numevents, &action, &minType, &maxType)) {
        return NULL;
    }
    if(SDL_ADDEVENT == action) {
        PyErr_SetString(PyExc_ValueError, "PeepEvents cannot ADD - use PushEvent");
        return NULL;
    }
    if(numevents <= 0) {
        return PyList_New(0);
    }

    SDL_Event *events = PyMem_New(SDL_Event, numevents);
    if(NULL == events) {
        return PyErr_NoMemory();
    }

    int got;
    Py_BEGIN_ALLOW_THREADS
        got = SDL_PeepEvents(events, numevents, (SDL_eventaction)action, minType, maxType);
    Py_END_ALLOW_THREADS
    if(0 > got) {
        PyMem_Free(events);
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }

    PyObject *list = PyList_New(got);
    if(NULL == list) {
        PyMem_Free(events);
        return NULL;
    }
    for(int idx = 0; idx < got; ++idx) {
        PyObject *item = _event(&events[idx], SDL_GETEVENT == action);
        if(NULL == item) {
            Py_DECREF(list);
            PyMem_Free(events);
            return NULL;
        }
        PyList_SET_ITEM(list, idx, item);
    }
    PyMem_Free(events);
    return list;
}

static PyObject * PySDL_FlushEvent(PyObject *self, PyObject *arg) {
    long type = PyLong_AsLong(arg);
    if(-1 == type && PyErr_Occurred()) {
        return NULL;
    }
    SDL_FlushEvent((Uint32)type);
    Py_RETURN_NONE;
}

static PyObject * PySDL_FlushEvents(PyObject *self, PyObject *args) {
    unsigned int minType = SDL_FIRSTEVENT;
    unsigned int maxType = SDL_LASTEVENT;
    if(!PyArg_ParseTuple(args, "|II", &minType, &maxType)) {
        return NULL;
    }
    SDL_FlushEvents(minType, maxType);
    Py_RETURN_NONE;
}

static PyObject * PySDL_HasEvent(PyObject *self, PyObject *arg) {
    long type = PyLong_AsLong(arg);
    if(-1 == type && PyErr_Occurred()) {
        return NULL;
    }
    return PyBool_FromLong(SDL_HasEvent((Uint32)type));
}

static PyObject * PySDL_HasEvents(PyObject *self, PyObject *args) {
    unsigned int minType = SDL_FIRSTEVENT;
    unsigned int maxType = SDL_LASTEVENT;
    if(!PyArg_ParseTuple(args, "|II", &minType, &maxType)) {
        return NULL;
    }
    return PyBool_FromLong(SDL_HasEvents(minType, maxType));
}

static PyObject * PySDL_RegisterEvents(PyObject *self, PyObject *arg) {
    long numevents = PyLong_AsLong(arg);
    if(-1 == numevents && PyErr_Occurred()) {
        return NULL;
    }
    Uint32 base = SDL_RegisterEvents((int)numevents);
    if((Uint32)-1 == base) {
        PyErr_SetString(pysdl_Error, "could not register events (queue full)");
        return NULL;
    }
    return PyLong_FromUnsignedLong(base);
}

static PyObject * PySDL_EventState(PyObject *self, PyObject *args) {
    unsigned int type;
    int state;
    if(!PyArg_ParseTuple(args, "Ii", &type, &state)) {
        return NULL;
    }
    return PyLong_FromLong(SDL_EventState(type, state));
}

static PyObject * PySDL_GetEventState(PyObject *self, PyObject *arg) {
    long type = PyLong_AsLong(arg);
    if(-1 == type && PyErr_Occurred()) {
        return NULL;
    }
    return PyLong_FromLong(SDL_EventState((Uint32)type, SDL_QUERY));
}

static PyObject * PySDL_QuitRequested(PyObject *self, PyObject *ign) {
    return PyBool_FromLong(SDL_QuitRequested());
}

//=========================================================
// event filter (optional / advanced)
//=========================================================

static PyObject *_py_event_filter = NULL;

// Run `callable` on the converted event; returns 1 to keep the event, 0 to drop.
static int _run_filter(PyObject *callable, SDL_Event *event) {
    PyGILState_STATE gil;
    if(!PySDL_ThreadEnter(&gil)) {
        return 1;  // interpreter gone: keep the event, do nothing
    }

    int keep = 1;
    PyObject *converted = _event(event, 0);
    if(NULL == converted) {
        PyErr_Print();
    } else {
        PyObject *result = PyObject_CallFunctionObjArgs(callable, converted, NULL);
        Py_DECREF(converted);
        if(NULL == result) {
            PyErr_Print();
        } else {
            keep = PyObject_IsTrue(result);
            Py_DECREF(result);
            if(-1 == keep) {
                PyErr_Print();
                keep = 1;
            }
        }
    }

    PySDL_ThreadLeave(gil);
    return keep;
}

static int SDLCALL _c_event_filter(void *userdata, SDL_Event *event) {
    (void)userdata;
    return NULL == _py_event_filter ? 1 : _run_filter(_py_event_filter, event);
}

static int SDLCALL _c_filter_once(void *userdata, SDL_Event *event) {
    return _run_filter((PyObject *)userdata, event);
}

static PyObject * PySDL_SetEventFilter(PyObject *self, PyObject *arg) {
    if(arg == Py_None) {
        SDL_SetEventFilter(NULL, NULL);
        Py_CLEAR(_py_event_filter);
        Py_RETURN_NONE;
    }
    if(!PyCallable_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "expected a callable or None");
        return NULL;
    }
    Py_INCREF(arg);
    Py_XSETREF(_py_event_filter, arg);
    SDL_SetEventFilter(_c_event_filter, NULL);
    Py_RETURN_NONE;
}

static PyObject * PySDL_GetEventFilter(PyObject *self, PyObject *ign) {
    if(NULL == _py_event_filter) {
        Py_RETURN_NONE;
    }
    return Py_NewRef(_py_event_filter);
}

static PyObject * PySDL_FilterEvents(PyObject *self, PyObject *arg) {
    if(!PyCallable_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "expected a callable");
        return NULL;
    }
    SDL_FilterEvents(_c_filter_once, arg);  // synchronous; arg stays alive
    Py_RETURN_NONE;
}

PyMethodDef pysdl_events_methods[] = {
    { "PumpEvents",        PySDL_PumpEvents,        METH_NOARGS  },
    { "PollEvent",         PySDL_PollEvent,         METH_NOARGS  },
    { "WaitEvent",         PySDL_WaitEvent,         METH_NOARGS  },
    { "WaitEventTimeout",  PySDL_WaitEventTimeout,  METH_O       },
    { "PushEvent",         (PyCFunction)PySDL_PushEvent, METH_VARARGS | METH_KEYWORDS },
    { "PeepEvents",        PySDL_PeepEvents,        METH_VARARGS },
    { "FlushEvent",        PySDL_FlushEvent,        METH_O       },
    { "FlushEvents",       PySDL_FlushEvents,       METH_VARARGS },
    { "HasEvent",          PySDL_HasEvent,          METH_O       },
    { "HasEvents",         PySDL_HasEvents,         METH_VARARGS },
    { "RegisterEvents",    PySDL_RegisterEvents,    METH_O       },
    { "EventState",        PySDL_EventState,        METH_VARARGS },
    { "GetEventState",     PySDL_GetEventState,     METH_O       },
    { "QuitRequested",     PySDL_QuitRequested,     METH_NOARGS  },
    { "SetEventFilter",    PySDL_SetEventFilter,    METH_O       },
    { "GetEventFilter",    PySDL_GetEventFilter,    METH_NOARGS  },
    { "FilterEvents",      PySDL_FilterEvents,      METH_O       },
    { NULL }
};
