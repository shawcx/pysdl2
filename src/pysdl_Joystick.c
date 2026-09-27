#include "pysdl.h"

static int        PySDL_Joystick_Type_init    (PySDL_Joystick*, PyObject*, PyObject*);
static void       PySDL_Joystick_Type_dealloc (PySDL_Joystick*);

static PyObject * PySDL_Joystick_Name              (PySDL_Joystick*, PyObject*);
static PyObject * PySDL_Joystick_GetGUID           (PySDL_Joystick*, PyObject*);
static PyObject * PySDL_Joystick_InstanceID        (PySDL_Joystick*, PyObject*);
static PyObject * PySDL_Joystick_Attached          (PySDL_Joystick*, PyObject*);
static PyObject * PySDL_Joystick_NumAxes           (PySDL_Joystick*, PyObject*);
static PyObject * PySDL_Joystick_NumButtons        (PySDL_Joystick*, PyObject*);
static PyObject * PySDL_Joystick_NumHats           (PySDL_Joystick*, PyObject*);
static PyObject * PySDL_Joystick_NumBalls          (PySDL_Joystick*, PyObject*);
static PyObject * PySDL_Joystick_GetAxis           (PySDL_Joystick*, PyObject*);
static PyObject * PySDL_Joystick_GetButton         (PySDL_Joystick*, PyObject*);
static PyObject * PySDL_Joystick_GetHat            (PySDL_Joystick*, PyObject*);
static PyObject * PySDL_Joystick_GetBall           (PySDL_Joystick*, PyObject*);
static PyObject * PySDL_Joystick_Rumble            (PySDL_Joystick*, PyObject*);
#if SDL_VERSION_ATLEAST(2,0,14)
static PyObject * PySDL_Joystick_RumbleTriggers    (PySDL_Joystick*, PyObject*);
static PyObject * PySDL_Joystick_SetLED            (PySDL_Joystick*, PyObject*);
static PyObject * PySDL_Joystick_HasLED            (PySDL_Joystick*, PyObject*);
#endif
static PyObject * PySDL_Joystick_CurrentPowerLevel (PySDL_Joystick*, PyObject*);
#if SDL_VERSION_ATLEAST(2,0,14)
static PyObject * PySDL_Joystick_SetVirtualAxis   (PySDL_Joystick*, PyObject*);
static PyObject * PySDL_Joystick_SetVirtualButton (PySDL_Joystick*, PyObject*);
static PyObject * PySDL_Joystick_SetVirtualHat    (PySDL_Joystick*, PyObject*);
#endif
static PyObject * PySDL_Joystick_GetVendor         (PySDL_Joystick*, PyObject*);
static PyObject * PySDL_Joystick_GetProduct        (PySDL_Joystick*, PyObject*);
static PyObject * PySDL_Joystick_GetProductVersion (PySDL_Joystick*, PyObject*);
static PyObject * PySDL_Joystick_GetType           (PySDL_Joystick*, PyObject*);
static PyObject * PySDL_Joystick_GetAxisInitialState (PySDL_Joystick*, PyObject*);
#if SDL_VERSION_ATLEAST(2,0,9)
static PyObject * PySDL_Joystick_GetPlayerIndex    (PySDL_Joystick*, PyObject*);
#endif
#if SDL_VERSION_ATLEAST(2,0,12)
static PyObject * PySDL_Joystick_SetPlayerIndex    (PySDL_Joystick*, PyObject*);
#endif
#if SDL_VERSION_ATLEAST(2,0,14)
static PyObject * PySDL_Joystick_GetSerial         (PySDL_Joystick*, PyObject*);
#endif
#if SDL_VERSION_ATLEAST(2,0,16)
static PyObject * PySDL_Joystick_SendEffect        (PySDL_Joystick*, PyObject*);
#endif
#if SDL_VERSION_ATLEAST(2,0,18)
static PyObject * PySDL_Joystick_HasRumble         (PySDL_Joystick*, PyObject*);
static PyObject * PySDL_Joystick_HasRumbleTriggers (PySDL_Joystick*, PyObject*);
#endif
#if SDL_VERSION_ATLEAST(2,24,0)
static PyObject * PySDL_Joystick_GetFirmwareVersion (PySDL_Joystick*, PyObject*);
static PyObject * PySDL_Joystick_Path              (PySDL_Joystick*, PyObject*);
#endif
static PyObject * PySDL_Joystick_Close             (PySDL_Joystick*, PyObject*);

static PyMethodDef PySDL_Joystick_methods[] = {
    { "Name",              (PyCFunction)PySDL_Joystick_Name,              METH_NOARGS },
    { "GetGUID",           (PyCFunction)PySDL_Joystick_GetGUID,           METH_NOARGS },
    { "InstanceID",        (PyCFunction)PySDL_Joystick_InstanceID,        METH_NOARGS },
    { "Attached",          (PyCFunction)PySDL_Joystick_Attached,          METH_NOARGS },
    { "NumAxes",           (PyCFunction)PySDL_Joystick_NumAxes,           METH_NOARGS },
    { "NumButtons",        (PyCFunction)PySDL_Joystick_NumButtons,        METH_NOARGS },
    { "NumHats",           (PyCFunction)PySDL_Joystick_NumHats,           METH_NOARGS },
    { "NumBalls",          (PyCFunction)PySDL_Joystick_NumBalls,          METH_NOARGS },
    { "GetAxis",           (PyCFunction)PySDL_Joystick_GetAxis,           METH_O      },
    { "GetButton",         (PyCFunction)PySDL_Joystick_GetButton,         METH_O      },
    { "GetHat",            (PyCFunction)PySDL_Joystick_GetHat,            METH_O      },
    { "GetBall",           (PyCFunction)PySDL_Joystick_GetBall,           METH_O      },
    { "Rumble",            (PyCFunction)PySDL_Joystick_Rumble,            METH_VARARGS },
#if SDL_VERSION_ATLEAST(2,0,14)
    { "RumbleTriggers",    (PyCFunction)PySDL_Joystick_RumbleTriggers,    METH_VARARGS },
    { "SetLED",            (PyCFunction)PySDL_Joystick_SetLED,            METH_VARARGS },
    { "HasLED",            (PyCFunction)PySDL_Joystick_HasLED,            METH_NOARGS },
#endif
    { "CurrentPowerLevel", (PyCFunction)PySDL_Joystick_CurrentPowerLevel, METH_NOARGS },
#if SDL_VERSION_ATLEAST(2,0,14)
    { "SetVirtualAxis",    (PyCFunction)PySDL_Joystick_SetVirtualAxis,    METH_VARARGS },
    { "SetVirtualButton",  (PyCFunction)PySDL_Joystick_SetVirtualButton,  METH_VARARGS },
    { "SetVirtualHat",     (PyCFunction)PySDL_Joystick_SetVirtualHat,     METH_VARARGS },
#endif
    { "GetVendor",         (PyCFunction)PySDL_Joystick_GetVendor,         METH_NOARGS },
    { "GetProduct",        (PyCFunction)PySDL_Joystick_GetProduct,        METH_NOARGS },
    { "GetProductVersion", (PyCFunction)PySDL_Joystick_GetProductVersion, METH_NOARGS },
    { "GetType",           (PyCFunction)PySDL_Joystick_GetType,           METH_NOARGS },
    { "GetAxisInitialState", (PyCFunction)PySDL_Joystick_GetAxisInitialState, METH_O },
#if SDL_VERSION_ATLEAST(2,0,9)
    { "GetPlayerIndex",    (PyCFunction)PySDL_Joystick_GetPlayerIndex,    METH_NOARGS },
#endif
#if SDL_VERSION_ATLEAST(2,0,12)
    { "SetPlayerIndex",    (PyCFunction)PySDL_Joystick_SetPlayerIndex,    METH_O      },
#endif
#if SDL_VERSION_ATLEAST(2,0,14)
    { "GetSerial",         (PyCFunction)PySDL_Joystick_GetSerial,         METH_NOARGS },
#endif
#if SDL_VERSION_ATLEAST(2,0,16)
    { "SendEffect",        (PyCFunction)PySDL_Joystick_SendEffect,        METH_O      },
#endif
#if SDL_VERSION_ATLEAST(2,0,18)
    { "HasRumble",         (PyCFunction)PySDL_Joystick_HasRumble,         METH_NOARGS },
    { "HasRumbleTriggers", (PyCFunction)PySDL_Joystick_HasRumbleTriggers, METH_NOARGS },
#endif
#if SDL_VERSION_ATLEAST(2,24,0)
    { "GetFirmwareVersion", (PyCFunction)PySDL_Joystick_GetFirmwareVersion, METH_NOARGS },
    { "Path",              (PyCFunction)PySDL_Joystick_Path,              METH_NOARGS },
#endif
    { "Close",            (PyCFunction)PySDL_Joystick_Close,             METH_NOARGS },
    { NULL }
};

PyTypeObject PySDL_Joystick_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name      = "SDL2.Joystick",
    .tp_basicsize = sizeof(PySDL_Joystick),
    .tp_dealloc   = (destructor)PySDL_Joystick_Type_dealloc,
    .tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc       = "SDL2.Joystick(device_index)",
    .tp_methods   = PySDL_Joystick_methods,
    .tp_init      = (initproc)PySDL_Joystick_Type_init,
    .tp_new       = PyType_GenericNew
};

static int PySDL_Joystick_Type_init(PySDL_Joystick *self, PyObject *args, PyObject *kwds) {
    int index = -1;
    if(!PyArg_ParseTuple(args, "|i", &index)) {
        return -1;
    }

    self->joystick = NULL;

    if(index >= 0) {
        self->joystick = SDL_JoystickOpen(index);
        if(NULL == self->joystick) {
            PyErr_SetString(pysdl_Error, SDL_GetError());
            return -1;
        }
    }
    return 0;
}

static void PySDL_Joystick_Type_dealloc(PySDL_Joystick *self) {
    if(NULL != self->joystick) {
        SDL_JoystickClose(self->joystick);
    }
    self->joystick = NULL;
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static SDL_Joystick * _js(PySDL_Joystick *self) {
    if(NULL == self->joystick) {
        PyErr_SetString(pysdl_Error, "Joystick is not open");
        return NULL;
    }
    return self->joystick;
}

static PyObject * PySDL_Joystick_Name(PySDL_Joystick *self, PyObject *ign) {
    SDL_Joystick *js = _js(self);
    if(NULL == js) return NULL;
    const char *name = SDL_JoystickName(js);
    return PyUnicode_FromString(name ? name : "");
}

static PyObject * PySDL_Joystick_GetGUID(PySDL_Joystick *self, PyObject *ign) {
    SDL_Joystick *js = _js(self);
    if(NULL == js) return NULL;
    return GUIDToPy(SDL_JoystickGetGUID(js));
}

static PyObject * PySDL_Joystick_InstanceID(PySDL_Joystick *self, PyObject *ign) {
    SDL_Joystick *js = _js(self);
    if(NULL == js) return NULL;
    return PyLong_FromLong(SDL_JoystickInstanceID(js));
}

static PyObject * PySDL_Joystick_Attached(PySDL_Joystick *self, PyObject *ign) {
    return PyBool_FromLong(self->joystick && SDL_JoystickGetAttached(self->joystick));
}

static PyObject * PySDL_Joystick_NumAxes(PySDL_Joystick *self, PyObject *ign) {
    SDL_Joystick *js = _js(self);
    return js ? PyLong_FromLong(SDL_JoystickNumAxes(js)) : NULL;
}

static PyObject * PySDL_Joystick_NumButtons(PySDL_Joystick *self, PyObject *ign) {
    SDL_Joystick *js = _js(self);
    return js ? PyLong_FromLong(SDL_JoystickNumButtons(js)) : NULL;
}

static PyObject * PySDL_Joystick_NumHats(PySDL_Joystick *self, PyObject *ign) {
    SDL_Joystick *js = _js(self);
    return js ? PyLong_FromLong(SDL_JoystickNumHats(js)) : NULL;
}

static PyObject * PySDL_Joystick_NumBalls(PySDL_Joystick *self, PyObject *ign) {
    SDL_Joystick *js = _js(self);
    return js ? PyLong_FromLong(SDL_JoystickNumBalls(js)) : NULL;
}

static PyObject * PySDL_Joystick_GetAxis(PySDL_Joystick *self, PyObject *arg) {
    SDL_Joystick *js = _js(self);
    long axis = PyLong_AsLong(arg);
    if(NULL == js || (-1 == axis && PyErr_Occurred())) {
        return NULL;
    }
    return PyLong_FromLong(SDL_JoystickGetAxis(js, (int)axis));
}

static PyObject * PySDL_Joystick_GetButton(PySDL_Joystick *self, PyObject *arg) {
    SDL_Joystick *js = _js(self);
    long button = PyLong_AsLong(arg);
    if(NULL == js || (-1 == button && PyErr_Occurred())) {
        return NULL;
    }
    return PyBool_FromLong(SDL_JoystickGetButton(js, (int)button));
}

static PyObject * PySDL_Joystick_GetHat(PySDL_Joystick *self, PyObject *arg) {
    SDL_Joystick *js = _js(self);
    long hat = PyLong_AsLong(arg);
    if(NULL == js || (-1 == hat && PyErr_Occurred())) {
        return NULL;
    }
    return PyLong_FromLong(SDL_JoystickGetHat(js, (int)hat));
}

static PyObject * PySDL_Joystick_GetBall(PySDL_Joystick *self, PyObject *arg) {
    SDL_Joystick *js = _js(self);
    long ball = PyLong_AsLong(arg);
    if(NULL == js || (-1 == ball && PyErr_Occurred())) {
        return NULL;
    }
    int dx = 0, dy = 0;
    if(0 > SDL_JoystickGetBall(js, (int)ball, &dx, &dy)) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }
    return Py_BuildValue("(ii)", dx, dy);
}

static PyObject * PySDL_Joystick_Rumble(PySDL_Joystick *self, PyObject *args) {
    unsigned int low, high, ms;
    SDL_Joystick *js = _js(self);
    if(NULL == js || !PyArg_ParseTuple(args, "III", &low, &high, &ms)) {
        return NULL;
    }
    return PyBool_FromLong(0 == SDL_JoystickRumble(js, (Uint16)low, (Uint16)high, ms));
}

#if SDL_VERSION_ATLEAST(2,0,14)
static PyObject * PySDL_Joystick_RumbleTriggers(PySDL_Joystick *self, PyObject *args) {
    unsigned int left, right, ms;
    SDL_Joystick *js = _js(self);
    if(NULL == js || !PyArg_ParseTuple(args, "III", &left, &right, &ms)) {
        return NULL;
    }
    return PyBool_FromLong(0 == SDL_JoystickRumbleTriggers(js, (Uint16)left, (Uint16)right, ms));
}

static PyObject * PySDL_Joystick_SetLED(PySDL_Joystick *self, PyObject *args) {
    unsigned char r, g, b;
    SDL_Joystick *js = _js(self);
    if(NULL == js || !PyArg_ParseTuple(args, "bbb", &r, &g, &b)) {
        return NULL;
    }
    return PyBool_FromLong(0 == SDL_JoystickSetLED(js, r, g, b));
}

static PyObject * PySDL_Joystick_HasLED(PySDL_Joystick *self, PyObject *ign) {
    return PyBool_FromLong(self->joystick && SDL_JoystickHasLED(self->joystick));
}
#endif

static PyObject * PySDL_Joystick_CurrentPowerLevel(PySDL_Joystick *self, PyObject *ign) {
    SDL_Joystick *js = _js(self);
    return js ? PyLong_FromLong(SDL_JoystickCurrentPowerLevel(js)) : NULL;
}

#if SDL_VERSION_ATLEAST(2,0,14)
static PyObject * PySDL_Joystick_SetVirtualAxis(PySDL_Joystick *self, PyObject *args) {
    int axis, value;
    SDL_Joystick *js = _js(self);
    if(NULL == js || !PyArg_ParseTuple(args, "ii", &axis, &value)) {
        return NULL;
    }
    if(0 > SDL_JoystickSetVirtualAxis(js, axis, (Sint16)value)) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject * PySDL_Joystick_SetVirtualButton(PySDL_Joystick *self, PyObject *args) {
    int button, value;
    SDL_Joystick *js = _js(self);
    if(NULL == js || !PyArg_ParseTuple(args, "ii", &button, &value)) {
        return NULL;
    }
    if(0 > SDL_JoystickSetVirtualButton(js, button, (Uint8)value)) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject * PySDL_Joystick_SetVirtualHat(PySDL_Joystick *self, PyObject *args) {
    int hat, value;
    SDL_Joystick *js = _js(self);
    if(NULL == js || !PyArg_ParseTuple(args, "ii", &hat, &value)) {
        return NULL;
    }
    if(0 > SDL_JoystickSetVirtualHat(js, hat, (Uint8)value)) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }
    Py_RETURN_NONE;
}
#endif

static PyObject * PySDL_Joystick_GetVendor(PySDL_Joystick *self, PyObject *ign) {
    SDL_Joystick *js = _js(self);
    return js ? PyLong_FromLong(SDL_JoystickGetVendor(js)) : NULL;
}

static PyObject * PySDL_Joystick_GetProduct(PySDL_Joystick *self, PyObject *ign) {
    SDL_Joystick *js = _js(self);
    return js ? PyLong_FromLong(SDL_JoystickGetProduct(js)) : NULL;
}

static PyObject * PySDL_Joystick_GetProductVersion(PySDL_Joystick *self, PyObject *ign) {
    SDL_Joystick *js = _js(self);
    return js ? PyLong_FromLong(SDL_JoystickGetProductVersion(js)) : NULL;
}

static PyObject * PySDL_Joystick_GetType(PySDL_Joystick *self, PyObject *ign) {
    SDL_Joystick *js = _js(self);
    return js ? PyLong_FromLong(SDL_JoystickGetType(js)) : NULL;
}

// -> the axis's initial value, or None if SDL has no initial state for it.
static PyObject * PySDL_Joystick_GetAxisInitialState(PySDL_Joystick *self, PyObject *arg) {
    SDL_Joystick *js = _js(self);
    long axis = PyLong_AsLong(arg);
    if(NULL == js || (-1 == axis && PyErr_Occurred())) {
        return NULL;
    }
    Sint16 state = 0;
    if(!SDL_JoystickGetAxisInitialState(js, (int)axis, &state)) {
        Py_RETURN_NONE;
    }
    return PyLong_FromLong(state);
}

#if SDL_VERSION_ATLEAST(2,0,9)
static PyObject * PySDL_Joystick_GetPlayerIndex(PySDL_Joystick *self, PyObject *ign) {
    SDL_Joystick *js = _js(self);
    return js ? PyLong_FromLong(SDL_JoystickGetPlayerIndex(js)) : NULL;
}
#endif

#if SDL_VERSION_ATLEAST(2,0,12)
static PyObject * PySDL_Joystick_SetPlayerIndex(PySDL_Joystick *self, PyObject *arg) {
    SDL_Joystick *js = _js(self);
    long index = PyLong_AsLong(arg);
    if(NULL == js || (-1 == index && PyErr_Occurred())) {
        return NULL;
    }
    SDL_JoystickSetPlayerIndex(js, (int)index);
    Py_RETURN_NONE;
}
#endif

#if SDL_VERSION_ATLEAST(2,0,14)
static PyObject * PySDL_Joystick_GetSerial(PySDL_Joystick *self, PyObject *ign) {
    SDL_Joystick *js = _js(self);
    if(NULL == js) return NULL;
    const char *serial = SDL_JoystickGetSerial(js);
    if(NULL == serial) {
        Py_RETURN_NONE;
    }
    return PyUnicode_FromString(serial);
}
#endif

#if SDL_VERSION_ATLEAST(2,0,16)
static PyObject * PySDL_Joystick_SendEffect(PySDL_Joystick *self, PyObject *arg) {
    SDL_Joystick *js = _js(self);
    if(NULL == js) return NULL;
    Py_buffer data;
    if(0 > PyObject_GetBuffer(arg, &data, PyBUF_SIMPLE)) {
        return NULL;
    }
    int rc = SDL_JoystickSendEffect(js, data.buf, (int)data.len);
    PyBuffer_Release(&data);
    return PyBool_FromLong(0 == rc);
}
#endif

#if SDL_VERSION_ATLEAST(2,0,18)
static PyObject * PySDL_Joystick_HasRumble(PySDL_Joystick *self, PyObject *ign) {
    return PyBool_FromLong(self->joystick && SDL_JoystickHasRumble(self->joystick));
}

static PyObject * PySDL_Joystick_HasRumbleTriggers(PySDL_Joystick *self, PyObject *ign) {
    return PyBool_FromLong(self->joystick && SDL_JoystickHasRumbleTriggers(self->joystick));
}
#endif

#if SDL_VERSION_ATLEAST(2,24,0)
static PyObject * PySDL_Joystick_GetFirmwareVersion(PySDL_Joystick *self, PyObject *ign) {
    SDL_Joystick *js = _js(self);
    return js ? PyLong_FromLong(SDL_JoystickGetFirmwareVersion(js)) : NULL;
}

static PyObject * PySDL_Joystick_Path(PySDL_Joystick *self, PyObject *ign) {
    SDL_Joystick *js = _js(self);
    if(NULL == js) return NULL;
    const char *path = SDL_JoystickPath(js);
    if(NULL == path) {
        Py_RETURN_NONE;
    }
    return PyUnicode_FromString(path);
}
#endif

static PyObject * PySDL_Joystick_Close(PySDL_Joystick *self, PyObject *ign) {
    if(NULL != self->joystick) {
        SDL_JoystickClose(self->joystick);
    }
    self->joystick = NULL;
    Py_RETURN_NONE;
}

//=========================================================
// GUID / instance helpers (shared with pysdl_GameController.c)
//=========================================================

PyObject * GUIDToPy(SDL_JoystickGUID guid) {
    char buffer[33];
    SDL_JoystickGetGUIDString(guid, buffer, sizeof(buffer));
    return PyUnicode_FromString(buffer);
}

int PyToGUID(PyObject *obj, SDL_JoystickGUID *guid) {
    if(PyUnicode_Check(obj)) {
        Py_ssize_t len;
        const char *text = PyUnicode_AsUTF8AndSize(obj, &len);
        if(NULL == text) {
            return 0;
        }
        if(32 != len) {
            PyErr_SetString(PyExc_ValueError, "GUID string must be 32 hex digits");
            return 0;
        }
        *guid = SDL_JoystickGetGUIDFromString(text);
        return 1;
    }

    Py_buffer raw;
    if(0 > PyObject_GetBuffer(obj, &raw, PyBUF_SIMPLE)) {
        PyErr_SetString(PyExc_TypeError, "GUID must be a 32-digit hex str or 16 bytes");
        return 0;
    }
    if(sizeof(guid->data) != raw.len) {
        PyBuffer_Release(&raw);
        PyErr_SetString(PyExc_ValueError, "GUID bytes must be 16 long");
        return 0;
    }
    SDL_memcpy(guid->data, raw.buf, sizeof(guid->data));
    PyBuffer_Release(&raw);
    return 1;
}

int PySDL_JoystickIndexForInstance(SDL_JoystickID id) {
    int found = -1;
#if SDL_VERSION_ATLEAST(2,0,7)
    SDL_LockJoysticks();
#endif
    int count = SDL_NumJoysticks();
    for(int idx = 0; idx < count; ++idx) {
        if(SDL_JoystickGetDeviceInstanceID(idx) == id) {
            found = idx;
            break;
        }
    }
#if SDL_VERSION_ATLEAST(2,0,7)
    SDL_UnlockJoysticks();
#endif
    return found;
}

// A new, owned wrapper for an already-open joystick. Re-opening by device index
// bumps SDL's refcount, so the wrapper stays valid even if the original owner
// closes. None when nothing with that instance id is open.
static PyObject * _wrap_open_joystick(SDL_Joystick *js) {
    if(NULL == js) {
        Py_RETURN_NONE;
    }
    int index = PySDL_JoystickIndexForInstance(SDL_JoystickInstanceID(js));
    if(0 > index) {
        Py_RETURN_NONE;
    }
    return PyObject_CallFunction((PyObject *)&PySDL_Joystick_Type, "i", index);
}

static int _index_arg(PyObject *arg, int *out) {
    long value = PyLong_AsLong(arg);
    if(-1 == value && PyErr_Occurred()) {
        return 0;
    }
    *out = (int)value;
    return 1;
}

//=========================================================
// module functions
//=========================================================

static PyObject * PySDL_NumJoysticks(PyObject *self, PyObject *ign) {
    return PyLong_FromLong(SDL_NumJoysticks());
}

static PyObject * PySDL_JoystickNameForIndex(PyObject *self, PyObject *arg) {
    long index = PyLong_AsLong(arg);
    if(-1 == index && PyErr_Occurred()) {
        return NULL;
    }
    const char *name = SDL_JoystickNameForIndex((int)index);
    return PyUnicode_FromString(name ? name : "");
}

static PyObject * PySDL_JoystickUpdate(PyObject *self, PyObject *ign) {
    SDL_JoystickUpdate();
    Py_RETURN_NONE;
}

static PyObject * PySDL_JoystickEventState(PyObject *self, PyObject *args) {
    int state = SDL_QUERY;
    if(!PyArg_ParseTuple(args, "|i", &state)) {
        return NULL;
    }
    return PyLong_FromLong(SDL_JoystickEventState(state));
}

static PyObject * PySDL_JoystickGetDeviceGUID(PyObject *self, PyObject *arg) {
    int index;
    return _index_arg(arg, &index) ? GUIDToPy(SDL_JoystickGetDeviceGUID(index)) : NULL;
}

static PyObject * PySDL_JoystickGetDeviceInstanceID(PyObject *self, PyObject *arg) {
    int index;
    return _index_arg(arg, &index) ? PyLong_FromLong(SDL_JoystickGetDeviceInstanceID(index)) : NULL;
}

static PyObject * PySDL_JoystickGetDeviceVendor(PyObject *self, PyObject *arg) {
    int index;
    return _index_arg(arg, &index) ? PyLong_FromLong(SDL_JoystickGetDeviceVendor(index)) : NULL;
}

static PyObject * PySDL_JoystickGetDeviceProduct(PyObject *self, PyObject *arg) {
    int index;
    return _index_arg(arg, &index) ? PyLong_FromLong(SDL_JoystickGetDeviceProduct(index)) : NULL;
}

static PyObject * PySDL_JoystickGetDeviceProductVersion(PyObject *self, PyObject *arg) {
    int index;
    return _index_arg(arg, &index) ? PyLong_FromLong(SDL_JoystickGetDeviceProductVersion(index)) : NULL;
}

static PyObject * PySDL_JoystickGetDeviceType(PyObject *self, PyObject *arg) {
    int index;
    return _index_arg(arg, &index) ? PyLong_FromLong(SDL_JoystickGetDeviceType(index)) : NULL;
}

#if SDL_VERSION_ATLEAST(2,0,9)
static PyObject * PySDL_JoystickGetDevicePlayerIndex(PyObject *self, PyObject *arg) {
    int index;
    return _index_arg(arg, &index) ? PyLong_FromLong(SDL_JoystickGetDevicePlayerIndex(index)) : NULL;
}
#endif

#if SDL_VERSION_ATLEAST(2,24,0)
static PyObject * PySDL_JoystickPathForIndex(PyObject *self, PyObject *arg) {
    int index;
    if(!_index_arg(arg, &index)) {
        return NULL;
    }
    const char *path = SDL_JoystickPathForIndex(index);
    if(NULL == path) {
        Py_RETURN_NONE;
    }
    return PyUnicode_FromString(path);
}
#endif

static PyObject * PySDL_JoystickFromInstanceID(PyObject *self, PyObject *arg) {
    int id;
    return _index_arg(arg, &id) ? _wrap_open_joystick(SDL_JoystickFromInstanceID(id)) : NULL;
}

#if SDL_VERSION_ATLEAST(2,0,12)
static PyObject * PySDL_JoystickFromPlayerIndex(PyObject *self, PyObject *arg) {
    int player;
    return _index_arg(arg, &player) ? _wrap_open_joystick(SDL_JoystickFromPlayerIndex(player)) : NULL;
}
#endif

#if SDL_VERSION_ATLEAST(2,0,7)
// Don't hold this across a call that can run a virtual-joystick callback on
// another thread: that callback needs the GIL.
static PyObject * PySDL_LockJoysticks(PyObject *self, PyObject *ign) {
    SDL_LockJoysticks();
    Py_RETURN_NONE;
}

static PyObject * PySDL_UnlockJoysticks(PyObject *self, PyObject *ign) {
    SDL_UnlockJoysticks();
    Py_RETURN_NONE;
}
#endif

// GUIDs are 32-char hex strings everywhere else in the binding; these convert
// to and from the raw 16 bytes.
static PyObject * PySDL_JoystickGetGUIDFromString(PyObject *self, PyObject *arg) {
    SDL_JoystickGUID guid;
    if(!PyToGUID(arg, &guid)) {
        return NULL;
    }
    return PyBytes_FromStringAndSize((const char *)guid.data, sizeof(guid.data));
}

static PyObject * PySDL_JoystickGetGUIDString(PyObject *self, PyObject *arg) {
    SDL_JoystickGUID guid;
    return PyToGUID(arg, &guid) ? GUIDToPy(guid) : NULL;
}

#if SDL_VERSION_ATLEAST(2,24,0)
static PyObject * PySDL_GUIDFromString(PyObject *self, PyObject *arg) {
    const char *text = PyUnicode_AsUTF8(arg);
    if(NULL == text) {
        return NULL;
    }
    SDL_GUID guid = SDL_GUIDFromString(text);
    return PyBytes_FromStringAndSize((const char *)guid.data, sizeof(guid.data));
}

static PyObject * PySDL_GUIDToString(PyObject *self, PyObject *arg) {
    SDL_GUID guid;
    if(!PyToGUID(arg, &guid)) {
        return NULL;
    }
    char buffer[33];
    SDL_GUIDToString(guid, buffer, sizeof(buffer));
    return PyUnicode_FromString(buffer);
}
#endif

#if SDL_VERSION_ATLEAST(2,26,0)
// -> (vendor, product, version, crc16)
static PyObject * PySDL_GetJoystickGUIDInfo(PyObject *self, PyObject *arg) {
    SDL_JoystickGUID guid;
    if(!PyToGUID(arg, &guid)) {
        return NULL;
    }
    Uint16 vendor, product, version, crc16;
    SDL_GetJoystickGUIDInfo(guid, &vendor, &product, &version, &crc16);
    return Py_BuildValue("(iiii)", vendor, product, version, crc16);
}
#endif

#if SDL_VERSION_ATLEAST(2,0,14)
static PyObject * PySDL_JoystickAttachVirtual(PyObject *self, PyObject *args) {
    int type, naxes, nbuttons, nhats;
    if(!PyArg_ParseTuple(args, "iiii", &type, &naxes, &nbuttons, &nhats)) {
        return NULL;
    }
    int index = SDL_JoystickAttachVirtual((SDL_JoystickType)type, naxes, nbuttons, nhats);
    if(0 > index) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }
    return PyLong_FromLong(index);
}
#endif

#if SDL_VERSION_ATLEAST(2,24,0)
//=========================================================
// virtual joystick callbacks (JoystickAttachVirtualEx)
//=========================================================

// userdata is a tuple of VJ_COUNT callables-or-None, held in _virtual_callbacks
// (instance id -> tuple) until JoystickDetachVirtual. SDL only gets a C
// pointer for the slots that are callable, so a None slot reads as unsupported
// (e.g. HasRumble() is False without a rumble callback).
enum { VJ_UPDATE, VJ_PLAYER, VJ_RUMBLE, VJ_TRIGGERS, VJ_LED, VJ_EFFECT, VJ_COUNT };

static PyObject *_virtual_callbacks = NULL;

// Call slot with Py_BuildValue(fmt, ...) args. Returns 0 when the callable
// returns None or a true value, -1 on a false value or an exception.
static int _vj_invoke(void *userdata, int slot, const char *fmt, ...) {
    PyGILState_STATE gil;
    if(!PySDL_ThreadEnter(&gil)) {
        return -1;
    }

    int rc = -1;
    va_list va;
    va_start(va, fmt);
    PyObject *args = Py_VaBuildValue(fmt, va);
    va_end(va);

    if(NULL != args) {
        PyObject *fn = PyTuple_GET_ITEM((PyObject *)userdata, slot);
        PyObject *result = PyObject_CallObject(fn, args);
        Py_DECREF(args);
        if(NULL != result) {
            if(result == Py_None) {
                rc = 0;
            } else {
                int truth = PyObject_IsTrue(result);
                rc = (1 == truth) ? 0 : -1;
            }
            Py_DECREF(result);
        }
    }
    if(PyErr_Occurred()) {
        PyErr_Print();
    }

    PySDL_ThreadLeave(gil);
    return rc;
}

static void SDLCALL _vj_update(void *ud) {
    _vj_invoke(ud, VJ_UPDATE, "()");
}

static void SDLCALL _vj_player(void *ud, int player) {
    _vj_invoke(ud, VJ_PLAYER, "(i)", player);
}

static int SDLCALL _vj_rumble(void *ud, Uint16 low, Uint16 high) {
    return _vj_invoke(ud, VJ_RUMBLE, "(ii)", (int)low, (int)high);
}

static int SDLCALL _vj_triggers(void *ud, Uint16 left, Uint16 right) {
    return _vj_invoke(ud, VJ_TRIGGERS, "(ii)", (int)left, (int)right);
}

static int SDLCALL _vj_led(void *ud, Uint8 r, Uint8 g, Uint8 b) {
    return _vj_invoke(ud, VJ_LED, "(iii)", (int)r, (int)g, (int)b);
}

static int SDLCALL _vj_effect(void *ud, const void *data, int size) {
    return _vj_invoke(ud, VJ_EFFECT, "(y#)", (const char *)data, (Py_ssize_t)size);
}

static PyObject * PySDL_JoystickAttachVirtualEx(PyObject *self, PyObject *args, PyObject *kwds) {
    int type = SDL_JOYSTICK_TYPE_GAMECONTROLLER;
    int naxes = 0, nbuttons = 0, nhats = 0;
    unsigned short vendor_id = 0, product_id = 0;
    unsigned int button_mask = 0, axis_mask = 0;
    const char *name = NULL;
    PyObject *fns[VJ_COUNT] = { Py_None, Py_None, Py_None, Py_None, Py_None, Py_None };

    static char *kwlist[] = {
        "type", "naxes", "nbuttons", "nhats",
        "vendor_id", "product_id", "button_mask", "axis_mask", "name",
        "update", "set_player_index", "rumble", "rumble_triggers", "set_led", "send_effect",
        NULL
    };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "|iiii$HHIIzOOOOOO", kwlist,
        &type, &naxes, &nbuttons, &nhats,
        &vendor_id, &product_id, &button_mask, &axis_mask, &name,
        &fns[VJ_UPDATE], &fns[VJ_PLAYER], &fns[VJ_RUMBLE],
        &fns[VJ_TRIGGERS], &fns[VJ_LED], &fns[VJ_EFFECT])) {
        return NULL;
    }

    int any = 0;
    for(int idx = 0; idx < VJ_COUNT; ++idx) {
        if(fns[idx] != Py_None) {
            if(!PyCallable_Check(fns[idx])) {
                PyErr_Format(PyExc_TypeError, "%s must be callable or None", kwlist[9 + idx]);
                return NULL;
            }
            any = 1;
        }
    }

    SDL_VirtualJoystickDesc desc;
    SDL_zero(desc);
    desc.version     = SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
    desc.type        = (Uint16)type;
    desc.naxes       = (Uint16)naxes;
    desc.nbuttons    = (Uint16)nbuttons;
    desc.nhats       = (Uint16)nhats;
    desc.vendor_id   = vendor_id;
    desc.product_id  = product_id;
    desc.button_mask = button_mask;
    desc.axis_mask   = axis_mask;
    desc.name        = name;

    PyObject *userdata = NULL;
    if(any) {
        userdata = PyTuple_Pack(VJ_COUNT, fns[0], fns[1], fns[2], fns[3], fns[4], fns[5]);
        if(NULL == userdata) {
            return NULL;
        }
        if(NULL == _virtual_callbacks && NULL == (_virtual_callbacks = PyDict_New())) {
            Py_DECREF(userdata);
            return NULL;
        }
        desc.userdata = userdata;
        if(fns[VJ_UPDATE]   != Py_None) desc.Update         = _vj_update;
        if(fns[VJ_PLAYER]   != Py_None) desc.SetPlayerIndex = _vj_player;
        if(fns[VJ_RUMBLE]   != Py_None) desc.Rumble         = _vj_rumble;
        if(fns[VJ_TRIGGERS] != Py_None) desc.RumbleTriggers = _vj_triggers;
        if(fns[VJ_LED]      != Py_None) desc.SetLED         = _vj_led;
        if(fns[VJ_EFFECT]   != Py_None) desc.SendEffect     = _vj_effect;
    }

    int index = SDL_JoystickAttachVirtualEx(&desc);
    if(0 > index) {
        Py_XDECREF(userdata);
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }

    if(NULL != userdata) {
        PyObject *key = PyLong_FromLong(SDL_JoystickGetDeviceInstanceID(index));
        int rc = key ? PyDict_SetItem(_virtual_callbacks, key, userdata) : -1;
        Py_XDECREF(key);
        Py_DECREF(userdata);  // the dict holds it now
        if(0 > rc) {
            SDL_JoystickDetachVirtual(index);
            return NULL;
        }
    }
    return PyLong_FromLong(index);
}
#endif

#if SDL_VERSION_ATLEAST(2,0,14)
static PyObject * PySDL_JoystickDetachVirtual(PyObject *self, PyObject *arg) {
    long index = PyLong_AsLong(arg);
    if(-1 == index && PyErr_Occurred()) {
        return NULL;
    }
    SDL_JoystickID id = SDL_JoystickGetDeviceInstanceID((int)index);
    if(0 > SDL_JoystickDetachVirtual((int)index)) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }
#if SDL_VERSION_ATLEAST(2,24,0)
    // SDL no longer calls back into this device: drop its callbacks.
    if(NULL != _virtual_callbacks) {
        PyObject *key = PyLong_FromLong(id);
        if(NULL == key) {
            return NULL;
        }
        if(0 > PyDict_DelItem(_virtual_callbacks, key)) {
            if(!PyErr_ExceptionMatches(PyExc_KeyError)) {
                Py_DECREF(key);
                return NULL;
            }
            PyErr_Clear();  // attached without callbacks
        }
        Py_DECREF(key);
    }
#else
    (void)id;
#endif
    Py_RETURN_NONE;
}

static PyObject * PySDL_JoystickIsVirtual(PyObject *self, PyObject *arg) {
    long index = PyLong_AsLong(arg);
    if(-1 == index && PyErr_Occurred()) {
        return NULL;
    }
    return PyBool_FromLong(SDL_JoystickIsVirtual((int)index));
}
#endif

PyMethodDef pysdl_joystick_methods[] = {
    { "NumJoysticks",           PySDL_NumJoysticks,           METH_NOARGS  },
    { "JoystickNameForIndex",   PySDL_JoystickNameForIndex,   METH_O       },
    { "JoystickUpdate",         PySDL_JoystickUpdate,         METH_NOARGS  },
    { "JoystickEventState",     PySDL_JoystickEventState,     METH_VARARGS },
    { "JoystickGetDeviceGUID",           PySDL_JoystickGetDeviceGUID,           METH_O },
    { "JoystickGetDeviceInstanceID",     PySDL_JoystickGetDeviceInstanceID,     METH_O },
    { "JoystickGetDeviceVendor",         PySDL_JoystickGetDeviceVendor,         METH_O },
    { "JoystickGetDeviceProduct",        PySDL_JoystickGetDeviceProduct,        METH_O },
    { "JoystickGetDeviceProductVersion", PySDL_JoystickGetDeviceProductVersion, METH_O },
    { "JoystickGetDeviceType",           PySDL_JoystickGetDeviceType,           METH_O },
    { "JoystickFromInstanceID",          PySDL_JoystickFromInstanceID,          METH_O },
    { "JoystickGetGUIDFromString",       PySDL_JoystickGetGUIDFromString,       METH_O },
    { "JoystickGetGUIDString",           PySDL_JoystickGetGUIDString,           METH_O },
#if SDL_VERSION_ATLEAST(2,0,7)
    { "LockJoysticks",          PySDL_LockJoysticks,          METH_NOARGS  },
    { "UnlockJoysticks",        PySDL_UnlockJoysticks,        METH_NOARGS  },
#endif
#if SDL_VERSION_ATLEAST(2,0,9)
    { "JoystickGetDevicePlayerIndex",    PySDL_JoystickGetDevicePlayerIndex,    METH_O },
#endif
#if SDL_VERSION_ATLEAST(2,0,12)
    { "JoystickFromPlayerIndex",         PySDL_JoystickFromPlayerIndex,         METH_O },
#endif
#if SDL_VERSION_ATLEAST(2,0,14)
    { "JoystickAttachVirtual",  PySDL_JoystickAttachVirtual,  METH_VARARGS },
    { "JoystickDetachVirtual",  PySDL_JoystickDetachVirtual,  METH_O       },
    { "JoystickIsVirtual",      PySDL_JoystickIsVirtual,      METH_O       },
#endif
#if SDL_VERSION_ATLEAST(2,24,0)
    { "JoystickAttachVirtualEx", (PyCFunction)PySDL_JoystickAttachVirtualEx, METH_VARARGS | METH_KEYWORDS },
    { "JoystickPathForIndex",   PySDL_JoystickPathForIndex,   METH_O       },
    { "GUIDFromString",         PySDL_GUIDFromString,         METH_O       },
    { "GUIDToString",           PySDL_GUIDToString,           METH_O       },
#endif
#if SDL_VERSION_ATLEAST(2,26,0)
    { "GetJoystickGUIDInfo",    PySDL_GetJoystickGUIDInfo,    METH_O       },
#endif
    { NULL }
};
