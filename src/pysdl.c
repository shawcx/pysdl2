#include "pysdl.h"

PyObject *pysdl_Error;

static PyObject * PySDL_Init                  (PyObject*, PyObject*);
static PyObject * PySDL_WasInit               (PyObject*, PyObject*);
static PyObject * PySDL_Quit                  (PyObject*, PyObject*);
static PyObject * PySDL_Version               (PyObject*, PyObject*);
static PyObject * PySDL_GetPlatform           (PyObject*, PyObject*);
static PyObject * PySDL_GetCurrentVideoDriver (PyObject*, PyObject*);
static PyObject * PySDL_GetVideoDrivers       (PyObject*, PyObject*);
static PyObject * PySDL_GetTicks              (PyObject*, PyObject*);
static PyObject * PySDL_LoadBMP               (PyObject*, PyObject*);
static PyObject * PySDL_LoadImage             (PyObject*, PyObject*);
static PyObject * PySDL_ShowCursor            (PyObject*, PyObject*);

static PyObject * PySDL_CreateRGBSurface      (PyObject*, PyObject*, PyObject *);
static PyObject * PySDL_CreateRGBSurfaceFrom  (PyObject*, PyObject*, PyObject *);

static PyObject * PySDL_PollEvent        (PyObject*, PyObject*);
static PyObject * PySDL_WaitEvent        (PyObject*, PyObject*);
static PyObject * PySDL_GetKeyboardState (PyObject*, PyObject*);
static PyObject * PySDL_GetModState      (PyObject*, PyObject*);

static PyObject * PySDL_GetCPUCount         (PyObject*, PyObject*);
static PyObject * PySDL_GetCPUCacheLineSize (PyObject*, PyObject*);
static PyObject * PySDL_Has3DNow            (PyObject*, PyObject*);
static PyObject * PySDL_HasAVX              (PyObject*, PyObject*);
static PyObject * PySDL_HasAVX2             (PyObject*, PyObject*);
static PyObject * PySDL_HasAltiVec          (PyObject*, PyObject*);
static PyObject * PySDL_HasMMX              (PyObject*, PyObject*);
static PyObject * PySDL_HasSSE              (PyObject*, PyObject*);
static PyObject * PySDL_HasSSE2             (PyObject*, PyObject*);
static PyObject * PySDL_HasSSE3             (PyObject*, PyObject*);
static PyObject * PySDL_HasSSE41            (PyObject*, PyObject*);
static PyObject * PySDL_HasSSE42            (PyObject*, PyObject*);

static PyObject * PySDL_GetNumVideoDisplays (PyObject*, PyObject*);
static PyObject * PySDL_GetDisplayMode      (PyObject*, PyObject*);
static PyObject * PySDL_GetDisplayBounds    (PyObject*, PyObject*);

static PyObject * PySDL_GL_SetAttribute       (PyObject*, PyObject*);
static PyObject * PySDL_GL_GetAttribute       (PyObject*, PyObject*);
static PyObject * PySDL_GL_ResetAttributes    (PyObject*, PyObject*);
static PyObject * PySDL_GL_ExtensionSupported (PyObject*, PyObject*);
static PyObject * PySDL_GL_SetSwapInterval    (PyObject*, PyObject*);
static PyObject * PySDL_GL_GetSwapInterval    (PyObject*, PyObject*);

static PyObject * PySDL_GetNumAudioDevices    (PyObject*, PyObject*);
static PyObject * PySDL_GetAudioDeviceName    (PyObject*, PyObject*);


static PyMethodDef pysdl_PyMethodDefs[] = {
    { "Init",                  PySDL_Init,                  METH_VARARGS },
    { "WasInit",               PySDL_WasInit,               METH_VARARGS },
    { "Quit",                  PySDL_Quit,                  METH_NOARGS  },
    { "Version",               PySDL_Version,               METH_NOARGS  },
    { "GetPlatform",           PySDL_GetPlatform,           METH_NOARGS  },
    { "GetCurrentVideoDriver", PySDL_GetCurrentVideoDriver, METH_NOARGS  },
    { "GetVideoDrivers",       PySDL_GetVideoDrivers,       METH_NOARGS  },
    { "GetTicks",              PySDL_GetTicks,              METH_NOARGS  },
    { "LoadBMP",               PySDL_LoadBMP,               METH_O       },
    { "LoadImage",             PySDL_LoadImage,             METH_O       },
    { "ShowCursor",            PySDL_ShowCursor,            METH_O       },

    { "CreateRGBSurface",     (PyCFunction)PySDL_CreateRGBSurface,     METH_VARARGS | METH_KEYWORDS },
    { "CreateRGBSurfaceFrom", (PyCFunction)PySDL_CreateRGBSurfaceFrom, METH_VARARGS | METH_KEYWORDS },

    { "PollEvent",             PySDL_PollEvent,             METH_NOARGS  },
    { "WaitEvent",             PySDL_WaitEvent,             METH_NOARGS  },
    { "GetKeyboardState",      PySDL_GetKeyboardState,      METH_NOARGS  },
    { "GetModState",           PySDL_GetModState,           METH_NOARGS  },

    { "GetCPUCount",           PySDL_GetCPUCount,           METH_NOARGS  },
    { "GetCPUCacheLineSize",   PySDL_GetCPUCacheLineSize,   METH_NOARGS  },
    { "Has3DNow",              PySDL_Has3DNow,              METH_NOARGS  },
    { "HasAVX",                PySDL_HasAVX,                METH_NOARGS  },
    { "HasAVX2",               PySDL_HasAVX2,               METH_NOARGS  },
    { "HasAltiVec",            PySDL_HasAltiVec,            METH_NOARGS  },
    { "HasMMX",                PySDL_HasMMX,                METH_NOARGS  },
    { "HasSSE",                PySDL_HasSSE,                METH_NOARGS  },
    { "HasSSE2",               PySDL_HasSSE2,               METH_NOARGS  },
    { "HasSSE3",               PySDL_HasSSE3,               METH_NOARGS  },
    { "HasSSE41",              PySDL_HasSSE41,              METH_NOARGS  },
    { "HasSSE42",              PySDL_HasSSE42,              METH_NOARGS  },

    { "GetNumVideoDisplays",   PySDL_GetNumVideoDisplays,   METH_NOARGS  },
    { "GetDisplayMode",        PySDL_GetDisplayMode,        METH_O       },
    { "GetDisplayBounds",      PySDL_GetDisplayBounds,      METH_O       },

    { "GL_SetAttribute",       PySDL_GL_SetAttribute,       METH_VARARGS },
    { "GL_GetAttribute",       PySDL_GL_GetAttribute,       METH_O       },
    { "GL_ResetAttributes",    PySDL_GL_ResetAttributes,    METH_NOARGS  },
    { "GL_ExtensionSupported", PySDL_GL_ExtensionSupported, METH_O       },
    { "GL_SetSwapInterval",    PySDL_GL_SetSwapInterval,    METH_O       },
    { "GL_GetSwapInterval",    PySDL_GL_GetSwapInterval,    METH_NOARGS  },

    { "GetNumAudioDevices",    PySDL_GetNumAudioDevices,    METH_VARARGS },
    { "GetAudioDeviceName",    PySDL_GetAudioDeviceName,    METH_VARARGS },

    { NULL }
};

static PyModuleDef pysdl_module = {
    PyModuleDef_HEAD_INIT,
    "SDL2",
    NULL,
    -1,
    pysdl_PyMethodDefs
};


PyMODINIT_FUNC PyInit_SDL2(void) {
    PyObject *module;
    int ok;

    module = PyModule_Create(&pysdl_module);
    if(NULL == module) {
        return NULL;
    }

    pysdl_Error = PyErr_NewException("SDL2.error", NULL, NULL);
    Py_INCREF(pysdl_Error);
    PyModule_AddObject(module, "error", pysdl_Error);

    ok = PyType_Ready(&PySDL_Window_Type);
    if(0 > ok) {
        return NULL;
    }
    Py_INCREF(&PySDL_Window_Type);
    PyModule_AddObject(module, "Window", (PyObject *)&PySDL_Window_Type);

    ok = PyType_Ready(&PySDL_Renderer_Type);
    if(0 > ok) {
        return NULL;
    }
    Py_INCREF(&PySDL_Renderer_Type);

    ok = PyType_Ready(&PySDL_Surface_Type);
    if(0 > ok) {
        return NULL;
    }
    Py_INCREF(&PySDL_Surface_Type);

    ok = PyType_Ready(&PySDL_Texture_Type);
    if(0 > ok) {
        return NULL;
    }
    Py_INCREF(&PySDL_Texture_Type);

    ok = PyType_Ready(&PySDL_Audio_Type);
    if(0 > ok) {
        return NULL;
    }
    Py_INCREF(&PySDL_Audio_Type);
    PyModule_AddObject(module, "Audio", (PyObject *)&PySDL_Audio_Type);

    _constants(module);

    return module;
}


static PyObject * PySDL_Init(PyObject *self, PyObject *args) {
    int flags = SDL_INIT_EVERYTHING;
    int ok = PyArg_ParseTuple(args, "|i", &flags);
    if(!ok) {
        return NULL;
    }
    ok = SDL_Init(flags);
    if(0 > ok) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }
    return PyLong_FromLong(ok);
}


static PyObject * PySDL_WasInit(PyObject *self, PyObject *args) {
    int flags = SDL_INIT_EVERYTHING;
    int ok = PyArg_ParseTuple(args, "|i", &flags);
    if(0 > ok) {
        return NULL;
    }
    uint32_t subsystems = SDL_WasInit(flags);
    return PyLong_FromLong(subsystems);
}


static PyObject * PySDL_Quit(PyObject *self, PyObject *ign) {
    SDL_Quit();
    Py_RETURN_NONE;
}


static PyObject * PySDL_Version(PyObject *self, PyObject *ign) {
    SDL_version compiled;
    SDL_VERSION(&compiled);
    return Py_BuildValue("(iii)", compiled.major, compiled.minor, compiled.patch);
}


static PyObject * PySDL_GetPlatform(PyObject *self, PyObject *ign) {
    return PyUnicode_FromString(SDL_GetPlatform());
}


static PyObject * PySDL_GetCurrentVideoDriver(PyObject *self, PyObject *ign) {
    return PyUnicode_FromString(SDL_GetCurrentVideoDriver());
}


static PyObject * PySDL_GetVideoDrivers(PyObject *self, PyObject *ign) {
    PyObject *list;
    int count;
    int idx;

    count = SDL_GetNumVideoDrivers();
    list = PyList_New(count);
    for(idx = 0; idx < count; ++idx) {
        PyList_SetItem(list, idx, PyUnicode_FromString(SDL_GetVideoDriver(idx)));
    }

    return list;
}


static PyObject * PySDL_GetTicks(PyObject *self, PyObject *ign) {
    return PyLong_FromUnsignedLong(SDL_GetTicks());
}


static PyObject * PySDL_LoadBMP(PyObject *self, PyObject *arg) {
    PySDL_Surface *pysdl_Surface = (PySDL_Surface *)PyObject_CallObject((PyObject *)&PySDL_Surface_Type, NULL);
    if(NULL == pysdl_Surface) {
        PyErr_SetString(PyExc_TypeError, "Could not create SDL2.Surface object");
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
        pysdl_Surface->surface = SDL_LoadBMP(PyUnicode_AsUTF8(arg));
    Py_END_ALLOW_THREADS

    if(NULL == pysdl_Surface->surface) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }

    return (PyObject *)pysdl_Surface;
}


static PyObject * PySDL_LoadImage(PyObject *self, PyObject *arg) {
    PySDL_Surface *pysdl_Surface = (PySDL_Surface *)PyObject_CallObject((PyObject *)&PySDL_Surface_Type, NULL);
    if(NULL == pysdl_Surface) {
        PyErr_SetString(PyExc_TypeError, "Could not create SDL2.Surface object");
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
        pysdl_Surface->surface = IMG_Load(PyUnicode_AsUTF8(arg));
    Py_END_ALLOW_THREADS

    if(NULL == pysdl_Surface->surface) {
        PyErr_SetString(pysdl_Error, IMG_GetError());
        return NULL;
    }

    return (PyObject *)pysdl_Surface;
}


static PyObject * PySDL_ShowCursor(PyObject *self, PyObject *args) {
    SDL_ShowCursor(PyLong_AsLong(args));
    Py_RETURN_NONE;
}


static PyObject * PySDL_CreateRGBSurface(PyObject *self, PyObject *args, PyObject *kwds) {
    int w = 0;
    int h = 0;
    int d = 32;

    uint32_t rmask = 0xff000000;
    uint32_t gmask = 0x00ff0000;
    uint32_t bmask = 0x0000ff00;
    uint32_t amask = 0x000000ff;

    static char *kwlist[] = {"size", "depth", "rmask", "gmask", "bmask", "amask", NULL};

    int ok = PyArg_ParseTupleAndKeywords(args, kwds, "(ii)|iIIII", kwlist,
        &w, &h, &d, &rmask, &gmask, &bmask, &amask);
    if(!ok) {
        return NULL;
    }

    PySDL_Surface *pysdl_Surface = (PySDL_Surface *)PyObject_CallObject((PyObject *)&PySDL_Surface_Type, NULL);
    if(NULL == pysdl_Surface) {
        PyErr_SetString(PyExc_TypeError, "Could not create SDL2.Surface object");
        return NULL;
    }

    pysdl_Surface->surface = SDL_CreateRGBSurface(0, w, h, d, rmask, gmask, bmask, amask);
    if(NULL == pysdl_Surface->surface) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }

    return (PyObject *)pysdl_Surface;
}


static PyObject * PySDL_CreateRGBSurfaceFrom(PyObject *self, PyObject *args, PyObject *kwds) {
    const uint8_t *pixels;
    Py_ssize_t pixelsSize;
    int w = 0;
    int h = 0;
    int d = 32;
    uint32_t rmask = 0xff000000;
    uint32_t gmask = 0x00ff0000;
    uint32_t bmask = 0x0000ff00;
    uint32_t amask = 0x000000ff;

    static char *kwlist[] = {"pixels","size", "depth", "rmask", "gmask", "bmask", "amask", NULL};

    int ok = PyArg_ParseTupleAndKeywords(args, kwds, "s#(ii)|iIIII", kwlist,
        &pixels, &pixelsSize, &w, &h, &d, &rmask, &gmask, &bmask, &amask);
    if(!ok) {
        return NULL;
    }

    PySDL_Surface * pysdl_Surface = (PySDL_Surface *)PyObject_CallObject((PyObject *)&PySDL_Surface_Type, NULL);
    if(NULL == pysdl_Surface) {
        PyErr_SetString(PyExc_TypeError, "Could not create SDL2.Surface object");
        return NULL;
    }

    int pitch = w * (d >> 3);
    pysdl_Surface->surface = SDL_CreateRGBSurfaceFrom((void *)pixels, w, h, d, pitch, rmask, gmask, bmask, amask);
    if(NULL == pysdl_Surface->surface) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }

    return (PyObject *)pysdl_Surface;
}



static PyObject * _event(SDL_Event *event) {
    PyObject *result;
    PyObject *data;

    result = PyTuple_New(2);
    PyTuple_SetItem(result, 0, PyLong_FromLong(event->type));

    switch(event->type) {
    //case SDL_ACTIVEEVENT: {
    //        Py_INCREF(Py_None);
    //        data = Py_None;
    //    }
    //    break;
    case SDL_KEYDOWN:
    case SDL_KEYUP: {
            data = PyTuple_New(4);
            SDL_KeyboardEvent *keyboard = (SDL_KeyboardEvent *)event;
            PyTuple_SetItem(data, 0, PyLong_FromLong(keyboard->state));
            PyTuple_SetItem(data, 1, PyLong_FromLong(keyboard->keysym.scancode));
            PyTuple_SetItem(data, 2, PyLong_FromLong(keyboard->keysym.sym));
            PyTuple_SetItem(data, 3, PyLong_FromLong(keyboard->keysym.mod));
            //PyTuple_SetItem(data, 5, PyLong_FromLong(keyboard->keysym.unicode));
        }
        break;
    case SDL_MOUSEMOTION: {
            data = PyTuple_New(5);
            SDL_MouseMotionEvent *mousemotion = (SDL_MouseMotionEvent *)event;
            PyTuple_SetItem(data, 0, PyLong_FromLong(mousemotion->state));
            PyTuple_SetItem(data, 1, PyLong_FromLong(mousemotion->x));
            PyTuple_SetItem(data, 2, PyLong_FromLong(mousemotion->y));
            PyTuple_SetItem(data, 3, PyLong_FromLong(mousemotion->xrel));
            PyTuple_SetItem(data, 4, PyLong_FromLong(mousemotion->yrel));
        }
        break;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP: {
            data = PyTuple_New(5);
            SDL_MouseButtonEvent *mousebutton = (SDL_MouseButtonEvent *)event;
            PyTuple_SetItem(data, 0, PyLong_FromLong(mousebutton->which));
            PyTuple_SetItem(data, 1, PyLong_FromLong(mousebutton->button));
            PyTuple_SetItem(data, 2, PyLong_FromLong(mousebutton->state));
            PyTuple_SetItem(data, 3, PyLong_FromLong(mousebutton->x));
            PyTuple_SetItem(data, 4, PyLong_FromLong(mousebutton->y));
        }
        break;
    case SDL_JOYAXISMOTION: {
            SDL_JoyAxisEvent *joyaxis = (SDL_JoyAxisEvent *)event;
            data = PyTuple_New(3);
            PyTuple_SetItem(data, 0, PyLong_FromLong(joyaxis->which));
            PyTuple_SetItem(data, 1, PyLong_FromLong(joyaxis->axis));
            PyTuple_SetItem(data, 2, PyLong_FromLong(joyaxis->value));
        }
        break;
    //case SDL_JOYBALLMOTION:
    //    data = PyTuple_New(1);
    //    break;
    //case SDL_JOYHATMOTION:
    //    data = PyTuple_New(1);
    //    break;
    case SDL_JOYBUTTONDOWN:
    case SDL_JOYBUTTONUP: {
            data = PyTuple_New(3);
            SDL_JoyButtonEvent *joybutton = (SDL_JoyButtonEvent *)event;
            PyTuple_SetItem(data, 0, PyLong_FromLong(joybutton->which));
            PyTuple_SetItem(data, 1, PyLong_FromLong(joybutton->button));
            PyTuple_SetItem(data, 2, PyLong_FromLong(joybutton->state));
        }
        break;
    case SDL_WINDOWEVENT: {
            data = PyTuple_New(4);
            PyTuple_SetItem(data, 0, PyLong_FromLong(event->window.event));
            PyTuple_SetItem(data, 1, PyLong_FromLong(event->window.data1));
            PyTuple_SetItem(data, 2, PyLong_FromLong(event->window.data2));
            PyTuple_SetItem(data, 3, PyLong_FromUnsignedLong(event->window.windowID));
        }
        break;
    //case SDL_VIDEOEXPOSE:
    //    data = PyTuple_New(1);
    //    break;
    case SDL_QUIT:
        data = Py_None;
        Py_INCREF(data);
        break;
    //case SDL_USEREVENT:
    //    data = PyTuple_New(1);
    //    break;
    //case SDL_SYSWMEVENT:
    //    data = PyTuple_New(1);
    //    break;
    default:
        data = Py_None;
        Py_INCREF(data);
    }

    PyTuple_SetItem(result, 1, data);
    return result;
}

static PyObject * PySDL_PollEvent(PyObject *self, PyObject *ign) {
    SDL_Event event;
    int ok;

    ok = SDL_PollEvent(&event);
    if(0 == ok) {
        Py_RETURN_NONE;
    }
    return _event(&event);
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
    return _event(&event);
}

//PyObject * PySDL_PushEvent(PyObject *self, PyObject *data) {
//    SDL_Event event;
//    event.type = (Uint8)PyInt_AsLong(PyTuple_GetItem(data, 0));
//    SDL_PushEvent(&event);
//    Py_RETURN_NONE;
//}

static PyObject * PySDL_GetKeyboardState(PyObject *self, PyObject *ign) {
    PyObject *list;
    const uint8_t *keys;
    int len;
    int idx;

    keys = SDL_GetKeyboardState(&len);
    list = PyList_New(len);
    for(idx = 0; idx < len; ++idx) {
        PyList_SetItem(list, idx, PyBool_FromLong(keys[idx]));
    }
    return list;
}

static PyObject * PySDL_GetModState(PyObject *self, PyObject *ign) {
    return PyLong_FromLong(SDL_GetModState());
}

static PyObject * PySDL_GetCPUCount(PyObject *self, PyObject *ign) {
    return PyLong_FromLong(SDL_GetCPUCount());
}

static PyObject * PySDL_GetCPUCacheLineSize(PyObject *self, PyObject *ign) {
    return PyLong_FromLong(SDL_GetCPUCacheLineSize());
}

static PyObject * PySDL_Has3DNow(PyObject *self, PyObject *ign) {
    PyObject *hasFeature = SDL_Has3DNow() ? Py_True : Py_False;
    Py_INCREF(hasFeature);
    return hasFeature;
}

static PyObject * PySDL_HasAVX(PyObject *self, PyObject *ign) {
    PyObject *hasFeature = SDL_HasAVX() ? Py_True : Py_False;
    Py_INCREF(hasFeature);
    return hasFeature;
}

static PyObject * PySDL_HasAVX2(PyObject *self, PyObject *ign) {
    PyObject *hasFeature = SDL_HasAVX2() ? Py_True : Py_False;
    Py_INCREF(hasFeature);
    return hasFeature;
}

static PyObject * PySDL_HasAltiVec(PyObject *self, PyObject *ign) {
    PyObject *hasFeature = SDL_HasAltiVec() ? Py_True : Py_False;
    Py_INCREF(hasFeature);
    return hasFeature;
}

static PyObject * PySDL_HasMMX(PyObject *self, PyObject *ign) {
    PyObject *hasFeature = SDL_HasMMX() ? Py_True : Py_False;
    Py_INCREF(hasFeature);
    return hasFeature;
}

static PyObject * PySDL_HasSSE(PyObject *self, PyObject *ign) {
    PyObject *hasFeature = SDL_HasSSE() ? Py_True : Py_False;
    Py_INCREF(hasFeature);
    return hasFeature;
}

static PyObject * PySDL_HasSSE2(PyObject *self, PyObject *ign) {
    PyObject *hasFeature = SDL_HasSSE2() ? Py_True : Py_False;
    Py_INCREF(hasFeature);
    return hasFeature;
}

static PyObject * PySDL_HasSSE3(PyObject *self, PyObject *ign) {
    PyObject *hasFeature = SDL_HasSSE3() ? Py_True : Py_False;
    Py_INCREF(hasFeature);
    return hasFeature;
}

static PyObject * PySDL_HasSSE41(PyObject *self, PyObject *ign) {
    PyObject *hasFeature = SDL_HasSSE41() ? Py_True : Py_False;
    Py_INCREF(hasFeature);
    return hasFeature;
}

static PyObject * PySDL_HasSSE42(PyObject *self, PyObject *ign) {
    PyObject *hasFeature = SDL_HasSSE42() ? Py_True : Py_False;
    Py_INCREF(hasFeature);
    return hasFeature;
}

static PyObject * PySDL_GetNumVideoDisplays(PyObject *self, PyObject *ign) {
    int displays = SDL_GetNumVideoDisplays();
    if(0 > displays) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }
    return PyLong_FromLong(displays);
}

static PyObject * PySDL_GetDisplayMode(PyObject *self, PyObject *args) {
    SDL_DisplayMode dm;
    int ok;

    ok = SDL_GetDisplayMode(PyLong_AsLong(args), 0, &dm);
    if(0 > ok) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }
    return Py_BuildValue("(iiii)", dm.format, dm.w, dm.h, dm.refresh_rate);
}

static PyObject * PySDL_GetDisplayBounds(PyObject *self, PyObject *args) {
    SDL_Rect rect;
    int ok = SDL_GetDisplayBounds(PyLong_AsLong(args), &rect);
    if(0 > ok) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }
    return Py_BuildValue("(iiii)", rect.x, rect.y, rect.w, rect.h);
}

static PyObject * PySDL_GL_SetAttribute(PyObject *self, PyObject *args) {
    int attrib;
    int value;
    int ok = PyArg_ParseTuple(args, "ii", &attrib, &value);
    if(!ok) {
        return NULL;
    }
    ok = SDL_GL_SetAttribute(attrib, value);
    if(0 > ok) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject * PySDL_GL_GetAttribute(PyObject *self, PyObject *arg) {
    int attribute = PyLong_AsLong(arg);
    int value;
    int ok = SDL_GL_GetAttribute(attribute, &value);
    if(0 > ok) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }
    return PyLong_FromLong(value);
}

static PyObject * PySDL_GL_ResetAttributes(PyObject *self, PyObject *ign) {
    SDL_GL_ResetAttributes();
    Py_RETURN_NONE;
}

static PyObject * PySDL_GL_ExtensionSupported(PyObject *self, PyObject *arg) {
    char *extension = PyBytes_AsString(arg);
    if(NULL == extension) {
        return NULL;
    }

    SDL_bool isSupported = SDL_GL_ExtensionSupported(extension);
    if(isSupported == SDL_TRUE) {
        Py_RETURN_TRUE;
    }
    else {
        Py_RETURN_FALSE;
    }
}

static PyObject * PySDL_GL_SetSwapInterval(PyObject *self, PyObject *arg) {
    int value = PyLong_AsLong(arg);
    int ok = SDL_GL_SetSwapInterval(value);
    if(0 > ok) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject * PySDL_GL_GetSwapInterval(PyObject *self, PyObject *ign) {
    return PyLong_FromLong(SDL_GL_GetSwapInterval());
}

static PyObject * PySDL_GetNumAudioDevices(PyObject *self, PyObject *args) {
    // TODO: iscapture arg
    return PyLong_FromLong(SDL_GetNumAudioDevices(0));
}

static PyObject * PySDL_GetAudioDeviceName(PyObject *self, PyObject *args) {
    // TODO: iscapture arg
    int idx = PyLong_AsLong(PyTuple_GetItem(args, 0));
    return PyUnicode_FromString(SDL_GetAudioDeviceName(idx, 0));
}

