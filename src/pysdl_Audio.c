#include "pysdl.h"

static int        PySDL_Audio_Type_init    (PySDL_Audio*, PyObject*, PyObject*);
static void       PySDL_Audio_Type_dealloc (PySDL_Audio* );

PyObject * PySDL_Audio_getter (PyObject*, void*);
int        PySDL_Audio_setter (PyObject*, PyObject*, void*);

static PyObject * PySDL_Audio_Open   (PySDL_Audio*, PyObject*);
static PyObject * PySDL_Audio_Close  (PySDL_Audio*, PyObject*);
static PyObject * PySDL_Audio_Pause  (PySDL_Audio*, PyObject*);
static PyObject * PySDL_Audio_Lock   (PySDL_Audio*, PyObject*);
static PyObject * PySDL_Audio_Unlock (PySDL_Audio*, PyObject*);
static PyObject * PySDL_Audio_Queue  (PySDL_Audio*, PyObject*);

static PyMethodDef PySDL_Audio_methods[] = {
    { "Open",   (PyCFunction)PySDL_Audio_Open,   METH_VARARGS },
    { "Close",  (PyCFunction)PySDL_Audio_Close,  METH_NOARGS  },
    { "Pause",  (PyCFunction)PySDL_Audio_Pause,  METH_O       },
    { "Lock",   (PyCFunction)PySDL_Audio_Lock,   METH_NOARGS  },
    { "Unlock", (PyCFunction)PySDL_Audio_Unlock, METH_NOARGS  },
    { "Queue",  (PyCFunction)PySDL_Audio_Queue,  METH_O       },
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

static void _audio_callback(void *data, Uint8 *stream, int len) {
    if(_Py_IsFinalizing()) {
        // python is shutting down
        return;
    }

    PyGILState_STATE _audio_thread = PyGILState_Ensure();

    PyObject *callback = PyTuple_GET_ITEM((PyObject *)data, 0);
    PyObject *userdata = PyTuple_GET_ITEM((PyObject *)data, 1);

    PyObject *audioData = PyObject_CallObject(callback, userdata);
    if(audioData) {
        memcpy(stream, PyBytes_AS_STRING(audioData), len);
        Py_DECREF(audioData);
    }
    else {
        PyErr_Print();
        PyErr_Clear();
    }

    PyGILState_Release(_audio_thread);
}

static int PySDL_Audio_Type_init(PySDL_Audio *self, PyObject *args, PyObject *kwds) {
    char *deviceName;
    SDL_AudioSpec want;
    SDL_AudioSpec have;
    PyObject *callback = NULL;
    PyObject *userdata = NULL;

    SDL_memset(&want, 0, sizeof(want));
    want.freq     = 48000;
    want.format   = AUDIO_S16LSB;
    want.channels = 1;
    want.samples  = 2048;

    static char *kwlist[] = {"deviceName", "format", "channels", "samples", "callback", "userdata", NULL};
    int ok = PyArg_ParseTupleAndKeywords(args, kwds, "s|iiiOO", kwlist,
        &deviceName, &want.freq, &want.format, &want.channels, &callback, &userdata);
    if(!ok) {
        return -1;
    }

    if(Py_None == callback) {
        callback = NULL;
    }

    if(NULL != callback) {
        ok = PyCallable_Check(callback);
        if(!ok) {
            PyErr_SetString(PyExc_TypeError, "The callback is not callable");
            return -1;
        }

        // set the C callback
        want.callback = _audio_callback;
        //Py_INCREF(userdata);
        if(NULL == userdata) {
            userdata = PyTuple_New(0);
        }
        want.userdata = Py_BuildValue("(OO)", callback, userdata);
        Py_DECREF(userdata);
    }

    self->deviceId = SDL_OpenAudioDevice(deviceName, 0, &want, &have, 0);
    if(0 == self->deviceId) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return -1;
    }

    // TODO: getter for audio format?
    //printf("%d %d %d %d\n", have.freq, have.format, have.channels, have.samples);

    return 0;
}

static void PySDL_Audio_Type_dealloc(PySDL_Audio *self) {
    if(0 != self->deviceId) {
        SDL_CloseAudioDevice(self->deviceId);
        self->deviceId = 0;
    }
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject * PySDL_Audio_Open(PySDL_Audio *self, PyObject *ign) {
    Py_RETURN_NONE;
}

static PyObject * PySDL_Audio_Close(PySDL_Audio *self, PyObject *ign) {
    SDL_CloseAudioDevice(self->deviceId);
    self->deviceId = 0;
    Py_RETURN_NONE;
}

static PyObject * PySDL_Audio_Lock(PySDL_Audio *self, PyObject *ign) {
    SDL_LockAudioDevice(self->deviceId);
    Py_RETURN_NONE;
}

static PyObject * PySDL_Audio_Unlock(PySDL_Audio *self, PyObject *ign) {
    SDL_UnlockAudioDevice(self->deviceId);
    Py_RETURN_NONE;
}

static PyObject * PySDL_Audio_Pause(PySDL_Audio *self, PyObject *arg) {
    int pause_on = PyLong_AsLong(arg);
    SDL_PauseAudioDevice(self->deviceId, pause_on);
    Py_RETURN_NONE;
}

static PyObject * PySDL_Audio_Queue(PySDL_Audio *self, PyObject *arg) {
    char *buffer;
    Py_ssize_t length;
    int ok = PyBytes_AsStringAndSize(arg, &buffer, &length);
    if(0 > ok) {
        return NULL;
    }
    ok = SDL_QueueAudio(self->deviceId, buffer, length);
    if(0 > ok) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
    }
    Py_RETURN_NONE;
}
