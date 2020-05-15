#include "pysdl.h"

static int        PySDL_Audio_Type_init    (PySDL_Audio*, PyObject*, PyObject*);
static void       PySDL_Audio_Type_dealloc (PySDL_Audio* );

PyObject * PySDL_Audio_getter (PyObject*, void*);
int        PySDL_Audio_setter (PyObject*, PyObject*, void*);

static PyObject * PySDL_Audio_LockAudioDevice   (PySDL_Audio*, PyObject*);
static PyObject * PySDL_Audio_UnlockAudioDevice (PySDL_Audio*, PyObject*);
static PyObject * PySDL_Audio_PauseAudioDevice  (PySDL_Audio*, PyObject*);
static PyObject * PySDL_Audio_CloseAudioDevice  (PySDL_Audio*, PyObject*);
static PyObject * PySDL_Audio_QueueAudio        (PySDL_Audio*, PyObject*);

static PyMethodDef PySDL_Audio_methods[] = {
    { "LockAudioDevice",   (PyCFunction)PySDL_Audio_LockAudioDevice,   METH_NOARGS },
    { "UnlockAudioDevice", (PyCFunction)PySDL_Audio_UnlockAudioDevice, METH_NOARGS },
    { "PauseAudioDevice",  (PyCFunction)PySDL_Audio_PauseAudioDevice,  METH_O      },
    { "CloseAudioDevice",  (PyCFunction)PySDL_Audio_CloseAudioDevice,  METH_NOARGS },
    { "QueueAudio",        (PyCFunction)PySDL_Audio_QueueAudio,        METH_O      },
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
    //PyObject *tuple = (PyObject *)data;
    //PyObject *callback = PyTuple_GET_ITEM(tuple, 0);
    //PyObject *userdata = PyTuple_GET_ITEM(tuple, 1);
    PyObject *callback = (PyObject *)data;

    printf("--- ptr:%p callable:%d refcount:%ld\n", callback, PyCallable_Check(callback), callback->ob_refcnt);
    //Py_INCREF(callback);
    PyObject *ret = PyObject_CallObject(callback, NULL);
    printf("4\n");

    memset(stream, 0, len);
    printf("5\n");

    if(NULL == ret) {
        PyErr_Print();
        PyErr_Clear();
    }

    Py_XDECREF(ret);
}

static int PySDL_Audio_Type_init(PySDL_Audio *self, PyObject *args, PyObject *kwds) {
    char *deviceName;
    SDL_AudioSpec want;
    SDL_AudioSpec have;
    PyObject *callback = NULL;
    //PyObject *userdata = Py_None;
    int ok;

    self->deviceId = 0;

    SDL_memset(&want, 0, sizeof(want));
    want.freq     = 48000;
    want.format   = AUDIO_S16LSB;
    want.channels = 2;
    want.samples  = 4096;

    //static char *kwlist[] = {"deviceName", "format", "channels", "samples", "callback", "userdata", NULL};
    static char *kwlist[] = {"deviceName", "format", "channels", "samples", "callback", NULL};

    //ok = PyArg_ParseTupleAndKeywords(args, kwds, "s|iiiOO", kwlist,
    ok = PyArg_ParseTupleAndKeywords(args, kwds, "s|iiiO", kwlist,
        &deviceName, &want.freq, &want.format, &want.channels, &callback);
        //&deviceName, &want.freq, &want.format, &want.channels, &callback, &userdata);
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
        // and pass it the Python callback object
        //PyObject *tuple = PyTuple_New(2);
        Py_INCREF(callback);
        //Py_INCREF(callback);
        //Py_INCREF(userdata);
        //PyTuple_SET_ITEM(tuple, 0, callback);
        //PyTuple_SET_ITEM(tuple, 1, userdata);
        want.userdata = callback;
    }

    self->deviceId = SDL_OpenAudioDevice(deviceName, 0, &want, &have, 0);
    if(0 == self->deviceId) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return -1;
    }

    // TODO: getter for audio format?

    return 0;
}

static void PySDL_Audio_Type_dealloc(PySDL_Audio *self) {
    if(0 != self->deviceId) {
        SDL_CloseAudioDevice(self->deviceId);
        self->deviceId = 0;
    }
    Py_TYPE(self)->tp_free((PyObject*)self);
}


static PyObject * PySDL_Audio_LockAudioDevice(PySDL_Audio *self, PyObject *ign) {
    SDL_LockAudioDevice(self->deviceId);
    Py_RETURN_NONE;
}


static PyObject * PySDL_Audio_UnlockAudioDevice(PySDL_Audio *self, PyObject *ign) {
    SDL_UnlockAudioDevice(self->deviceId);
    Py_RETURN_NONE;
}


static PyObject * PySDL_Audio_PauseAudioDevice(PySDL_Audio *self, PyObject *arg) {
    int pause_on = PyLong_AsLong(arg);
    SDL_PauseAudioDevice(self->deviceId, pause_on);
    Py_RETURN_NONE;
}


static PyObject * PySDL_Audio_CloseAudioDevice(PySDL_Audio *self, PyObject *ign) {
    SDL_CloseAudioDevice(self->deviceId);
    self->deviceId = 0;
    Py_RETURN_NONE;
}


static PyObject * PySDL_Audio_QueueAudio(PySDL_Audio *self, PyObject *arg) {
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
