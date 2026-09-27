#include "pysdl.h"

// SDL_image: IMG_Init / IMG_Quit, LoadImage (optionally typed), the IMG_is*
// format probes, sized SVG, XPM-from-array, and animations. Sources are a path
// (str / os.PathLike) or the file's bytes, per PySDL_RWFromObject. New module
// functions keep the IMG_ prefix (as IMG_Init always has); LoadImage predates
// that. Saving to PNG/JPG lives on Surface, texture loading on Renderer.
// Registered in PyInit_SDL2 via PyModule_AddFunctions(module, pysdl_image_methods).

static PyObject * _raise(void) {
    PyErr_SetString(pysdl_Error, IMG_GetError());
    return NULL;
}

// Wrap a surface SDL_image allocated as an owned SDL2.Surface; frees it on failure.
static PyObject * _wrap_surface(SDL_Surface *surface) {
    if(NULL == surface) {
        return _raise();
    }
    PySDL_Surface *wrapper = (PySDL_Surface *)PySDL_New(&PySDL_Surface_Type);
    if(NULL == wrapper) {
        SDL_FreeSurface(surface);
        return NULL;
    }
    wrapper->surface = surface;
    return (PyObject *)wrapper;
}

//=========================================================
// init / version
//=========================================================

static PyObject * PySDL_IMG_Init(PyObject *self, PyObject *args) {
    int flags = IMG_INIT_JPG | IMG_INIT_PNG;
    if(!PyArg_ParseTuple(args, "|i", &flags)) {
        return NULL;
    }
    int got = IMG_Init(flags);
    if(flags && (got & flags) != flags) {
        return _raise();
    }
    return PyLong_FromLong(got);
}

static PyObject * PySDL_IMG_Quit(PyObject *self, PyObject *ign) {
    IMG_Quit();
    Py_RETURN_NONE;
}

// -> (major, minor, patch) of the SDL_image library actually loaded.
static PyObject * PySDL_IMG_Linked_Version(PyObject *self, PyObject *ign) {
    const SDL_version *v = IMG_Linked_Version();
    return Py_BuildValue("(iii)", v->major, v->minor, v->patch);
}

//=========================================================
// loading
//=========================================================

// LoadImage(src, type=None) -> Surface. Formats with a magic number are always
// detected from the data; `type` ("TGA") only matters for magic-less ones. That
// makes this equivalent to the per-format IMG_Load<FMT>_RW functions. A path
// with no type goes through IMG_Load so its extension can still hint the format.
static PyObject * PySDL_LoadImage(PyObject *self, PyObject *args, PyObject *kwds) {
    PyObject *src;
    const char *type = NULL;
    static char *kwlist[] = {"src", "type", NULL};
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "O|z", kwlist, &src, &type)) {
        return NULL;
    }

    SDL_Surface *surface = NULL;
    if(NULL == type && PyUnicode_Check(src)) {
        const char *path = PyUnicode_AsUTF8(src);
        if(NULL == path) {
            return NULL;
        }
        Py_BEGIN_ALLOW_THREADS
            surface = IMG_Load(path);
        Py_END_ALLOW_THREADS
        return _wrap_surface(surface);
    }

    Py_buffer view;
    SDL_RWops *rw = PySDL_RWFromObject(src, &view);
    if(NULL == rw) {
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
        surface = IMG_LoadTyped_RW(rw, 1, type);
    Py_END_ALLOW_THREADS
    PyBuffer_Release(&view);
    return _wrap_surface(surface);
}

#if SDL_IMAGE_VERSION_ATLEAST(2,6,0)
// IMG_LoadSizedSVG(src, width, height) -> Surface rasterised at that size (a 0
// dimension keeps the aspect ratio).
static PyObject * PySDL_IMG_LoadSizedSVG(PyObject *self, PyObject *args) {
    PyObject *src;
    int width, height;
    if(!PyArg_ParseTuple(args, "Oii", &src, &width, &height)) {
        return NULL;
    }
    Py_buffer view;
    SDL_RWops *rw = PySDL_RWFromObject(src, &view);
    if(NULL == rw) {
        return NULL;
    }
    SDL_Surface *surface;
    Py_BEGIN_ALLOW_THREADS
        surface = IMG_LoadSizedSVG_RW(rw, width, height);  // does not close rw
    Py_END_ALLOW_THREADS
    SDL_RWclose(rw);
    PyBuffer_Release(&view);
    return _wrap_surface(surface);
}
#endif

// Collect a sequence of str into a NULL-terminated char* array for the XPM
// readers. The pointers borrow from `fast`, which the caller must keep alive.
static char ** _xpm_lines(PyObject *fast) {
    Py_ssize_t count = PySequence_Fast_GET_SIZE(fast);
    char **lines = PyMem_Calloc(count + 1, sizeof(char *));
    if(NULL == lines) {
        PyErr_NoMemory();
        return NULL;
    }
    for(Py_ssize_t idx = 0; idx < count; ++idx) {
        lines[idx] = (char *)PyUnicode_AsUTF8(PySequence_Fast_GET_ITEM(fast, idx));
        if(NULL == lines[idx]) {
            PyMem_Free(lines);
            return NULL;
        }
    }
    return lines;
}

static PyObject * _read_xpm(PyObject *arg, int rgb888) {
    PyObject *fast = PySequence_Fast(arg, "expected a list of XPM lines (str)");
    if(NULL == fast) {
        return NULL;
    }
    char **lines = _xpm_lines(fast);
    if(NULL == lines) {
        Py_DECREF(fast);
        return NULL;
    }
    SDL_Surface *surface;
#if SDL_IMAGE_VERSION_ATLEAST(2,6,0)
    surface = rgb888 ? IMG_ReadXPMFromArrayToRGB888(lines) : IMG_ReadXPMFromArray(lines);
#else
    surface = IMG_ReadXPMFromArray(lines);
#endif
    PyMem_Free(lines);
    Py_DECREF(fast);
    return _wrap_surface(surface);
}

// IMG_ReadXPMFromArray(lines) -> Surface (8-bit paletted or 32-bit)
static PyObject * PySDL_IMG_ReadXPMFromArray(PyObject *self, PyObject *arg) {
    return _read_xpm(arg, 0);
}

#if SDL_IMAGE_VERSION_ATLEAST(2,6,0)
// IMG_ReadXPMFromArrayToRGB888(lines) -> Surface (always 32-bit RGB888)
static PyObject * PySDL_IMG_ReadXPMFromArrayToRGB888(PyObject *self, PyObject *arg) {
    return _read_xpm(arg, 1);
}
#endif

//=========================================================
// animation
//=========================================================

#if SDL_IMAGE_VERSION_ATLEAST(2,6,0)
// IMG_LoadAnimation(src, type=None) -> (w, h, [(Surface, delay_ms), ...])
// Each frame becomes an owned Surface (taken out of the IMG_Animation before
// IMG_FreeAnimation runs), so there is no separate animation type to free.
// GIF / WEBP are detected from the data (`type` is passed through for parity),
// which covers IMG_LoadGIFAnimation_RW / IMG_LoadWEBPAnimation_RW.
static PyObject * PySDL_IMG_LoadAnimation(PyObject *self, PyObject *args, PyObject *kwds) {
    PyObject *src;
    const char *type = NULL;
    static char *kwlist[] = {"src", "type", NULL};
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "O|z", kwlist, &src, &type)) {
        return NULL;
    }

    IMG_Animation *anim = NULL;
    if(NULL == type && PyUnicode_Check(src)) {
        const char *path = PyUnicode_AsUTF8(src);
        if(NULL == path) {
            return NULL;
        }
        Py_BEGIN_ALLOW_THREADS
            anim = IMG_LoadAnimation(path);
        Py_END_ALLOW_THREADS
    } else {
        Py_buffer view;
        SDL_RWops *rw = PySDL_RWFromObject(src, &view);
        if(NULL == rw) {
            return NULL;
        }
        Py_BEGIN_ALLOW_THREADS
            anim = IMG_LoadAnimationTyped_RW(rw, 1, type);
        Py_END_ALLOW_THREADS
        PyBuffer_Release(&view);
    }
    if(NULL == anim) {
        return _raise();
    }

    PyObject *frames = PyList_New(anim->count);
    if(NULL == frames) {
        IMG_FreeAnimation(anim);
        return NULL;
    }
    for(int idx = 0; idx < anim->count; ++idx) {
        PySDL_Surface *wrapper = (PySDL_Surface *)PySDL_New(&PySDL_Surface_Type);
        if(NULL == wrapper) {
            Py_DECREF(frames);
            IMG_FreeAnimation(anim);
            return NULL;
        }
        wrapper->surface = anim->frames[idx];
        anim->frames[idx] = NULL;  // now owned by the wrapper
        PyObject *item = Py_BuildValue("(Ni)", (PyObject *)wrapper, anim->delays[idx]);
        if(NULL == item) {
            Py_DECREF(frames);
            IMG_FreeAnimation(anim);
            return NULL;
        }
        PyList_SET_ITEM(frames, idx, item);
    }

    PyObject *result = Py_BuildValue("(iiN)", anim->w, anim->h, frames);
    IMG_FreeAnimation(anim);  // frees the (now NULL) frame slots and arrays
    return result;
}
#endif

//=========================================================
// format probes: IMG_is<FMT>(src) -> bool
//=========================================================

static PyObject * _is(PyObject *src, int (SDLCALL *probe)(SDL_RWops *)) {
    Py_buffer view;
    SDL_RWops *rw = PySDL_RWFromObject(src, &view);
    if(NULL == rw) {
        return NULL;
    }
    int yes = probe(rw);
    SDL_RWclose(rw);
    PyBuffer_Release(&view);
    return PyBool_FromLong(yes);
}

#define IMG_IS(FMT) \
    static PyObject * PySDL_IMG_is##FMT(PyObject *self, PyObject *src) { \
        return _is(src, IMG_is##FMT); \
    }

IMG_IS(BMP)
IMG_IS(CUR)
IMG_IS(GIF)
IMG_IS(ICO)
IMG_IS(JPG)
IMG_IS(LBM)
IMG_IS(PCX)
IMG_IS(PNG)
IMG_IS(PNM)
IMG_IS(TIF)
IMG_IS(WEBP)
IMG_IS(XCF)
IMG_IS(XPM)
IMG_IS(XV)
#if SDL_IMAGE_VERSION_ATLEAST(2,0,2)
IMG_IS(SVG)
#endif
#if SDL_IMAGE_VERSION_ATLEAST(2,6,0)
IMG_IS(AVIF)
IMG_IS(JXL)
IMG_IS(QOI)
#endif

#define IMG_IS_ENTRY(FMT) { "IMG_is" #FMT, PySDL_IMG_is##FMT, METH_O }

PyMethodDef pysdl_image_methods[] = {
    { "IMG_Init",             PySDL_IMG_Init,             METH_VARARGS },
    { "IMG_Quit",             PySDL_IMG_Quit,             METH_NOARGS  },
    { "IMG_Linked_Version",   PySDL_IMG_Linked_Version,   METH_NOARGS  },
    { "LoadImage",            (PyCFunction)PySDL_LoadImage, METH_VARARGS | METH_KEYWORDS },
    { "IMG_ReadXPMFromArray", PySDL_IMG_ReadXPMFromArray, METH_O       },
#if SDL_IMAGE_VERSION_ATLEAST(2,6,0)
    { "IMG_ReadXPMFromArrayToRGB888", PySDL_IMG_ReadXPMFromArrayToRGB888, METH_O },
    { "IMG_LoadSizedSVG",     PySDL_IMG_LoadSizedSVG,     METH_VARARGS },
    { "IMG_LoadAnimation",    (PyCFunction)PySDL_IMG_LoadAnimation, METH_VARARGS | METH_KEYWORDS },
#endif
    IMG_IS_ENTRY(BMP),
    IMG_IS_ENTRY(CUR),
    IMG_IS_ENTRY(GIF),
    IMG_IS_ENTRY(ICO),
    IMG_IS_ENTRY(JPG),
    IMG_IS_ENTRY(LBM),
    IMG_IS_ENTRY(PCX),
    IMG_IS_ENTRY(PNG),
    IMG_IS_ENTRY(PNM),
    IMG_IS_ENTRY(TIF),
    IMG_IS_ENTRY(WEBP),
    IMG_IS_ENTRY(XCF),
    IMG_IS_ENTRY(XPM),
    IMG_IS_ENTRY(XV),
#if SDL_IMAGE_VERSION_ATLEAST(2,0,2)
    IMG_IS_ENTRY(SVG),
#endif
#if SDL_IMAGE_VERSION_ATLEAST(2,6,0)
    IMG_IS_ENTRY(AVIF),
    IMG_IS_ENTRY(JXL),
    IMG_IS_ENTRY(QOI),
#endif
    { NULL }
};
