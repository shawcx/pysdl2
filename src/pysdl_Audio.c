#include "pysdl.h"

static int        PySDL_Audio_Type_init    (PySDL_Audio*, PyObject*, PyObject*);
static void       PySDL_Audio_Type_dealloc (PySDL_Audio* );

PyObject * PySDL_Audio_getter (PyObject*, void*);
int        PySDL_Audio_setter (PyObject*, PyObject*, void*);

static PyObject * PySDL_Audio_Open         (PySDL_Audio*, PyObject*, PyObject*);
static PyObject * PySDL_Audio_Close        (PySDL_Audio*, PyObject*);
static PyObject * PySDL_Audio_Pause        (PySDL_Audio*, PyObject*);
static PyObject * PySDL_Audio_Lock         (PySDL_Audio*, PyObject*);
static PyObject * PySDL_Audio_Unlock       (PySDL_Audio*, PyObject*);
static PyObject * PySDL_Audio_Queue        (PySDL_Audio*, PyObject*);
static PyObject * PySDL_Audio_Dequeue      (PySDL_Audio*, PyObject*);
static PyObject * PySDL_Audio_GetQueueSize (PySDL_Audio*, PyObject*);

static PyMethodDef PySDL_Audio_methods[] = {
    { "Open",         (PyCFunction)PySDL_Audio_Open,         METH_VARARGS | METH_KEYWORDS },
    { "Close",        (PyCFunction)PySDL_Audio_Close,        METH_NOARGS  },
    { "Pause",        (PyCFunction)PySDL_Audio_Pause,        METH_O       },
    { "Lock",         (PyCFunction)PySDL_Audio_Lock,         METH_NOARGS  },
    { "Unlock",       (PyCFunction)PySDL_Audio_Unlock,       METH_NOARGS  },
    { "Queue",        (PyCFunction)PySDL_Audio_Queue,        METH_O       },
    { "Dequeue",      (PyCFunction)PySDL_Audio_Dequeue,      METH_O       },
    { "GetQueueSize", (PyCFunction)PySDL_Audio_GetQueueSize, METH_NOARGS  },
    { NULL }
};

PyTypeObject PySDL_Audio_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name      = "SDL2.Audio",
    .tp_basicsize = sizeof(PySDL_Audio),
    .tp_dealloc   = (destructor)PySDL_Audio_Type_dealloc,
    .tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc       = "SDL2.Audio Class",
    .tp_methods   = PySDL_Audio_methods,
    .tp_init      = (initproc)PySDL_Audio_Type_init,
    .tp_new       = PyType_GenericNew
};

static void _playback_callback(void *data, Uint8 *stream, int len) {
    PyGILState_STATE gil;

    // Silence is the fallback whenever the Python callback does not deliver a
    // full buffer (error, wrong type, short bytes).
    SDL_memset(stream, 0, len);

    if(!PySDL_ThreadEnter(&gil)) {
        return;
    }

    PyObject *callback = PyTuple_GET_ITEM((PyObject *)data, 0);
    PyObject *userdata = PyTuple_GET_ITEM((PyObject *)data, 1);
    PyObject *size     = PyLong_FromLong(len);

    PyObject *audioData = PyObject_CallFunctionObjArgs(callback, size, userdata, NULL);
    if(NULL == audioData) {
        PyErr_Print();
    } else if(!PyBytes_Check(audioData)) {
        PyErr_Format(PyExc_TypeError, "audio callback must return bytes, not %s", Py_TYPE(audioData)->tp_name);
        PyErr_Print();
        Py_DECREF(audioData);
    } else {
        Py_ssize_t have = PyBytes_GET_SIZE(audioData);
        memcpy(stream, PyBytes_AS_STRING(audioData), have < len ? (size_t)have : (size_t)len);
        Py_DECREF(audioData);
    }

    Py_XDECREF(size);

    PySDL_ThreadLeave(gil);
}

static void _capture_callback(void *data, Uint8 *stream, int len) {
    PyGILState_STATE gil;

    if(!PySDL_ThreadEnter(&gil)) {
        return;
    }

    PyObject *callback = PyTuple_GET_ITEM((PyObject *)data, 0);
    PyObject *userdata = PyTuple_GET_ITEM((PyObject *)data, 1);

    PyObject *audioData = PyBytes_FromStringAndSize((char *)stream, len);
    if(NULL != audioData) {
        PyObject *retval = PyObject_CallFunctionObjArgs(callback, audioData, userdata, NULL);
        if(NULL == retval) {
            PyErr_Print();
        } else {
            Py_DECREF(retval);
        }
        Py_DECREF(audioData);
    } else {
        PyErr_Print();
    }

    PySDL_ThreadLeave(gil);
}

static int PySDL_Audio_Type_init(PySDL_Audio *self, PyObject *args, PyObject *kwds) {
    self->deviceId = 0;
    self->pycallback = NULL;
    return 0;
}

static void PySDL_Audio_Type_dealloc(PySDL_Audio *self) {
    if(0 != self->deviceId) {
        Py_BEGIN_ALLOW_THREADS
            SDL_CloseAudioDevice(self->deviceId);
        Py_END_ALLOW_THREADS
        self->deviceId = 0;
    }
    Py_XDECREF(self->pycallback);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject * PySDL_Audio_Open(PySDL_Audio *self, PyObject *args, PyObject *kwds) {
    char *deviceName = NULL;
    int capture = 0;
    SDL_AudioSpec want;
    SDL_AudioSpec have;
    int flags = SDL_AUDIO_ALLOW_ANY_CHANGE;
    PyObject *callback = NULL;
    PyObject *userdata = NULL;

    if(0 != self->deviceId) {
        PyErr_SetString(pysdl_Error, "Audio device already open");
        return NULL;
    }

    SDL_memset(&want, 0, sizeof(want));
    SDL_memset(&have, 0, sizeof(have));

    //want.freq     = 48000;
    //want.format   = AUDIO_S16LSB;
    //want.channels = 2;
    //want.samples  = 4096;

    static char *kwlist[] = {"deviceName", "capture", "freq", "format", "channels", "samples", "flags", "callback", "userdata", NULL};
    int ok = PyArg_ParseTupleAndKeywords(args, kwds, "s|iiiiiiOO", kwlist,
        &deviceName, &capture, &want.freq, &want.format, &want.channels, &want.samples, &flags, &callback, &userdata);
    if(!ok) {
        return NULL;
    }

    PyObject *cbtuple = NULL;

    if(NULL != callback) {
        if(!PyCallable_Check(callback)) {
            PyErr_SetString(PyExc_TypeError, "The callback is not a callable object");
            return NULL;
        }

        // set the C callback; the tuple owns the only references SDL's audio
        // thread holds and is released in Close/dealloc.
        want.callback = capture ? _capture_callback : _playback_callback;
        cbtuple = Py_BuildValue("(OO)", callback, userdata ? userdata : Py_None);
        if(NULL == cbtuple) {
            return NULL;
        }
        want.userdata = cbtuple;
    }

    self->deviceId = SDL_OpenAudioDevice(deviceName, capture, &want, &have, flags);
    if(0 == self->deviceId) {
        Py_XDECREF(cbtuple);
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }

    self->pycallback = cbtuple;

    return Py_BuildValue("(iiii)", have.freq, have.format, have.channels, have.samples);
}

static PyObject * PySDL_Audio_Close(PySDL_Audio *self, PyObject *ign) {
    if(0 != self->deviceId) {
        // Drop the GIL: SDL_CloseAudioDevice joins the callback thread, which
        // is itself trying to re-acquire the GIL.
        Py_BEGIN_ALLOW_THREADS
            SDL_CloseAudioDevice(self->deviceId);
        Py_END_ALLOW_THREADS
        self->deviceId = 0;
    }
    Py_CLEAR(self->pycallback);
    Py_RETURN_NONE;
}

static PyObject * PySDL_Audio_Lock(PySDL_Audio *self, PyObject *ign) {
    Py_BEGIN_ALLOW_THREADS
        SDL_LockAudioDevice(self->deviceId);
    Py_END_ALLOW_THREADS
    Py_RETURN_NONE;
}

static PyObject * PySDL_Audio_Unlock(PySDL_Audio *self, PyObject *ign) {
    SDL_UnlockAudioDevice(self->deviceId);
    Py_RETURN_NONE;
}

static PyObject * PySDL_Audio_Pause(PySDL_Audio *self, PyObject *arg) {
    int pause = PyObject_IsTrue(arg);
    if(-1 == pause) {
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
        SDL_PauseAudioDevice(self->deviceId, pause);
    Py_END_ALLOW_THREADS
    Py_RETURN_NONE;
}

static PyObject * PySDL_Audio_Queue(PySDL_Audio *self, PyObject *arg) {
    Uint8 *buffer;
    Py_ssize_t length;

    if(0 > PyBytes_AsStringAndSize(arg, (char **)&buffer, &length)) {
        return NULL;
    }

    if(0 > SDL_QueueAudio(self->deviceId, buffer, length)) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
    }

    Py_RETURN_NONE;
}

static PyObject * PySDL_Audio_Dequeue(PySDL_Audio *self, PyObject *arg) {
    PyObject *buffer = NULL;
    Py_ssize_t length = 0;
    Uint32 returned;

    length = PyLong_AsLong(arg);
    if(-1 == length && PyErr_Occurred()) {
        return NULL;
    }

    buffer = PyBytes_FromStringAndSize(NULL, length);
    returned = SDL_DequeueAudio(self->deviceId, PyBytes_AsString(buffer), length);
    if(0 > returned) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }
    return buffer;
}

static PyObject * PySDL_Audio_GetQueueSize(PySDL_Audio *self, PyObject *ign) {
    return PyLong_FromLong(SDL_GetQueuedAudioSize(self->deviceId));
}

