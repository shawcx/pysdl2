#include "pysdl.h"

static int        PySDL_GameController_Type_init    (PySDL_GameController*, PyObject*, PyObject*);
static void       PySDL_GameController_Type_dealloc (PySDL_GameController*);

static PyObject * PySDL_GameController_Name           (PySDL_GameController*, PyObject*);
static PyObject * PySDL_GameController_Attached       (PySDL_GameController*, PyObject*);
static PyObject * PySDL_GameController_GetAxis        (PySDL_GameController*, PyObject*);
static PyObject * PySDL_GameController_GetButton      (PySDL_GameController*, PyObject*);
static PyObject * PySDL_GameController_Mapping        (PySDL_GameController*, PyObject*);
static PyObject * PySDL_GameController_GetJoystick    (PySDL_GameController*, PyObject*);
static PyObject * PySDL_GameController_GetBindForAxis   (PySDL_GameController*, PyObject*);
static PyObject * PySDL_GameController_GetBindForButton (PySDL_GameController*, PyObject*);
static PyObject * PySDL_GameController_Rumble         (PySDL_GameController*, PyObject*);
static PyObject * PySDL_GameController_GetVendor         (PySDL_GameController*, PyObject*);
static PyObject * PySDL_GameController_GetProduct        (PySDL_GameController*, PyObject*);
static PyObject * PySDL_GameController_GetProductVersion (PySDL_GameController*, PyObject*);
#if SDL_VERSION_ATLEAST(2,0,9)
static PyObject * PySDL_GameController_GetPlayerIndex (PySDL_GameController*, PyObject*);
#endif
#if SDL_VERSION_ATLEAST(2,0,12)
static PyObject * PySDL_GameController_SetPlayerIndex (PySDL_GameController*, PyObject*);
static PyObject * PySDL_GameController_GetType        (PySDL_GameController*, PyObject*);
#endif
#if SDL_VERSION_ATLEAST(2,0,14)
static PyObject * PySDL_GameController_RumbleTriggers (PySDL_GameController*, PyObject*);
static PyObject * PySDL_GameController_SetLED         (PySDL_GameController*, PyObject*);
static PyObject * PySDL_GameController_HasLED         (PySDL_GameController*, PyObject*);
static PyObject * PySDL_GameController_HasAxis        (PySDL_GameController*, PyObject*);
static PyObject * PySDL_GameController_HasButton      (PySDL_GameController*, PyObject*);
static PyObject * PySDL_GameController_GetSerial      (PySDL_GameController*, PyObject*);
static PyObject * PySDL_GameController_HasSensor        (PySDL_GameController*, PyObject*);
static PyObject * PySDL_GameController_SetSensorEnabled (PySDL_GameController*, PyObject*);
static PyObject * PySDL_GameController_IsSensorEnabled  (PySDL_GameController*, PyObject*);
static PyObject * PySDL_GameController_GetSensorData    (PySDL_GameController*, PyObject*, PyObject*);
static PyObject * PySDL_GameController_GetNumTouchpads       (PySDL_GameController*, PyObject*);
static PyObject * PySDL_GameController_GetNumTouchpadFingers (PySDL_GameController*, PyObject*);
static PyObject * PySDL_GameController_GetTouchpadFinger     (PySDL_GameController*, PyObject*);
#endif
#if SDL_VERSION_ATLEAST(2,0,16)
static PyObject * PySDL_GameController_GetSensorDataRate (PySDL_GameController*, PyObject*);
static PyObject * PySDL_GameController_SendEffect     (PySDL_GameController*, PyObject*);
#endif
#if SDL_VERSION_ATLEAST(2,0,18)
static PyObject * PySDL_GameController_HasRumble         (PySDL_GameController*, PyObject*);
static PyObject * PySDL_GameController_HasRumbleTriggers (PySDL_GameController*, PyObject*);
static PyObject * PySDL_GameController_GetAppleSFSymbolsNameForButton (PySDL_GameController*, PyObject*);
static PyObject * PySDL_GameController_GetAppleSFSymbolsNameForAxis   (PySDL_GameController*, PyObject*);
#endif
#if SDL_VERSION_ATLEAST(2,24,0)
static PyObject * PySDL_GameController_GetFirmwareVersion (PySDL_GameController*, PyObject*);
static PyObject * PySDL_GameController_Path           (PySDL_GameController*, PyObject*);
#endif
#if SDL_VERSION_ATLEAST(2,26,0)
static PyObject * PySDL_GameController_GetSensorDataWithTimestamp (PySDL_GameController*, PyObject*, PyObject*);
#endif
#if SDL_VERSION_ATLEAST(2,30,0)
static PyObject * PySDL_GameController_GetSteamHandle (PySDL_GameController*, PyObject*);
#endif
static PyObject * PySDL_GameController_Close          (PySDL_GameController*, PyObject*);

static PyMethodDef PySDL_GameController_methods[] = {
    { "Name",           (PyCFunction)PySDL_GameController_Name,           METH_NOARGS },
    { "Attached",       (PyCFunction)PySDL_GameController_Attached,       METH_NOARGS },
    { "GetAxis",        (PyCFunction)PySDL_GameController_GetAxis,        METH_O      },
    { "GetButton",      (PyCFunction)PySDL_GameController_GetButton,      METH_O      },
    { "Mapping",        (PyCFunction)PySDL_GameController_Mapping,        METH_NOARGS },
    { "GetJoystick",    (PyCFunction)PySDL_GameController_GetJoystick,    METH_NOARGS },
    { "GetBindForAxis",   (PyCFunction)PySDL_GameController_GetBindForAxis,   METH_O },
    { "GetBindForButton", (PyCFunction)PySDL_GameController_GetBindForButton, METH_O },
    { "Rumble",         (PyCFunction)PySDL_GameController_Rumble,         METH_VARARGS },
    { "GetVendor",         (PyCFunction)PySDL_GameController_GetVendor,         METH_NOARGS },
    { "GetProduct",        (PyCFunction)PySDL_GameController_GetProduct,        METH_NOARGS },
    { "GetProductVersion", (PyCFunction)PySDL_GameController_GetProductVersion, METH_NOARGS },
#if SDL_VERSION_ATLEAST(2,0,9)
    { "GetPlayerIndex", (PyCFunction)PySDL_GameController_GetPlayerIndex, METH_NOARGS },
#endif
#if SDL_VERSION_ATLEAST(2,0,12)
    { "SetPlayerIndex", (PyCFunction)PySDL_GameController_SetPlayerIndex, METH_O      },
    { "GetType",        (PyCFunction)PySDL_GameController_GetType,        METH_NOARGS },
#endif
#if SDL_VERSION_ATLEAST(2,0,14)
    { "RumbleTriggers", (PyCFunction)PySDL_GameController_RumbleTriggers, METH_VARARGS },
    { "SetLED",         (PyCFunction)PySDL_GameController_SetLED,         METH_VARARGS },
    { "HasLED",         (PyCFunction)PySDL_GameController_HasLED,         METH_NOARGS },
    { "HasAxis",        (PyCFunction)PySDL_GameController_HasAxis,        METH_O      },
    { "HasButton",      (PyCFunction)PySDL_GameController_HasButton,      METH_O      },
    { "GetSerial",      (PyCFunction)PySDL_GameController_GetSerial,      METH_NOARGS },
    { "HasSensor",        (PyCFunction)PySDL_GameController_HasSensor,        METH_O       },
    { "SetSensorEnabled", (PyCFunction)PySDL_GameController_SetSensorEnabled, METH_VARARGS },
    { "IsSensorEnabled",  (PyCFunction)PySDL_GameController_IsSensorEnabled,  METH_O       },
    { "GetSensorData",    (PyCFunction)PySDL_GameController_GetSensorData,    METH_VARARGS | METH_KEYWORDS },
    { "GetNumTouchpads",       (PyCFunction)PySDL_GameController_GetNumTouchpads,       METH_NOARGS  },
    { "GetNumTouchpadFingers", (PyCFunction)PySDL_GameController_GetNumTouchpadFingers, METH_O       },
    { "GetTouchpadFinger",     (PyCFunction)PySDL_GameController_GetTouchpadFinger,     METH_VARARGS },
#endif
#if SDL_VERSION_ATLEAST(2,0,16)
    { "GetSensorDataRate", (PyCFunction)PySDL_GameController_GetSensorDataRate, METH_O },
    { "SendEffect",     (PyCFunction)PySDL_GameController_SendEffect,     METH_O      },
#endif
#if SDL_VERSION_ATLEAST(2,0,18)
    { "HasRumble",         (PyCFunction)PySDL_GameController_HasRumble,         METH_NOARGS },
    { "HasRumbleTriggers", (PyCFunction)PySDL_GameController_HasRumbleTriggers, METH_NOARGS },
    { "GetAppleSFSymbolsNameForButton", (PyCFunction)PySDL_GameController_GetAppleSFSymbolsNameForButton, METH_O },
    { "GetAppleSFSymbolsNameForAxis",   (PyCFunction)PySDL_GameController_GetAppleSFSymbolsNameForAxis,   METH_O },
#endif
#if SDL_VERSION_ATLEAST(2,24,0)
    { "GetFirmwareVersion", (PyCFunction)PySDL_GameController_GetFirmwareVersion, METH_NOARGS },
    { "Path",           (PyCFunction)PySDL_GameController_Path,           METH_NOARGS },
#endif
#if SDL_VERSION_ATLEAST(2,26,0)
    { "GetSensorDataWithTimestamp", (PyCFunction)PySDL_GameController_GetSensorDataWithTimestamp, METH_VARARGS | METH_KEYWORDS },
#endif
#if SDL_VERSION_ATLEAST(2,30,0)
    { "GetSteamHandle", (PyCFunction)PySDL_GameController_GetSteamHandle, METH_NOARGS },
#endif
    { "Close",          (PyCFunction)PySDL_GameController_Close,          METH_NOARGS },
    { NULL }
};

PyTypeObject PySDL_GameController_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name      = "SDL2.GameController",
    .tp_basicsize = sizeof(PySDL_GameController),
    .tp_dealloc   = (destructor)PySDL_GameController_Type_dealloc,
    .tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc       = "SDL2.GameController(device_index)",
    .tp_methods   = PySDL_GameController_methods,
    .tp_init      = (initproc)PySDL_GameController_Type_init,
    .tp_new       = PyType_GenericNew
};

static int PySDL_GameController_Type_init(PySDL_GameController *self, PyObject *args, PyObject *kwds) {
    int index = -1;
    if(!PyArg_ParseTuple(args, "|i", &index)) {
        return -1;
    }

    self->controller = NULL;

    if(index >= 0) {
        self->controller = SDL_GameControllerOpen(index);
        if(NULL == self->controller) {
            PyErr_SetString(pysdl_Error, SDL_GetError());
            return -1;
        }
    }
    return 0;
}

static void PySDL_GameController_Type_dealloc(PySDL_GameController *self) {
    if(NULL != self->controller) {
        SDL_GameControllerClose(self->controller);
        self->controller = NULL;
    }
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static SDL_GameController * _gc(PySDL_GameController *self) {
    if(NULL == self->controller) {
        PyErr_SetString(pysdl_Error, "GameController is not open");
        return NULL;
    }
    return self->controller;
}

// Parse a METH_O int argument alongside the open-controller guard.
static SDL_GameController * _gc_int(PySDL_GameController *self, PyObject *arg, int *out) {
    SDL_GameController *gc = _gc(self);
    long value = PyLong_AsLong(arg);
    if(NULL == gc || (-1 == value && PyErr_Occurred())) {
        return NULL;
    }
    *out = (int)value;
    return gc;
}

// A const char* SDL may return as NULL -> str or None.
static PyObject * _str_or_none(const char *text) {
    if(NULL == text) {
        Py_RETURN_NONE;
    }
    return PyUnicode_FromString(text);
}

// An SDL_malloc'd mapping string -> str (freed) or None.
static PyObject * _take_mapping(char *mapping) {
    if(NULL == mapping) {
        Py_RETURN_NONE;
    }
    PyObject *result = PyUnicode_FromString(mapping);
    SDL_free(mapping);
    return result;
}

static PyObject * PySDL_GameController_Name(PySDL_GameController *self, PyObject *ign) {
    SDL_GameController *gc = _gc(self);
    if(NULL == gc) return NULL;
    const char *name = SDL_GameControllerName(gc);
    return PyUnicode_FromString(name ? name : "");
}

static PyObject * PySDL_GameController_Attached(PySDL_GameController *self, PyObject *ign) {
    return PyBool_FromLong(self->controller && SDL_GameControllerGetAttached(self->controller));
}

static PyObject * PySDL_GameController_GetAxis(PySDL_GameController *self, PyObject *arg) {
    SDL_GameController *gc = _gc(self);
    long axis = PyLong_AsLong(arg);
    if(NULL == gc || (-1 == axis && PyErr_Occurred())) {
        return NULL;
    }
    return PyLong_FromLong(SDL_GameControllerGetAxis(gc, (SDL_GameControllerAxis)axis));
}

static PyObject * PySDL_GameController_GetButton(PySDL_GameController *self, PyObject *arg) {
    SDL_GameController *gc = _gc(self);
    long button = PyLong_AsLong(arg);
    if(NULL == gc || (-1 == button && PyErr_Occurred())) {
        return NULL;
    }
    return PyBool_FromLong(SDL_GameControllerGetButton(gc, (SDL_GameControllerButton)button));
}

static PyObject * PySDL_GameController_Mapping(PySDL_GameController *self, PyObject *ign) {
    SDL_GameController *gc = _gc(self);
    if(NULL == gc) return NULL;
    return _take_mapping(SDL_GameControllerMapping(gc));
}

// The controller's joystick as an owned Joystick: re-opening it by device index
// bumps SDL's refcount, so it stays valid after the controller is closed (a
// borrowed pointer would dangle). None if the device is no longer attached.
static PyObject * PySDL_GameController_GetJoystick(PySDL_GameController *self, PyObject *ign) {
    SDL_GameController *gc = _gc(self);
    if(NULL == gc) return NULL;

    SDL_Joystick *joystick = SDL_GameControllerGetJoystick(gc);
    int index = joystick ? PySDL_JoystickIndexForInstance(SDL_JoystickInstanceID(joystick)) : -1;
    if(0 > index) {
        SDL_ClearError();
        Py_RETURN_NONE;
    }
    return PyObject_CallFunction((PyObject *)&PySDL_Joystick_Type, "i", index);
}

// -> None (unbound), (BINDTYPE_BUTTON, button), (BINDTYPE_AXIS, axis) or
//    (BINDTYPE_HAT, hat, hat_mask)
static PyObject * _bind_to_py(SDL_GameControllerButtonBind bind) {
    switch(bind.bindType) {
    case SDL_CONTROLLER_BINDTYPE_BUTTON:
        return Py_BuildValue("(ii)", bind.bindType, bind.value.button);
    case SDL_CONTROLLER_BINDTYPE_AXIS:
        return Py_BuildValue("(ii)", bind.bindType, bind.value.axis);
    case SDL_CONTROLLER_BINDTYPE_HAT:
        return Py_BuildValue("(iii)", bind.bindType, bind.value.hat.hat, bind.value.hat.hat_mask);
    default:
        Py_RETURN_NONE;
    }
}

static PyObject * PySDL_GameController_GetBindForAxis(PySDL_GameController *self, PyObject *arg) {
    int axis;
    SDL_GameController *gc = _gc_int(self, arg, &axis);
    return gc ? _bind_to_py(SDL_GameControllerGetBindForAxis(gc, (SDL_GameControllerAxis)axis)) : NULL;
}

static PyObject * PySDL_GameController_GetBindForButton(PySDL_GameController *self, PyObject *arg) {
    int button;
    SDL_GameController *gc = _gc_int(self, arg, &button);
    return gc ? _bind_to_py(SDL_GameControllerGetBindForButton(gc, (SDL_GameControllerButton)button)) : NULL;
}

static PyObject * PySDL_GameController_Rumble(PySDL_GameController *self, PyObject *args) {
    unsigned int low, high, ms;
    SDL_GameController *gc = _gc(self);
    if(NULL == gc || !PyArg_ParseTuple(args, "III", &low, &high, &ms)) {
        return NULL;
    }
    return PyBool_FromLong(0 == SDL_GameControllerRumble(gc, (Uint16)low, (Uint16)high, ms));
}

static PyObject * PySDL_GameController_GetVendor(PySDL_GameController *self, PyObject *ign) {
    SDL_GameController *gc = _gc(self);
    return gc ? PyLong_FromLong(SDL_GameControllerGetVendor(gc)) : NULL;
}

static PyObject * PySDL_GameController_GetProduct(PySDL_GameController *self, PyObject *ign) {
    SDL_GameController *gc = _gc(self);
    return gc ? PyLong_FromLong(SDL_GameControllerGetProduct(gc)) : NULL;
}

static PyObject * PySDL_GameController_GetProductVersion(PySDL_GameController *self, PyObject *ign) {
    SDL_GameController *gc = _gc(self);
    return gc ? PyLong_FromLong(SDL_GameControllerGetProductVersion(gc)) : NULL;
}

#if SDL_VERSION_ATLEAST(2,0,9)
static PyObject * PySDL_GameController_GetPlayerIndex(PySDL_GameController *self, PyObject *ign) {
    SDL_GameController *gc = _gc(self);
    return gc ? PyLong_FromLong(SDL_GameControllerGetPlayerIndex(gc)) : NULL;
}
#endif

#if SDL_VERSION_ATLEAST(2,0,12)
static PyObject * PySDL_GameController_SetPlayerIndex(PySDL_GameController *self, PyObject *arg) {
    int player;
    SDL_GameController *gc = _gc_int(self, arg, &player);
    if(NULL == gc) return NULL;
    SDL_GameControllerSetPlayerIndex(gc, player);
    Py_RETURN_NONE;
}

static PyObject * PySDL_GameController_GetType(PySDL_GameController *self, PyObject *ign) {
    SDL_GameController *gc = _gc(self);
    return gc ? PyLong_FromLong(SDL_GameControllerGetType(gc)) : NULL;
}
#endif

#if SDL_VERSION_ATLEAST(2,0,14)
static PyObject * PySDL_GameController_RumbleTriggers(PySDL_GameController *self, PyObject *args) {
    unsigned int left, right, ms;
    SDL_GameController *gc = _gc(self);
    if(NULL == gc || !PyArg_ParseTuple(args, "III", &left, &right, &ms)) {
        return NULL;
    }
    return PyBool_FromLong(0 == SDL_GameControllerRumbleTriggers(gc, (Uint16)left, (Uint16)right, ms));
}

static PyObject * PySDL_GameController_SetLED(PySDL_GameController *self, PyObject *args) {
    unsigned char r, g, b;
    SDL_GameController *gc = _gc(self);
    if(NULL == gc || !PyArg_ParseTuple(args, "bbb", &r, &g, &b)) {
        return NULL;
    }
    return PyBool_FromLong(0 == SDL_GameControllerSetLED(gc, r, g, b));
}

static PyObject * PySDL_GameController_HasLED(PySDL_GameController *self, PyObject *ign) {
    return PyBool_FromLong(self->controller && SDL_GameControllerHasLED(self->controller));
}

static PyObject * PySDL_GameController_HasAxis(PySDL_GameController *self, PyObject *arg) {
    int axis;
    SDL_GameController *gc = _gc_int(self, arg, &axis);
    return gc ? PyBool_FromLong(SDL_GameControllerHasAxis(gc, (SDL_GameControllerAxis)axis)) : NULL;
}

static PyObject * PySDL_GameController_HasButton(PySDL_GameController *self, PyObject *arg) {
    int button;
    SDL_GameController *gc = _gc_int(self, arg, &button);
    return gc ? PyBool_FromLong(SDL_GameControllerHasButton(gc, (SDL_GameControllerButton)button)) : NULL;
}

static PyObject * PySDL_GameController_GetSerial(PySDL_GameController *self, PyObject *ign) {
    SDL_GameController *gc = _gc(self);
    return gc ? _str_or_none(SDL_GameControllerGetSerial(gc)) : NULL;
}

static PyObject * PySDL_GameController_HasSensor(PySDL_GameController *self, PyObject *arg) {
    int type;
    SDL_GameController *gc = _gc_int(self, arg, &type);
    return gc ? PyBool_FromLong(SDL_GameControllerHasSensor(gc, (SDL_SensorType)type)) : NULL;
}

static PyObject * PySDL_GameController_SetSensorEnabled(PySDL_GameController *self, PyObject *args) {
    int type, enabled;
    SDL_GameController *gc = _gc(self);
    if(NULL == gc || !PyArg_ParseTuple(args, "ip", &type, &enabled)) {
        return NULL;
    }
    if(0 > SDL_GameControllerSetSensorEnabled(gc, (SDL_SensorType)type, enabled ? SDL_TRUE : SDL_FALSE)) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject * PySDL_GameController_IsSensorEnabled(PySDL_GameController *self, PyObject *arg) {
    int type;
    SDL_GameController *gc = _gc_int(self, arg, &type);
    return gc ? PyBool_FromLong(SDL_GameControllerIsSensorEnabled(gc, (SDL_SensorType)type)) : NULL;
}

static PyObject * _floats_to_tuple(const float *values, int count) {
    PyObject *tuple = PyTuple_New(count);
    if(NULL == tuple) {
        return NULL;
    }
    for(int idx = 0; idx < count; ++idx) {
        PyObject *f = PyFloat_FromDouble(values[idx]);
        if(NULL == f) {
            Py_DECREF(tuple);
            return NULL;
        }
        PyTuple_SET_ITEM(tuple, idx, f);
    }
    return tuple;
}

static int _sensor_args(PyObject *args, PyObject *kwds, int *type, int *count) {
    static char *kwlist[] = {"type", "count", NULL};
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "i|i", kwlist, type, count)) {
        return 0;
    }
    if(*count < 1 || *count > 16) {
        PyErr_SetString(PyExc_ValueError, "count must be 1..16");
        return 0;
    }
    return 1;
}

// GetSensorData(type, count=3) -> tuple of `count` floats
static PyObject * PySDL_GameController_GetSensorData(PySDL_GameController *self, PyObject *args, PyObject *kwds) {
    int type, count = 3;
    SDL_GameController *gc = _gc(self);
    if(NULL == gc || !_sensor_args(args, kwds, &type, &count)) {
        return NULL;
    }
    float values[16];
    if(0 > SDL_GameControllerGetSensorData(gc, (SDL_SensorType)type, values, count)) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }
    return _floats_to_tuple(values, count);
}

static PyObject * PySDL_GameController_GetNumTouchpads(PySDL_GameController *self, PyObject *ign) {
    SDL_GameController *gc = _gc(self);
    return gc ? PyLong_FromLong(SDL_GameControllerGetNumTouchpads(gc)) : NULL;
}

static PyObject * PySDL_GameController_GetNumTouchpadFingers(PySDL_GameController *self, PyObject *arg) {
    int touchpad;
    SDL_GameController *gc = _gc_int(self, arg, &touchpad);
    if(NULL == gc) return NULL;
    int count = SDL_GameControllerGetNumTouchpadFingers(gc, touchpad);
    if(0 > count) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }
    return PyLong_FromLong(count);
}

// GetTouchpadFinger(touchpad, finger) -> (state, x, y, pressure)
static PyObject * PySDL_GameController_GetTouchpadFinger(PySDL_GameController *self, PyObject *args) {
    int touchpad, finger;
    SDL_GameController *gc = _gc(self);
    if(NULL == gc || !PyArg_ParseTuple(args, "ii", &touchpad, &finger)) {
        return NULL;
    }
    Uint8 state = 0;
    float x = 0, y = 0, pressure = 0;
    if(0 > SDL_GameControllerGetTouchpadFinger(gc, touchpad, finger, &state, &x, &y, &pressure)) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }
    return Py_BuildValue("(ifff)", state, x, y, pressure);
}
#endif

#if SDL_VERSION_ATLEAST(2,0,16)
static PyObject * PySDL_GameController_GetSensorDataRate(PySDL_GameController *self, PyObject *arg) {
    int type;
    SDL_GameController *gc = _gc_int(self, arg, &type);
    return gc ? PyFloat_FromDouble(SDL_GameControllerGetSensorDataRate(gc, (SDL_SensorType)type)) : NULL;
}

static PyObject * PySDL_GameController_SendEffect(PySDL_GameController *self, PyObject *arg) {
    SDL_GameController *gc = _gc(self);
    if(NULL == gc) return NULL;
    Py_buffer data;
    if(0 > PyObject_GetBuffer(arg, &data, PyBUF_SIMPLE)) {
        return NULL;
    }
    int rc = SDL_GameControllerSendEffect(gc, data.buf, (int)data.len);
    PyBuffer_Release(&data);
    return PyBool_FromLong(0 == rc);
}
#endif

#if SDL_VERSION_ATLEAST(2,0,18)
static PyObject * PySDL_GameController_HasRumble(PySDL_GameController *self, PyObject *ign) {
    return PyBool_FromLong(self->controller && SDL_GameControllerHasRumble(self->controller));
}

static PyObject * PySDL_GameController_HasRumbleTriggers(PySDL_GameController *self, PyObject *ign) {
    return PyBool_FromLong(self->controller && SDL_GameControllerHasRumbleTriggers(self->controller));
}

// Apple platforms only; None elsewhere.
static PyObject * PySDL_GameController_GetAppleSFSymbolsNameForButton(PySDL_GameController *self, PyObject *arg) {
    int button;
    SDL_GameController *gc = _gc_int(self, arg, &button);
    return gc ? _str_or_none(SDL_GameControllerGetAppleSFSymbolsNameForButton(gc, (SDL_GameControllerButton)button)) : NULL;
}

static PyObject * PySDL_GameController_GetAppleSFSymbolsNameForAxis(PySDL_GameController *self, PyObject *arg) {
    int axis;
    SDL_GameController *gc = _gc_int(self, arg, &axis);
    return gc ? _str_or_none(SDL_GameControllerGetAppleSFSymbolsNameForAxis(gc, (SDL_GameControllerAxis)axis)) : NULL;
}
#endif

#if SDL_VERSION_ATLEAST(2,24,0)
static PyObject * PySDL_GameController_GetFirmwareVersion(PySDL_GameController *self, PyObject *ign) {
    SDL_GameController *gc = _gc(self);
    return gc ? PyLong_FromLong(SDL_GameControllerGetFirmwareVersion(gc)) : NULL;
}

static PyObject * PySDL_GameController_Path(PySDL_GameController *self, PyObject *ign) {
    SDL_GameController *gc = _gc(self);
    return gc ? _str_or_none(SDL_GameControllerPath(gc)) : NULL;
}
#endif

#if SDL_VERSION_ATLEAST(2,26,0)
// GetSensorDataWithTimestamp(type, count=3) -> (timestamp_us, (floats...))
static PyObject * PySDL_GameController_GetSensorDataWithTimestamp(PySDL_GameController *self, PyObject *args, PyObject *kwds) {
    int type, count = 3;
    SDL_GameController *gc = _gc(self);
    if(NULL == gc || !_sensor_args(args, kwds, &type, &count)) {
        return NULL;
    }
    float values[16];
    Uint64 timestamp = 0;
    if(0 > SDL_GameControllerGetSensorDataWithTimestamp(gc, (SDL_SensorType)type, &timestamp, values, count)) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }
    PyObject *data = _floats_to_tuple(values, count);
    if(NULL == data) {
        return NULL;
    }
    return Py_BuildValue("(KN)", (unsigned long long)timestamp, data);
}
#endif

#if SDL_VERSION_ATLEAST(2,30,0)
// The Steam Input handle (an InputHandle_t), or 0 when not driven by Steam.
static PyObject * PySDL_GameController_GetSteamHandle(PySDL_GameController *self, PyObject *ign) {
    SDL_GameController *gc = _gc(self);
    return gc ? PyLong_FromUnsignedLongLong(SDL_GameControllerGetSteamHandle(gc)) : NULL;
}
#endif

static PyObject * PySDL_GameController_Close(PySDL_GameController *self, PyObject *ign) {
    if(NULL != self->controller) {
        SDL_GameControllerClose(self->controller);
        self->controller = NULL;
    }
    Py_RETURN_NONE;
}

//=========================================================
// module functions
//=========================================================

static int _int_arg(PyObject *arg, int *out) {
    long value = PyLong_AsLong(arg);
    if(-1 == value && PyErr_Occurred()) {
        return 0;
    }
    *out = (int)value;
    return 1;
}

// A new, owned wrapper for an already-open controller (re-opening by device
// index bumps SDL's refcount), or None when nothing with that id is open.
static PyObject * _wrap_open_controller(SDL_GameController *gc) {
    if(NULL == gc) {
        Py_RETURN_NONE;
    }
    SDL_JoystickID id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(gc));
    int index = PySDL_JoystickIndexForInstance(id);
    if(0 > index) {
        Py_RETURN_NONE;
    }
    return PyObject_CallFunction((PyObject *)&PySDL_GameController_Type, "i", index);
}

static PyObject * PySDL_IsGameController(PyObject *self, PyObject *arg) {
    long index = PyLong_AsLong(arg);
    if(-1 == index && PyErr_Occurred()) {
        return NULL;
    }
    return PyBool_FromLong(SDL_IsGameController((int)index));
}

static PyObject * PySDL_GameControllerNameForIndex(PyObject *self, PyObject *arg) {
    long index = PyLong_AsLong(arg);
    if(-1 == index && PyErr_Occurred()) {
        return NULL;
    }
    const char *name = SDL_GameControllerNameForIndex((int)index);
    return PyUnicode_FromString(name ? name : "");
}

static PyObject * PySDL_GameControllerAddMapping(PyObject *self, PyObject *arg) {
    const char *mapping = PyUnicode_AsUTF8(arg);
    if(NULL == mapping) {
        return NULL;
    }
    int rc = SDL_GameControllerAddMapping(mapping);
    if(-1 == rc) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }
    return PyLong_FromLong(rc);  // 1 added, 0 updated
}

// Takes a path (str) or the file's contents (bytes) -> number of mappings added.
static PyObject * PySDL_GameControllerAddMappingsFromFile(PyObject *self, PyObject *arg) {
    int rc;
    if(PyBytes_Check(arg) || PyByteArray_Check(arg)) {
        Py_buffer buffer;
        if(0 > PyObject_GetBuffer(arg, &buffer, PyBUF_SIMPLE)) {
            return NULL;
        }
        SDL_RWops *rw = SDL_RWFromConstMem(buffer.buf, (int)buffer.len);
        rc = rw ? SDL_GameControllerAddMappingsFromRW(rw, 1) : -1;
        PyBuffer_Release(&buffer);
    } else {
        const char *path = PyUnicode_AsUTF8(arg);
        if(NULL == path) {
            return NULL;
        }
        rc = SDL_GameControllerAddMappingsFromFile(path);
    }
    if(-1 == rc) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }
    return PyLong_FromLong(rc);
}

static PyObject * PySDL_GameControllerUpdate(PyObject *self, PyObject *ign) {
    SDL_GameControllerUpdate();
    Py_RETURN_NONE;
}

static PyObject * PySDL_GameControllerEventState(PyObject *self, PyObject *args) {
    int state = SDL_QUERY;
    if(!PyArg_ParseTuple(args, "|i", &state)) {
        return NULL;
    }
    return PyLong_FromLong(SDL_GameControllerEventState(state));
}

static PyObject * PySDL_GameControllerFromInstanceID(PyObject *self, PyObject *arg) {
    int id;
    return _int_arg(arg, &id) ? _wrap_open_controller(SDL_GameControllerFromInstanceID(id)) : NULL;
}

static PyObject * PySDL_GameControllerMappingForGUID(PyObject *self, PyObject *arg) {
    SDL_JoystickGUID guid;
    return PyToGUID(arg, &guid) ? _take_mapping(SDL_GameControllerMappingForGUID(guid)) : NULL;
}

static PyObject * PySDL_GameControllerNumMappings(PyObject *self, PyObject *ign) {
    return PyLong_FromLong(SDL_GameControllerNumMappings());
}

static PyObject * PySDL_GameControllerMappingForIndex(PyObject *self, PyObject *arg) {
    int index;
    return _int_arg(arg, &index) ? _take_mapping(SDL_GameControllerMappingForIndex(index)) : NULL;
}

static PyObject * PySDL_GameControllerGetAxisFromString(PyObject *self, PyObject *arg) {
    const char *name = PyUnicode_AsUTF8(arg);
    return name ? PyLong_FromLong(SDL_GameControllerGetAxisFromString(name)) : NULL;
}

static PyObject * PySDL_GameControllerGetButtonFromString(PyObject *self, PyObject *arg) {
    const char *name = PyUnicode_AsUTF8(arg);
    return name ? PyLong_FromLong(SDL_GameControllerGetButtonFromString(name)) : NULL;
}

static PyObject * PySDL_GameControllerGetStringForAxis(PyObject *self, PyObject *arg) {
    int axis;
    return _int_arg(arg, &axis) ? _str_or_none(SDL_GameControllerGetStringForAxis((SDL_GameControllerAxis)axis)) : NULL;
}

static PyObject * PySDL_GameControllerGetStringForButton(PyObject *self, PyObject *arg) {
    int button;
    return _int_arg(arg, &button) ? _str_or_none(SDL_GameControllerGetStringForButton((SDL_GameControllerButton)button)) : NULL;
}

#if SDL_VERSION_ATLEAST(2,0,9)
static PyObject * PySDL_GameControllerMappingForDeviceIndex(PyObject *self, PyObject *arg) {
    int index;
    return _int_arg(arg, &index) ? _take_mapping(SDL_GameControllerMappingForDeviceIndex(index)) : NULL;
}
#endif

#if SDL_VERSION_ATLEAST(2,0,12)
static PyObject * PySDL_GameControllerFromPlayerIndex(PyObject *self, PyObject *arg) {
    int player;
    return _int_arg(arg, &player) ? _wrap_open_controller(SDL_GameControllerFromPlayerIndex(player)) : NULL;
}

static PyObject * PySDL_GameControllerTypeForIndex(PyObject *self, PyObject *arg) {
    int index;
    return _int_arg(arg, &index) ? PyLong_FromLong(SDL_GameControllerTypeForIndex(index)) : NULL;
}
#endif

#if SDL_VERSION_ATLEAST(2,24,0)
static PyObject * PySDL_GameControllerPathForIndex(PyObject *self, PyObject *arg) {
    int index;
    return _int_arg(arg, &index) ? _str_or_none(SDL_GameControllerPathForIndex(index)) : NULL;
}
#endif

PyMethodDef pysdl_gamecontroller_methods[] = {
    { "IsGameController",                 PySDL_IsGameController,                 METH_O       },
    { "GameControllerNameForIndex",       PySDL_GameControllerNameForIndex,       METH_O       },
    { "GameControllerAddMapping",         PySDL_GameControllerAddMapping,         METH_O       },
    { "GameControllerAddMappingsFromFile", PySDL_GameControllerAddMappingsFromFile, METH_O     },
    { "GameControllerUpdate",             PySDL_GameControllerUpdate,             METH_NOARGS  },
    { "GameControllerEventState",         PySDL_GameControllerEventState,         METH_VARARGS },
    { "GameControllerFromInstanceID",     PySDL_GameControllerFromInstanceID,     METH_O       },
    { "GameControllerMappingForGUID",     PySDL_GameControllerMappingForGUID,     METH_O       },
    { "GameControllerNumMappings",        PySDL_GameControllerNumMappings,        METH_NOARGS  },
    { "GameControllerMappingForIndex",    PySDL_GameControllerMappingForIndex,    METH_O       },
    { "GameControllerGetAxisFromString",  PySDL_GameControllerGetAxisFromString,  METH_O       },
    { "GameControllerGetButtonFromString", PySDL_GameControllerGetButtonFromString, METH_O     },
    { "GameControllerGetStringForAxis",   PySDL_GameControllerGetStringForAxis,   METH_O       },
    { "GameControllerGetStringForButton", PySDL_GameControllerGetStringForButton, METH_O       },
#if SDL_VERSION_ATLEAST(2,0,9)
    { "GameControllerMappingForDeviceIndex", PySDL_GameControllerMappingForDeviceIndex, METH_O },
#endif
#if SDL_VERSION_ATLEAST(2,0,12)
    { "GameControllerFromPlayerIndex",    PySDL_GameControllerFromPlayerIndex,    METH_O       },
    { "GameControllerTypeForIndex",       PySDL_GameControllerTypeForIndex,       METH_O       },
#endif
#if SDL_VERSION_ATLEAST(2,24,0)
    { "GameControllerPathForIndex",       PySDL_GameControllerPathForIndex,       METH_O       },
#endif
    { NULL }
};
