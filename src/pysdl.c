#include "pysdl.h"

PyObject *pysdl_Error;

static PyObject * PySDL_Init                  (PyObject*, PyObject*);
static PyObject * PySDL_WasInit               (PyObject*, PyObject*);
static PyObject * PySDL_Quit                  (PyObject*, PyObject*);
static PyObject * PySDL_GetError              (PyObject*, PyObject*);
static PyObject * PySDL_Version               (PyObject*, PyObject*);
static PyObject * PySDL_GetPlatform           (PyObject*, PyObject*);
static PyObject * PySDL_GetCurrentVideoDriver (PyObject*, PyObject*);
static PyObject * PySDL_GetVideoDrivers       (PyObject*, PyObject*);
static PyObject * PySDL_GetTicks              (PyObject*, PyObject*);
static PyObject * PySDL_LoadBMP               (PyObject*, PyObject*);
static PyObject * PySDL_LoadImage             (PyObject*, PyObject*);
static PyObject * PySDL_ShowCursor            (PyObject*, PyObject*);

static PyObject * PySDL_CreateRGBSurface             (PyObject*, PyObject*, PyObject *);
static PyObject * PySDL_CreateRGBSurfaceFrom         (PyObject*, PyObject*, PyObject *);
static PyObject * PySDL_CreateRGBSurfaceWithFormat     (PyObject*, PyObject*, PyObject *);
static PyObject * PySDL_CreateRGBSurfaceWithFormatFrom (PyObject*, PyObject*, PyObject *);

static PyObject * PySDL_GetPixelFormatName     (PyObject*, PyObject*);
static PyObject * PySDL_PixelFormatEnumToMasks (PyObject*, PyObject*);
static PyObject * PySDL_MasksToPixelFormatEnum (PyObject*, PyObject*);

static PyObject * PySDL_IMG_Init (PyObject*, PyObject*);
static PyObject * PySDL_IMG_Quit (PyObject*, PyObject*);

static PyObject * PySDL_CreateSoftwareRenderer (PyObject*, PyObject*);
static PyObject * PySDL_ComposeCustomBlendMode (PyObject*, PyObject*);

static PyObject * PySDL_PollEvent             (PyObject*, PyObject*);
static PyObject * PySDL_WaitEvent             (PyObject*, PyObject*);
static PyObject * PySDL_GetKeyState           (PyObject*, PyObject*);
static PyObject * PySDL_GetModState           (PyObject*, PyObject*);

static PyObject * PySDL_GetCPUCount           (PyObject*, PyObject*);
static PyObject * PySDL_GetCPUCacheLineSize   (PyObject*, PyObject*);
static PyObject * PySDL_Has3DNow              (PyObject*, PyObject*);
static PyObject * PySDL_HasAVX                (PyObject*, PyObject*);
static PyObject * PySDL_HasAVX2               (PyObject*, PyObject*);
static PyObject * PySDL_HasAltiVec            (PyObject*, PyObject*);
static PyObject * PySDL_HasMMX                (PyObject*, PyObject*);
static PyObject * PySDL_HasSSE                (PyObject*, PyObject*);
static PyObject * PySDL_HasSSE2               (PyObject*, PyObject*);
static PyObject * PySDL_HasSSE3               (PyObject*, PyObject*);
static PyObject * PySDL_HasSSE41              (PyObject*, PyObject*);
static PyObject * PySDL_HasSSE42              (PyObject*, PyObject*);

static PyObject * PySDL_GetNumVideoDisplays   (PyObject*, PyObject*);
static PyObject * PySDL_GetDisplayMode        (PyObject*, PyObject*);
static PyObject * PySDL_GetDesktopDisplayMode (PyObject*, PyObject*);
static PyObject * PySDL_GetCurrentDisplayMode (PyObject*, PyObject*);
static PyObject * PySDL_GetDisplayBounds      (PyObject*, PyObject*);
static PyObject * PySDL_GetDisplayDPI         (PyObject*, PyObject*);

static PyObject * PySDL_GL_SetAttribute       (PyObject*, PyObject*);
static PyObject * PySDL_GL_GetAttribute       (PyObject*, PyObject*);
static PyObject * PySDL_GL_ResetAttributes    (PyObject*, PyObject*);
static PyObject * PySDL_GL_ExtensionSupported (PyObject*, PyObject*);
static PyObject * PySDL_GL_SetSwapInterval    (PyObject*, PyObject*);
static PyObject * PySDL_GL_GetSwapInterval    (PyObject*, PyObject*);

static PyObject * PySDL_GetNumRenderDrivers   (PyObject*, PyObject*);
static PyObject * PySDL_GetRenderDriverInfo   (PyObject*, PyObject*);

static PyObject * PySDL_GetNumAudioDevices    (PyObject*, PyObject*);
static PyObject * PySDL_GetAudioDeviceName    (PyObject*, PyObject*);

static PyObject * PySDL_Delay                    (PyObject*, PyObject*);
#if SDL_VERSION_ATLEAST(2,0,18)
static PyObject * PySDL_GetTicks64               (PyObject*, PyObject*);
#endif
static PyObject * PySDL_GetPerformanceCounter    (PyObject*, PyObject*);
static PyObject * PySDL_GetPerformanceFrequency  (PyObject*, PyObject*);
static PyObject * PySDL_GetVersion               (PyObject*, PyObject*);
static PyObject * PySDL_GetRevision              (PyObject*, PyObject*);
static PyObject * PySDL_SetError                 (PyObject*, PyObject*);
static PyObject * PySDL_ClearError               (PyObject*, PyObject*);
static PyObject * PySDL_GetBasePath              (PyObject*, PyObject*);
static PyObject * PySDL_GetPrefPath              (PyObject*, PyObject*);
static PyObject * PySDL_EnableScreenSaver        (PyObject*, PyObject*);
static PyObject * PySDL_DisableScreenSaver       (PyObject*, PyObject*);
static PyObject * PySDL_IsScreenSaverEnabled     (PyObject*, PyObject*);
static PyObject * PySDL_GetClipboardText         (PyObject*, PyObject*);
static PyObject * PySDL_SetClipboardText         (PyObject*, PyObject*);
static PyObject * PySDL_HasClipboardText         (PyObject*, PyObject*);

static PyMethodDef pysdl_PyMethodDefs[] = {
    { "Init",                  PySDL_Init,                  METH_VARARGS },
    { "WasInit",               PySDL_WasInit,               METH_VARARGS },
    { "Quit",                  PySDL_Quit,                  METH_NOARGS  },
    { "GetError",              PySDL_GetError,              METH_NOARGS  },
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
    { "CreateRGBSurfaceWithFormat",     (PyCFunction)PySDL_CreateRGBSurfaceWithFormat,     METH_VARARGS | METH_KEYWORDS },
    { "CreateRGBSurfaceWithFormatFrom", (PyCFunction)PySDL_CreateRGBSurfaceWithFormatFrom, METH_VARARGS | METH_KEYWORDS },

    { "GetPixelFormatName",     PySDL_GetPixelFormatName,     METH_O       },
    { "PixelFormatEnumToMasks", PySDL_PixelFormatEnumToMasks, METH_O       },
    { "MasksToPixelFormatEnum", PySDL_MasksToPixelFormatEnum, METH_VARARGS },

    { "IMG_Init",             PySDL_IMG_Init,              METH_VARARGS },
    { "IMG_Quit",             PySDL_IMG_Quit,              METH_NOARGS  },

    { "CreateSoftwareRenderer", PySDL_CreateSoftwareRenderer, METH_O       },
    { "ComposeCustomBlendMode", PySDL_ComposeCustomBlendMode, METH_VARARGS },

    { "PollEvent",             PySDL_PollEvent,             METH_NOARGS  },
    { "WaitEvent",             PySDL_WaitEvent,             METH_NOARGS  },
    { "GetKeyState",           PySDL_GetKeyState,           METH_NOARGS  },
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
    { "GetDesktopDisplayMode", PySDL_GetDesktopDisplayMode, METH_O       },
    { "GetCurrentDisplayMode", PySDL_GetCurrentDisplayMode, METH_O       },
    { "GetDisplayBounds",      PySDL_GetDisplayBounds,      METH_O       },
    { "GetDisplayDPI",         PySDL_GetDisplayDPI,         METH_O       },

    { "GL_SetAttribute",       PySDL_GL_SetAttribute,       METH_VARARGS },
    { "GL_GetAttribute",       PySDL_GL_GetAttribute,       METH_O       },
    { "GL_ResetAttributes",    PySDL_GL_ResetAttributes,    METH_NOARGS  },
    { "GL_ExtensionSupported", PySDL_GL_ExtensionSupported, METH_O       },
    { "GL_SetSwapInterval",    PySDL_GL_SetSwapInterval,    METH_O       },
    { "GL_GetSwapInterval",    PySDL_GL_GetSwapInterval,    METH_NOARGS  },

    { "GetNumRenderDrivers",   PySDL_GetNumRenderDrivers,   METH_NOARGS  },
    { "GetRenderDriverInfo",   PySDL_GetRenderDriverInfo,   METH_O       },

    { "GetNumAudioDevices",    PySDL_GetNumAudioDevices,    METH_VARARGS },
    { "GetAudioDeviceName",    PySDL_GetAudioDeviceName,    METH_VARARGS },

    { "Delay",                    PySDL_Delay,                    METH_O       },
#if SDL_VERSION_ATLEAST(2,0,18)
    { "GetTicks64",               PySDL_GetTicks64,               METH_NOARGS  },
#endif
    { "GetPerformanceCounter",    PySDL_GetPerformanceCounter,    METH_NOARGS  },
    { "GetPerformanceFrequency",  PySDL_GetPerformanceFrequency,  METH_NOARGS  },
    { "GetVersion",               PySDL_GetVersion,               METH_NOARGS  },
    { "GetRevision",              PySDL_GetRevision,              METH_NOARGS  },
    { "SetError",                 PySDL_SetError,                 METH_O       },
    { "ClearError",               PySDL_ClearError,               METH_NOARGS  },
    { "GetBasePath",              PySDL_GetBasePath,              METH_NOARGS  },
    { "GetPrefPath",              PySDL_GetPrefPath,              METH_VARARGS },
    { "EnableScreenSaver",        PySDL_EnableScreenSaver,        METH_NOARGS  },
    { "DisableScreenSaver",       PySDL_DisableScreenSaver,       METH_NOARGS  },
    { "IsScreenSaverEnabled",     PySDL_IsScreenSaverEnabled,     METH_NOARGS  },
    { "GetClipboardText",         PySDL_GetClipboardText,         METH_NOARGS  },
    { "SetClipboardText",         PySDL_SetClipboardText,         METH_O       },
    { "HasClipboardText",         PySDL_HasClipboardText,         METH_NOARGS  },

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

    module = PyModule_Create(&pysdl_module);
    if(NULL == module) {
        return NULL;
    }

    pysdl_Error = PyErr_NewException("SDL2.error", NULL, NULL);
    Py_INCREF(pysdl_Error);
    PyModule_AddObject(module, "error", pysdl_Error);

    if(0 > PyType_Ready(&PySDL_Window_Type)) {
        return NULL;
    }
    Py_INCREF(&PySDL_Window_Type);
    PyModule_AddObject(module, "Window", (PyObject *)&PySDL_Window_Type);

    if(0 > PyType_Ready(&PySDL_Renderer_Type)) {
        return NULL;
    }
    Py_INCREF(&PySDL_Renderer_Type);
    PyModule_AddObject(module, "Renderer", (PyObject *)&PySDL_Renderer_Type);

    if(0 > PyType_Ready(&PySDL_Surface_Type)) {
        return NULL;
    }
    Py_INCREF(&PySDL_Surface_Type);

    if(0 > PyType_Ready(&PySDL_Texture_Type)) {
        return NULL;
    }
    Py_INCREF(&PySDL_Texture_Type);
    PyModule_AddObject(module, "Texture", (PyObject *)&PySDL_Texture_Type);

    if(0 > PyType_Ready(&PySDL_PixelFormat_Type)) {
        return NULL;
    }
    Py_INCREF(&PySDL_PixelFormat_Type);
    PyModule_AddObject(module, "PixelFormat", (PyObject *)&PySDL_PixelFormat_Type);

    if(0 > PyType_Ready(&PySDL_Palette_Type)) {
        return NULL;
    }
    Py_INCREF(&PySDL_Palette_Type);
    PyModule_AddObject(module, "Palette", (PyObject *)&PySDL_Palette_Type);

    if(0 > PyType_Ready(&PySDL_Audio_Type)) {
        return NULL;
    }
    Py_INCREF(&PySDL_Audio_Type);
    PyModule_AddObject(module, "Audio", (PyObject *)&PySDL_Audio_Type);

    if(0 > PyType_Ready(&PySDL_Cursor_Type)) {
        return NULL;
    }
    Py_INCREF(&PySDL_Cursor_Type);
    PyModule_AddObject(module, "Cursor", (PyObject *)&PySDL_Cursor_Type);

    if(0 > PyModule_AddFunctions(module, pysdl_input_methods)) {
        return NULL;
    }
    if(0 > PyModule_AddFunctions(module, pysdl_cursor_methods)) {
        return NULL;
    }

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

static PyObject * PySDL_GetError(PyObject *self, PyObject *ign) {
    return PyUnicode_FromString(SDL_GetError());
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

// Both loaders take a filesystem path (str) or the file's bytes.
static PyObject * _load_surface(PyObject *arg, int is_image) {
    PySDL_Surface *wrapper = (PySDL_Surface *)PySDL_New(&PySDL_Surface_Type);
    if(NULL == wrapper) {
        return NULL;
    }

    if(PyBytes_Check(arg) || PyByteArray_Check(arg)) {
        Py_buffer buffer;
        if(0 > PyObject_GetBuffer(arg, &buffer, PyBUF_SIMPLE)) {
            Py_DECREF(wrapper);
            return NULL;
        }
        SDL_RWops *rw = SDL_RWFromConstMem(buffer.buf, (int)buffer.len);
        if(NULL != rw) {
            Py_BEGIN_ALLOW_THREADS
                wrapper->surface = is_image ? IMG_Load_RW(rw, 1) : SDL_LoadBMP_RW(rw, 1);
            Py_END_ALLOW_THREADS
        }
        PyBuffer_Release(&buffer);
    } else {
        const char *path = PyUnicode_AsUTF8(arg);
        if(NULL == path) {
            Py_DECREF(wrapper);
            return NULL;
        }
        Py_BEGIN_ALLOW_THREADS
            wrapper->surface = is_image ? IMG_Load(path) : SDL_LoadBMP(path);
        Py_END_ALLOW_THREADS
    }

    if(NULL == wrapper->surface) {
        Py_DECREF(wrapper);
        PyErr_SetString(pysdl_Error, is_image ? IMG_GetError() : SDL_GetError());
        return NULL;
    }

    return (PyObject *)wrapper;
}

static PyObject * PySDL_LoadBMP(PyObject *self, PyObject *arg) {
    return _load_surface(arg, 0);
}

static PyObject * PySDL_LoadImage(PyObject *self, PyObject *arg) {
    return _load_surface(arg, 1);
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

    PySDL_Surface *pysdl_Surface = (PySDL_Surface *)PySDL_New(&PySDL_Surface_Type);
    if(NULL == pysdl_Surface) {
        return NULL;
    }

    pysdl_Surface->surface = SDL_CreateRGBSurface(0, w, h, d, rmask, gmask, bmask, amask);
    if(NULL == pysdl_Surface->surface) {
        Py_DECREF(pysdl_Surface);
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }

    return (PyObject *)pysdl_Surface;
}

static PyObject * PySDL_CreateRGBSurfaceFrom(PyObject *self, PyObject *args, PyObject *kwds) {
    Py_buffer pixels;
    int w = 0;
    int h = 0;
    int d = 32;
    int pitch = 0;
    uint32_t rmask = 0xff000000;
    uint32_t gmask = 0x00ff0000;
    uint32_t bmask = 0x0000ff00;
    uint32_t amask = 0x000000ff;

    static char *kwlist[] = {"pixels", "size", "depth", "rmask", "gmask", "bmask", "amask", "pitch", NULL};

    int ok = PyArg_ParseTupleAndKeywords(args, kwds, "y*(ii)|iIIIIi", kwlist,
        &pixels, &w, &h, &d, &rmask, &gmask, &bmask, &amask, &pitch);
    if(!ok) {
        return NULL;
    }

    PySDL_Surface *pysdl_Surface = (PySDL_Surface *)PySDL_New(&PySDL_Surface_Type);
    if(NULL == pysdl_Surface) {
        PyBuffer_Release(&pixels);
        return NULL;
    }

    if(pitch <= 0) {
        pitch = w * (d >> 3);
    }
    pysdl_Surface->surface = SDL_CreateRGBSurfaceFrom(pixels.buf, w, h, d, pitch, rmask, gmask, bmask, amask);
    if(NULL == pysdl_Surface->surface) {
        Py_DECREF(pysdl_Surface);
        PyBuffer_Release(&pixels);
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }

    // Keep the buffer alive for the life of the surface (SDL does not copy it).
    pysdl_Surface->pixels = pixels;
    return (PyObject *)pysdl_Surface;
}

static PyObject * PySDL_CreateRGBSurfaceWithFormat(PyObject *self, PyObject *args, PyObject *kwds) {
    int w = 0;
    int h = 0;
    int depth = 0;
    unsigned int format;

    static char *kwlist[] = {"size", "format", "depth", NULL};
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "(ii)I|i", kwlist, &w, &h, &format, &depth)) {
        return NULL;
    }
    if(depth <= 0) {
        depth = SDL_BITSPERPIXEL(format);
    }

    PySDL_Surface *pysdl_Surface = (PySDL_Surface *)PySDL_New(&PySDL_Surface_Type);
    if(NULL == pysdl_Surface) {
        return NULL;
    }

    pysdl_Surface->surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, depth, format);
    if(NULL == pysdl_Surface->surface) {
        Py_DECREF(pysdl_Surface);
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }

    return (PyObject *)pysdl_Surface;
}

static PyObject * PySDL_CreateRGBSurfaceWithFormatFrom(PyObject *self, PyObject *args, PyObject *kwds) {
    Py_buffer pixels;
    int w = 0;
    int h = 0;
    int depth = 0;
    int pitch = 0;
    unsigned int format;

    static char *kwlist[] = {"pixels", "size", "format", "depth", "pitch", NULL};
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "y*(ii)I|ii", kwlist,
        &pixels, &w, &h, &format, &depth, &pitch)) {
        return NULL;
    }
    if(depth <= 0) {
        depth = SDL_BITSPERPIXEL(format);
    }
    if(pitch <= 0) {
        pitch = w * SDL_BYTESPERPIXEL(format);
    }

    PySDL_Surface *pysdl_Surface = (PySDL_Surface *)PySDL_New(&PySDL_Surface_Type);
    if(NULL == pysdl_Surface) {
        PyBuffer_Release(&pixels);
        return NULL;
    }

    pysdl_Surface->surface = SDL_CreateRGBSurfaceWithFormatFrom(pixels.buf, w, h, depth, pitch, format);
    if(NULL == pysdl_Surface->surface) {
        Py_DECREF(pysdl_Surface);
        PyBuffer_Release(&pixels);
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }

    pysdl_Surface->pixels = pixels;
    return (PyObject *)pysdl_Surface;
}

static PyObject * PySDL_GetPixelFormatName(PyObject *self, PyObject *arg) {
    unsigned long format = PyLong_AsUnsignedLong(arg);
    if((unsigned long)-1 == format && PyErr_Occurred()) {
        return NULL;
    }
    return PyUnicode_FromString(SDL_GetPixelFormatName((Uint32)format));
}

static PyObject * PySDL_PixelFormatEnumToMasks(PyObject *self, PyObject *arg) {
    unsigned long format = PyLong_AsUnsignedLong(arg);
    if((unsigned long)-1 == format && PyErr_Occurred()) {
        return NULL;
    }

    int bpp = 0;
    Uint32 rmask, gmask, bmask, amask;
    if(SDL_FALSE == SDL_PixelFormatEnumToMasks((Uint32)format, &bpp, &rmask, &gmask, &bmask, &amask)) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }
    return Py_BuildValue("(iIIII)", bpp, rmask, gmask, bmask, amask);
}

static PyObject * PySDL_MasksToPixelFormatEnum(PyObject *self, PyObject *args) {
    int bpp;
    uint32_t rmask, gmask, bmask, amask;
    if(!PyArg_ParseTuple(args, "iIIII", &bpp, &rmask, &gmask, &bmask, &amask)) {
        return NULL;
    }
    return PyLong_FromUnsignedLong(SDL_MasksToPixelFormatEnum(bpp, rmask, gmask, bmask, amask));
}

static PyObject * PySDL_IMG_Init(PyObject *self, PyObject *args) {
    int flags = IMG_INIT_JPG | IMG_INIT_PNG;
    if(!PyArg_ParseTuple(args, "|i", &flags)) {
        return NULL;
    }
    int got = IMG_Init(flags);
    if(flags && (got & flags) != flags) {
        PyErr_SetString(pysdl_Error, IMG_GetError());
        return NULL;
    }
    return PyLong_FromLong(got);
}

static PyObject * PySDL_IMG_Quit(PyObject *self, PyObject *ign) {
    IMG_Quit();
    Py_RETURN_NONE;
}

static PyObject * PySDL_CreateSoftwareRenderer(PyObject *self, PyObject *arg) {
    if(!PyObject_TypeCheck(arg, &PySDL_Surface_Type)) {
        PyErr_SetString(PyExc_TypeError, "expected an SDL2.Surface");
        return NULL;
    }

    PySDL_Renderer *pysdl_Renderer = (PySDL_Renderer *)PySDL_New(&PySDL_Renderer_Type);
    if(NULL == pysdl_Renderer) {
        return NULL;
    }

    pysdl_Renderer->renderer = SDL_CreateSoftwareRenderer(((PySDL_Surface *)arg)->surface);
    if(NULL == pysdl_Renderer->renderer) {
        Py_DECREF(pysdl_Renderer);
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }

    return (PyObject *)pysdl_Renderer;
}

static PyObject * PySDL_ComposeCustomBlendMode(PyObject *self, PyObject *args) {
    int srcColor, dstColor, colorOp, srcAlpha, dstAlpha, alphaOp;

    if(!PyArg_ParseTuple(args, "iiiiii",
        &srcColor, &dstColor, &colorOp, &srcAlpha, &dstAlpha, &alphaOp)) {
        return NULL;
    }

    SDL_BlendMode mode = SDL_ComposeCustomBlendMode(
        (SDL_BlendFactor)srcColor, (SDL_BlendFactor)dstColor, (SDL_BlendOperation)colorOp,
        (SDL_BlendFactor)srcAlpha, (SDL_BlendFactor)dstAlpha, (SDL_BlendOperation)alphaOp);

    return PyLong_FromLong(mode);
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
            data = PyTuple_New(5);
            SDL_KeyboardEvent *keyboard = (SDL_KeyboardEvent *)event;
            PyTuple_SetItem(data, 0, PyLong_FromLong(keyboard->state));
            PyTuple_SetItem(data, 1, PyLong_FromLong(keyboard->keysym.scancode));
            PyTuple_SetItem(data, 2, PyLong_FromLong(keyboard->keysym.sym));
            PyTuple_SetItem(data, 3, PyLong_FromLong(keyboard->keysym.mod));
            PyTuple_SetItem(data, 4, PyBool_FromLong(keyboard->repeat));
        }
        break;
    case SDL_TEXTINPUT:
        data = PyUnicode_FromString(event->text.text);
        break;
    case SDL_TEXTEDITING: {
            data = PyTuple_New(3);
            PyTuple_SetItem(data, 0, PyUnicode_FromString(event->edit.text));
            PyTuple_SetItem(data, 1, PyLong_FromLong(event->edit.start));
            PyTuple_SetItem(data, 2, PyLong_FromLong(event->edit.length));
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
            data = PyTuple_New(3);
            SDL_JoyAxisEvent *joyaxis = (SDL_JoyAxisEvent *)event;
            PyTuple_SetItem(data, 0, PyLong_FromLong(joyaxis->which));
            PyTuple_SetItem(data, 1, PyLong_FromLong(joyaxis->axis));
            PyTuple_SetItem(data, 2, PyLong_FromLong(joyaxis->value));
        }
        break;
    case SDL_MOUSEWHEEL: {
            data = PyTuple_New(4);
            SDL_MouseWheelEvent *mousewheel = (SDL_MouseWheelEvent *)event;
            PyTuple_SetItem(data, 0, PyLong_FromLong(mousewheel->which));
            PyTuple_SetItem(data, 1, PyLong_FromLong(mousewheel->x));
            PyTuple_SetItem(data, 2, PyLong_FromLong(mousewheel->y));
            PyTuple_SetItem(data, 3, PyLong_FromLong(mousewheel->direction));
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

static PyObject * PySDL_GetKeyState(PyObject *self, PyObject *ign) {
    PyObject *list;
    const uint8_t *keys;
    int len;

    keys = SDL_GetKeyboardState(&len);
    list = PyList_New(len);
    for(int idx = 0; idx < len; ++idx) {
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

    if(0 > SDL_GetDisplayMode(PyLong_AsLong(args), 0, &dm)) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }

    return Py_BuildValue("(iiii)", dm.format, dm.w, dm.h, dm.refresh_rate);
}

static PyObject * PySDL_GetDesktopDisplayMode(PyObject *self, PyObject *args) {
    SDL_DisplayMode dm;

    if(0 > SDL_GetDesktopDisplayMode(PyLong_AsLong(args), &dm)) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }

    return Py_BuildValue("(iiii)", dm.format, dm.w, dm.h, dm.refresh_rate);
}

static PyObject * PySDL_GetCurrentDisplayMode(PyObject *self, PyObject *args) {
    SDL_DisplayMode dm;

    if(0 > SDL_GetCurrentDisplayMode(PyLong_AsLong(args), &dm)) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }

    return Py_BuildValue("(iiii)", dm.format, dm.w, dm.h, dm.refresh_rate);
}

static PyObject * PySDL_GetDisplayBounds(PyObject *self, PyObject *args) {
    SDL_Rect rect;

    if(0 > SDL_GetDisplayBounds(PyLong_AsLong(args), &rect)) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }

    return Py_BuildValue("(iiii)", rect.x, rect.y, rect.w, rect.h);
}

static PyObject * PySDL_GetDisplayDPI(PyObject *self, PyObject *args) {
    float ddpi = 0;
    float hdpi = 0;
    float vdpi = 0;

    if(0 > SDL_GetDisplayDPI(PyLong_AsLong(args), &ddpi, &hdpi, &vdpi)) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }

    return Py_BuildValue("(fff)", ddpi, hdpi, vdpi);
}

static PyObject * PySDL_GL_SetAttribute(PyObject *self, PyObject *args) {
    int attrib;
    int value;

    if(0 > PyArg_ParseTuple(args, "ii", &attrib, &value)) {
        return NULL;
    }

    if(0 > SDL_GL_SetAttribute(attrib, value)) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject * PySDL_GL_GetAttribute(PyObject *self, PyObject *arg) {
    int attribute = PyLong_AsLong(arg);
    int value;

    if(0 > SDL_GL_GetAttribute(attribute, &value)) {
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
    } else {
        Py_RETURN_FALSE;
    }
}

static PyObject * PySDL_GL_SetSwapInterval(PyObject *self, PyObject *arg) {
    int value = PyLong_AsLong(arg);

    if(0 > SDL_GL_SetSwapInterval(value)) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject * PySDL_GL_GetSwapInterval(PyObject *self, PyObject *ign) {
    return PyLong_FromLong(SDL_GL_GetSwapInterval());
}

static PyObject * PySDL_GetNumRenderDrivers(PyObject *self, PyObject *args) {
    return PyLong_FromLong(SDL_GetNumRenderDrivers());
}

static PyObject * PySDL_GetRenderDriverInfo(PyObject *self, PyObject *args) {
    SDL_RendererInfo ri;
    PyObject *list;
    PyObject *ret;

    if(0 > SDL_GetRenderDriverInfo(PyLong_AsLong(args), &ri)) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }

    list = PyList_New(ri.num_texture_formats);
    for(uint32_t idx = 0; idx < ri.num_texture_formats; ++idx) {
        PyList_SetItem(list, idx, PyLong_FromUnsignedLong(ri.texture_formats[idx]));
    }

    ret = Py_BuildValue("(sIOii)", ri.name, ri.flags, list, ri.max_texture_width, ri.max_texture_height);

    Py_DECREF(list);

    return ret;
}

static PyObject * PySDL_GetNumAudioDevices(PyObject *self, PyObject *args) {
    int isCapture = 0;
    if(0 > PyArg_ParseTuple(args, "|p", &isCapture)) {
        return NULL;
    }

    return PyLong_FromLong(SDL_GetNumAudioDevices(isCapture));
}

static PyObject * PySDL_GetAudioDeviceName(PyObject *self, PyObject *args) {
    int idx;
    int isCapture = 0;

    if(0 > PyArg_ParseTuple(args, "i|p", &idx, &isCapture)) {
        return NULL;
    }

    return PyUnicode_FromString(SDL_GetAudioDeviceName(idx, isCapture));
}

static PyObject * PySDL_Delay(PyObject *self, PyObject *arg) {
    long ms = PyLong_AsLong(arg);
    if(-1 == ms && PyErr_Occurred()) {
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
        SDL_Delay((Uint32)ms);
    Py_END_ALLOW_THREADS
    Py_RETURN_NONE;
}

#if SDL_VERSION_ATLEAST(2,0,18)
static PyObject * PySDL_GetTicks64(PyObject *self, PyObject *ign) {
    return PyLong_FromUnsignedLongLong(SDL_GetTicks64());
}
#endif

static PyObject * PySDL_GetPerformanceCounter(PyObject *self, PyObject *ign) {
    return PyLong_FromUnsignedLongLong(SDL_GetPerformanceCounter());
}

static PyObject * PySDL_GetPerformanceFrequency(PyObject *self, PyObject *ign) {
    return PyLong_FromUnsignedLongLong(SDL_GetPerformanceFrequency());
}

static PyObject * PySDL_GetVersion(PyObject *self, PyObject *ign) {
    SDL_version linked;
    SDL_GetVersion(&linked);
    return Py_BuildValue("(iii)", linked.major, linked.minor, linked.patch);
}

static PyObject * PySDL_GetRevision(PyObject *self, PyObject *ign) {
    return PyUnicode_FromString(SDL_GetRevision());
}

static PyObject * PySDL_SetError(PyObject *self, PyObject *arg) {
    const char *msg = PyUnicode_AsUTF8(arg);
    if(NULL == msg) {
        return NULL;
    }
    SDL_SetError("%s", msg);
    Py_RETURN_NONE;
}

static PyObject * PySDL_ClearError(PyObject *self, PyObject *ign) {
    SDL_ClearError();
    Py_RETURN_NONE;
}

static PyObject * PySDL_GetBasePath(PyObject *self, PyObject *ign) {
    char *path = SDL_GetBasePath();
    if(NULL == path) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }
    PyObject *result = PyUnicode_FromString(path);
    SDL_free(path);
    return result;
}

static PyObject * PySDL_GetPrefPath(PyObject *self, PyObject *args) {
    const char *org;
    const char *app;
    if(!PyArg_ParseTuple(args, "ss", &org, &app)) {
        return NULL;
    }
    char *path = SDL_GetPrefPath(org, app);
    if(NULL == path) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }
    PyObject *result = PyUnicode_FromString(path);
    SDL_free(path);
    return result;
}

static PyObject * PySDL_EnableScreenSaver(PyObject *self, PyObject *ign) {
    SDL_EnableScreenSaver();
    Py_RETURN_NONE;
}

static PyObject * PySDL_DisableScreenSaver(PyObject *self, PyObject *ign) {
    SDL_DisableScreenSaver();
    Py_RETURN_NONE;
}

static PyObject * PySDL_IsScreenSaverEnabled(PyObject *self, PyObject *ign) {
    return PyBool_FromLong(SDL_IsScreenSaverEnabled());
}

static PyObject * PySDL_GetClipboardText(PyObject *self, PyObject *ign) {
    char *text = SDL_GetClipboardText();
    PyObject *result = PyUnicode_FromString(text ? text : "");
    SDL_free(text);
    return result;
}

static PyObject * PySDL_SetClipboardText(PyObject *self, PyObject *arg) {
    const char *text = PyUnicode_AsUTF8(arg);
    if(NULL == text) {
        return NULL;
    }
    if(0 != SDL_SetClipboardText(text)) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject * PySDL_HasClipboardText(PyObject *self, PyObject *ign) {
    return PyBool_FromLong(SDL_HasClipboardText());
}
