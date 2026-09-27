#include "pysdl.h"

// Module-level video functions: extra display queries, message boxes, hints,
// misc (OpenURL / locales), the GL and Vulkan loader entry points, and the
// borrowed-window lookups. Registered via PyModule_AddFunctions.

static PyObject * _raise(void) {
    PyErr_SetString(pysdl_Error, SDL_GetError());
    return NULL;
}

//=========================================================
// displays
//=========================================================

static PyObject * PySDL_GetDisplayName(PyObject *self, PyObject *arg) {
    long display = PyLong_AsLong(arg);
    if(-1 == display && PyErr_Occurred()) {
        return NULL;
    }
    const char *name = SDL_GetDisplayName((int)display);
    if(NULL == name) {
        return _raise();
    }
    return PyUnicode_FromString(name);
}

static PyObject * PySDL_GetDisplayUsableBounds(PyObject *self, PyObject *arg) {
    long display = PyLong_AsLong(arg);
    if(-1 == display && PyErr_Occurred()) {
        return NULL;
    }
    SDL_Rect rect;
    if(0 > SDL_GetDisplayUsableBounds((int)display, &rect)) {
        return _raise();
    }
    return Py_BuildValue("(iiii)", rect.x, rect.y, rect.w, rect.h);
}

static PyObject * PySDL_GetDisplayOrientation(PyObject *self, PyObject *arg) {
    long display = PyLong_AsLong(arg);
    if(-1 == display && PyErr_Occurred()) {
        return NULL;
    }
    return PyLong_FromLong(SDL_GetDisplayOrientation((int)display));
}

static PyObject * PySDL_GetNumDisplayModes(PyObject *self, PyObject *arg) {
    long display = PyLong_AsLong(arg);
    if(-1 == display && PyErr_Occurred()) {
        return NULL;
    }
    int count = SDL_GetNumDisplayModes((int)display);
    if(0 > count) {
        return _raise();
    }
    return PyLong_FromLong(count);
}

static PyObject * PySDL_GetClosestDisplayMode(PyObject *self, PyObject *args) {
    int display;
    SDL_DisplayMode want;
    SDL_memset(&want, 0, sizeof(want));
    if(!PyArg_ParseTuple(args, "i(iiii)", &display, &want.format, &want.w, &want.h, &want.refresh_rate)) {
        return NULL;
    }
    SDL_DisplayMode closest;
    if(NULL == SDL_GetClosestDisplayMode(display, &want, &closest)) {
        Py_RETURN_NONE;
    }
    return Py_BuildValue("(iiii)", closest.format, closest.w, closest.h, closest.refresh_rate);
}

#if SDL_VERSION_ATLEAST(2,24,0)
static PyObject * PySDL_GetPointDisplayIndex(PyObject *self, PyObject *arg) {
    SDL_Point point;
    if(!PyToPoint(arg, &point)) {
        return NULL;
    }
    int index = SDL_GetPointDisplayIndex(&point);
    if(0 > index) {
        return _raise();
    }
    return PyLong_FromLong(index);
}

static PyObject * PySDL_GetRectDisplayIndex(PyObject *self, PyObject *arg) {
    SDL_Rect rect;
    if(!PyToRect(arg, &rect)) {
        return NULL;
    }
    int index = SDL_GetRectDisplayIndex(&rect);
    if(0 > index) {
        return _raise();
    }
    return PyLong_FromLong(index);
}
#endif

//=========================================================
// borrowed-window lookups
//=========================================================

static PyObject * PySDL_GetWindowFromID(PyObject *self, PyObject *arg) {
    unsigned long id = PyLong_AsUnsignedLong(arg);
    if((unsigned long)-1 == id && PyErr_Occurred()) {
        return NULL;
    }
    return PySDL_WrapWindow(SDL_GetWindowFromID((Uint32)id));
}

#if SDL_VERSION_ATLEAST(2,0,16)
static PyObject * PySDL_GetGrabbedWindow(PyObject *self, PyObject *ign) {
    return PySDL_WrapWindow(SDL_GetGrabbedWindow());
}
#endif

//=========================================================
// message boxes
//=========================================================

static PyObject * PySDL_ShowSimpleMessageBox(PyObject *self, PyObject *args, PyObject *kwds) {
    unsigned int flags = SDL_MESSAGEBOX_INFORMATION;
    const char *title;
    const char *message;
    PyObject *window_py = Py_None;

    static char *kwlist[] = {"title", "message", "flags", "window", NULL};
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "ss|IO", kwlist,
        &title, &message, &flags, &window_py)) {
        return NULL;
    }

    SDL_Window *window = NULL;
    if(window_py != Py_None) {
        if(!PyObject_TypeCheck(window_py, &PySDL_Window_Type)) {
            PyErr_SetString(PyExc_TypeError, "window must be an SDL2.Window or None");
            return NULL;
        }
        window = ((PySDL_Window *)window_py)->window;
    }

    if(0 > SDL_ShowSimpleMessageBox(flags, title, message, window)) {
        return _raise();
    }
    Py_RETURN_NONE;
}

static PyObject * PySDL_ShowMessageBox(PyObject *self, PyObject *arg) {
    if(!PyDict_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "expected a dict {title, message, buttons, flags, window}");
        return NULL;
    }

    SDL_MessageBoxData data;
    SDL_memset(&data, 0, sizeof(data));

    PyObject *v;
    v = PyDict_GetItemString(arg, "flags");
    data.flags = v ? (Uint32)PyLong_AsUnsignedLong(v) : SDL_MESSAGEBOX_INFORMATION;
    v = PyDict_GetItemString(arg, "title");
    data.title = v ? PyUnicode_AsUTF8(v) : "";
    v = PyDict_GetItemString(arg, "message");
    data.message = v ? PyUnicode_AsUTF8(v) : "";
    if(PyErr_Occurred()) {
        return NULL;
    }

    PyObject *window_py = PyDict_GetItemString(arg, "window");
    if(window_py && window_py != Py_None) {
        if(!PyObject_TypeCheck(window_py, &PySDL_Window_Type)) {
            PyErr_SetString(PyExc_TypeError, "window must be an SDL2.Window or None");
            return NULL;
        }
        data.window = ((PySDL_Window *)window_py)->window;
    }

    PyObject *buttons_py = PyDict_GetItemString(arg, "buttons");
    if(NULL == buttons_py) {
        PyErr_SetString(PyExc_KeyError, "message box needs a 'buttons' list of (id, text[, flags])");
        return NULL;
    }
    PyObject *fast = PySequence_Fast(buttons_py, "'buttons' must be a list");
    if(NULL == fast) {
        return NULL;
    }
    Py_ssize_t nbuttons = PySequence_Fast_GET_SIZE(fast);
    SDL_MessageBoxButtonData *buttons = PyMem_New(SDL_MessageBoxButtonData, nbuttons > 0 ? nbuttons : 1);
    if(NULL == buttons) {
        Py_DECREF(fast);
        return PyErr_NoMemory();
    }
    for(Py_ssize_t idx = 0; idx < nbuttons; ++idx) {
        PyObject *b = PySequence_Fast_GET_ITEM(fast, idx);
        int bid = 0;
        const char *text = "";
        unsigned int bflags = 0;
        if(!PyArg_ParseTuple(b, "is|I", &bid, &text, &bflags)) {
            PyMem_Free(buttons);
            Py_DECREF(fast);
            return NULL;
        }
        buttons[idx].buttonid = bid;
        buttons[idx].text = text;
        buttons[idx].flags = bflags;
    }
    data.numbuttons = (int)nbuttons;
    data.buttons = buttons;

    int result = -1;
    int rc = SDL_ShowMessageBox(&data, &result);
    PyMem_Free(buttons);
    Py_DECREF(fast);
    if(0 > rc) {
        return _raise();
    }
    return PyLong_FromLong(result);
}

//=========================================================
// hints
//=========================================================

static PyObject * PySDL_SetHint(PyObject *self, PyObject *args) {
    const char *name;
    const char *value;
    if(!PyArg_ParseTuple(args, "ss", &name, &value)) {
        return NULL;
    }
    return PyBool_FromLong(SDL_SetHint(name, value));
}

static PyObject * PySDL_SetHintWithPriority(PyObject *self, PyObject *args) {
    const char *name;
    const char *value;
    int priority;
    if(!PyArg_ParseTuple(args, "ssi", &name, &value, &priority)) {
        return NULL;
    }
    return PyBool_FromLong(SDL_SetHintWithPriority(name, value, (SDL_HintPriority)priority));
}

static PyObject * PySDL_GetHint(PyObject *self, PyObject *arg) {
    const char *name = PyUnicode_AsUTF8(arg);
    if(NULL == name) {
        return NULL;
    }
    const char *value = SDL_GetHint(name);
    if(NULL == value) {
        Py_RETURN_NONE;
    }
    return PyUnicode_FromString(value);
}

static PyObject * PySDL_GetHintBoolean(PyObject *self, PyObject *args) {
    const char *name;
    int default_value = 0;
    if(!PyArg_ParseTuple(args, "s|p", &name, &default_value)) {
        return NULL;
    }
    return PyBool_FromLong(SDL_GetHintBoolean(name, default_value ? SDL_TRUE : SDL_FALSE));
}

#if SDL_VERSION_ATLEAST(2,24,0)
static PyObject * PySDL_ResetHint(PyObject *self, PyObject *arg) {
    const char *name = PyUnicode_AsUTF8(arg);
    if(NULL == name) {
        return NULL;
    }
    return PyBool_FromLong(SDL_ResetHint(name));
}
#endif

//---------------------------------------------------------
// hint callbacks
//---------------------------------------------------------

// Each registration is a (name, callable) tuple, used as the SDL userdata and
// kept alive in this list until DelHintCallback / ClearHints.
static PyObject *_hint_callbacks = NULL;

static void SDLCALL _c_hint_callback(void *userdata, const char *name, const char *old, const char *new_) {
    PyGILState_STATE gil;
    if(!PySDL_ThreadEnter(&gil)) {
        return;
    }
    // Hold the registration: the callback may DelHintCallback itself.
    PyObject *entry = Py_NewRef((PyObject *)userdata);
    PyObject *result = PyObject_CallFunction(PyTuple_GET_ITEM(entry, 1), "szz", name, old, new_);
    if(NULL == result) {
        PyErr_Print();
    }
    Py_XDECREF(result);
    Py_DECREF(entry);
    PySDL_ThreadLeave(gil);
}

// Index of the (name, callable) registration, matched by name and identity.
static Py_ssize_t _hint_index(const char *name, PyObject *callable) {
    Py_ssize_t count = _hint_callbacks ? PyList_GET_SIZE(_hint_callbacks) : 0;
    for(Py_ssize_t idx = 0; idx < count; ++idx) {
        PyObject *entry = PyList_GET_ITEM(_hint_callbacks, idx);
        const char *entry_name = PyUnicode_AsUTF8(PyTuple_GET_ITEM(entry, 0));
        if(PyTuple_GET_ITEM(entry, 1) == callable && entry_name && 0 == SDL_strcmp(entry_name, name)) {
            return idx;
        }
    }
    return -1;
}

static void _hint_remove(const char *name, Py_ssize_t idx) {
    PyObject *entry = PyList_GET_ITEM(_hint_callbacks, idx);
    SDL_DelHintCallback(name, _c_hint_callback, entry);
    PySequence_DelItem(_hint_callbacks, idx);
}

// AddHintCallback(name, callback): callback(name, old, new) runs now with the
// current value, then on every change of that hint (None when unset).
// Re-adding the same callback for the same hint replaces it, as in SDL.
static PyObject * PySDL_AddHintCallback(PyObject *self, PyObject *args) {
    const char *name;
    PyObject *callable;
    if(!PyArg_ParseTuple(args, "sO", &name, &callable)) {
        return NULL;
    }
    if(!PyCallable_Check(callable)) {
        PyErr_SetString(PyExc_TypeError, "callback must be callable");
        return NULL;
    }
    if(NULL == _hint_callbacks && NULL == (_hint_callbacks = PyList_New(0))) {
        return NULL;
    }
    Py_ssize_t idx = _hint_index(name, callable);
    if(idx >= 0) {
        _hint_remove(name, idx);
    }

    PyObject *entry = Py_BuildValue("(sO)", name, callable);
    if(NULL == entry) {
        return NULL;
    }
    int rc = PyList_Append(_hint_callbacks, entry);
    Py_DECREF(entry);  // the list holds it
    if(0 > rc) {
        return NULL;
    }
    SDL_AddHintCallback(name, _c_hint_callback, entry);  // calls it once right away
    Py_RETURN_NONE;
}

static PyObject * PySDL_DelHintCallback(PyObject *self, PyObject *args) {
    const char *name;
    PyObject *callable;
    if(!PyArg_ParseTuple(args, "sO", &name, &callable)) {
        return NULL;
    }
    Py_ssize_t idx = _hint_index(name, callable);
    if(0 > idx) {
        PyErr_Format(PyExc_ValueError, "callback is not registered for hint %s", name);
        return NULL;
    }
    _hint_remove(name, idx);
    Py_RETURN_NONE;
}

// SDL_ClearHints also frees every hint callback: drop our registrations too.
static PyObject * PySDL_ClearHints(PyObject *self, PyObject *ign) {
    SDL_ClearHints();
    if(NULL != _hint_callbacks && 0 > PyList_SetSlice(_hint_callbacks, 0, PY_SSIZE_T_MAX, NULL)) {
        return NULL;
    }
    Py_RETURN_NONE;
}

#if SDL_VERSION_ATLEAST(2,26,0)
// Reset every hint to its default (environment) value; callbacks stay and fire.
static PyObject * PySDL_ResetHints(PyObject *self, PyObject *ign) {
    SDL_ResetHints();
    Py_RETURN_NONE;
}
#endif

//=========================================================
// misc
//=========================================================

#if SDL_VERSION_ATLEAST(2,0,14)
static PyObject * PySDL_OpenURL(PyObject *self, PyObject *arg) {
    const char *url = PyUnicode_AsUTF8(arg);
    if(NULL == url) {
        return NULL;
    }
    if(0 > SDL_OpenURL(url)) {
        return _raise();
    }
    Py_RETURN_NONE;
}

static PyObject * PySDL_GetPreferredLocales(PyObject *self, PyObject *ign) {
    SDL_Locale *locales = SDL_GetPreferredLocales();
    if(NULL == locales) {
        return PyList_New(0);
    }
    PyObject *list = PyList_New(0);
    if(NULL == list) {
        SDL_free(locales);
        return NULL;
    }
    for(SDL_Locale *l = locales; l->language != NULL; ++l) {
        PyObject *country = l->country ? PyUnicode_FromString(l->country) : Py_NewRef(Py_None);
        PyObject *entry = country ? Py_BuildValue("(sN)", l->language, country) : NULL;
        if(NULL == entry || 0 > PyList_Append(list, entry)) {
            Py_XDECREF(entry);
            Py_DECREF(list);
            SDL_free(locales);
            return NULL;
        }
        Py_DECREF(entry);
    }
    SDL_free(locales);
    return list;
}
#endif

//=========================================================
// GL / Vulkan loaders
//=========================================================

static PyObject * PySDL_GL_LoadLibrary(PyObject *self, PyObject *args) {
    const char *path = NULL;
    if(!PyArg_ParseTuple(args, "|z", &path)) {
        return NULL;
    }
    if(0 > SDL_GL_LoadLibrary(path)) {
        return _raise();
    }
    Py_RETURN_NONE;
}

static PyObject * PySDL_GL_UnloadLibrary(PyObject *self, PyObject *ign) {
    SDL_GL_UnloadLibrary();
    Py_RETURN_NONE;
}

static PyObject * PySDL_GL_GetProcAddress(PyObject *self, PyObject *arg) {
    const char *name = PyUnicode_AsUTF8(arg);
    if(NULL == name) {
        return NULL;
    }
    return PyLong_FromVoidPtr(SDL_GL_GetProcAddress(name));
}

static PyObject * PySDL_GL_GetCurrentWindow(PyObject *self, PyObject *ign) {
    return PySDL_WrapWindow(SDL_GL_GetCurrentWindow());
}

static PyObject * PySDL_GL_GetCurrentContext(PyObject *self, PyObject *ign) {
    return PyLong_FromVoidPtr(SDL_GL_GetCurrentContext());
}

static PyObject * PySDL_Vulkan_LoadLibrary(PyObject *self, PyObject *args) {
    const char *path = NULL;
    if(!PyArg_ParseTuple(args, "|z", &path)) {
        return NULL;
    }
    if(0 > SDL_Vulkan_LoadLibrary(path)) {
        return _raise();
    }
    Py_RETURN_NONE;
}

static PyObject * PySDL_Vulkan_UnloadLibrary(PyObject *self, PyObject *ign) {
    SDL_Vulkan_UnloadLibrary();
    Py_RETURN_NONE;
}

static PyObject * PySDL_Vulkan_GetVkGetInstanceProcAddr(PyObject *self, PyObject *ign) {
    void *fn = SDL_Vulkan_GetVkGetInstanceProcAddr();
    if(NULL == fn) {
        return _raise();
    }
    return PyLong_FromVoidPtr(fn);
}

#if SDL_VERSION_ATLEAST(2,0,14)
static PyObject * PySDL_Metal_GetLayer(PyObject *self, PyObject *arg) {
    void *view = PyLong_AsVoidPtr(arg);
    if(NULL == view && PyErr_Occurred()) {
        return NULL;
    }
    return PyLong_FromVoidPtr(SDL_Metal_GetLayer((SDL_MetalView)view));
}

static PyObject * PySDL_Metal_DestroyView(PyObject *self, PyObject *arg) {
    void *view = PyLong_AsVoidPtr(arg);
    if(NULL == view && PyErr_Occurred()) {
        return NULL;
    }
    SDL_Metal_DestroyView((SDL_MetalView)view);
    Py_RETURN_NONE;
}
#endif

static PyObject * PySDL_Vulkan_GetInstanceExtensions(PyObject *self, PyObject *args) {
    PyObject *window_py = Py_None;
    if(!PyArg_ParseTuple(args, "|O", &window_py)) {
        return NULL;
    }

    SDL_Window *window = NULL;
    if(window_py != Py_None) {
        if(!PyObject_TypeCheck(window_py, &PySDL_Window_Type)) {
            PyErr_SetString(PyExc_TypeError, "window must be an SDL2.Window or None");
            return NULL;
        }
        window = ((PySDL_Window *)window_py)->window;
    }

    unsigned int count = 0;
    if(SDL_FALSE == SDL_Vulkan_GetInstanceExtensions(window, &count, NULL)) {
        return _raise();
    }

    const char **names = PyMem_New(const char *, count > 0 ? count : 1);
    if(NULL == names) {
        return PyErr_NoMemory();
    }
    if(SDL_FALSE == SDL_Vulkan_GetInstanceExtensions(window, &count, names)) {
        PyMem_Free(names);
        return _raise();
    }

    PyObject *list = PyList_New(count);
    if(NULL == list) {
        PyMem_Free(names);
        return NULL;
    }
    for(unsigned int idx = 0; idx < count; ++idx) {
        PyObject *s = PyUnicode_FromString(names[idx]);
        if(NULL == s) {
            Py_DECREF(list);
            PyMem_Free(names);
            return NULL;
        }
        PyList_SET_ITEM(list, idx, s);
    }
    PyMem_Free(names);
    return list;
}

//=========================================================
// video subsystem
//=========================================================

// VideoInit(driver=None): start just the video subsystem, optionally with a
// named driver (see GetVideoDrivers); VideoQuit() shuts it down.
static PyObject * PySDL_VideoInit(PyObject *self, PyObject *args) {
    const char *driver = NULL;
    if(!PyArg_ParseTuple(args, "|z", &driver)) {
        return NULL;
    }
    int rc = SDL_VideoInit(driver);
    PySDL_InvalidateWindows();  // SDL_VideoInit first shuts down any running video
    if(0 > rc) {
        return _raise();
    }
    Py_RETURN_NONE;
}

static PyObject * PySDL_VideoQuit(PyObject *self, PyObject *ign) {
    SDL_VideoQuit();
    PySDL_InvalidateWindows();
    Py_RETURN_NONE;
}

//=========================================================
// window constructors beyond SDL2.Window(...)
//=========================================================

// An owned SDL2.Window around an SDL_Window*, or raise for NULL.
static PyObject * _own_window(SDL_Window *window) {
    if(NULL == window) {
        return _raise();
    }
    PySDL_Window *wrapper = (PySDL_Window *)PySDL_New(&PySDL_Window_Type);
    if(NULL == wrapper) {
        SDL_DestroyWindow(window);
        return NULL;
    }
    wrapper->window = window;
    return (PyObject *)wrapper;
}

// CreateWindowFrom(native_handle) -> Window around an existing native window
// (an HWND / NSWindow* / X11 Window id, as an int).
static PyObject * PySDL_CreateWindowFrom(PyObject *self, PyObject *arg) {
    void *handle = PyLong_AsVoidPtr(arg);
    if(NULL == handle) {
        if(!PyErr_Occurred()) {
            PyErr_SetString(PyExc_ValueError, "native handle must be non-zero");
        }
        return NULL;
    }
    return _own_window(SDL_CreateWindowFrom(handle));
}

// CreateShapedWindow(title, size, position=(CENTERED, CENTERED), flags=0)
static PyObject * PySDL_CreateShapedWindow(PyObject *self, PyObject *args, PyObject *kwds) {
    const char *title;
    int w, h;
    int x = SDL_WINDOWPOS_CENTERED, y = SDL_WINDOWPOS_CENTERED;
    unsigned int flags = 0;
    static char *kwlist[] = {"title", "size", "position", "flags", NULL};
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "s(ii)|(ii)I", kwlist, &title, &w, &h, &x, &y, &flags)) {
        return NULL;
    }
    SDL_ClearError();
    SDL_Window *window = SDL_CreateShapedWindow(title, (unsigned)x, (unsigned)y, (unsigned)w, (unsigned)h, flags);
    if(NULL == window && '\0' == *SDL_GetError()) {
        // SDL sets no error when the video driver has no shaped-window support.
        PyErr_SetString(pysdl_Error, "shaped windows are not supported by this video driver");
        return NULL;
    }
    return _own_window(window);
}

// CreateWindowAndRenderer(size, flags=0) -> (Window, Renderer)
static PyObject * PySDL_CreateWindowAndRenderer(PyObject *self, PyObject *args, PyObject *kwds) {
    int w, h;
    unsigned int flags = 0;
    static char *kwlist[] = {"size", "flags", NULL};
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "(ii)|I", kwlist, &w, &h, &flags)) {
        return NULL;
    }

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    if(0 > SDL_CreateWindowAndRenderer(w, h, flags, &window, &renderer)) {
        return _raise();
    }

    PySDL_Renderer *rwrap = (PySDL_Renderer *)PySDL_New(&PySDL_Renderer_Type);
    if(NULL == rwrap) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        return NULL;
    }
    rwrap->renderer = renderer;
    PyObject *wwrap = _own_window(window);
    if(NULL == wwrap) {
        Py_DECREF(rwrap);  // destroys the renderer; _own_window destroyed the window
        return NULL;
    }
    return Py_BuildValue("(NN)", wwrap, (PyObject *)rwrap);
}

PyMethodDef pysdl_video_methods[] = {
    { "GetDisplayName",           PySDL_GetDisplayName,           METH_O       },
    { "GetDisplayUsableBounds",   PySDL_GetDisplayUsableBounds,   METH_O       },
    { "GetDisplayOrientation",    PySDL_GetDisplayOrientation,    METH_O       },
    { "GetNumDisplayModes",       PySDL_GetNumDisplayModes,       METH_O       },
    { "GetClosestDisplayMode",    PySDL_GetClosestDisplayMode,    METH_VARARGS },

    { "GetWindowFromID",          PySDL_GetWindowFromID,          METH_O       },
    { "VideoInit",                PySDL_VideoInit,                METH_VARARGS },
    { "VideoQuit",                PySDL_VideoQuit,                METH_NOARGS  },
    { "CreateWindowFrom",         PySDL_CreateWindowFrom,         METH_O       },
    { "CreateShapedWindow",       (PyCFunction)PySDL_CreateShapedWindow, METH_VARARGS | METH_KEYWORDS },
    { "CreateWindowAndRenderer",  (PyCFunction)PySDL_CreateWindowAndRenderer, METH_VARARGS | METH_KEYWORDS },

    { "ShowSimpleMessageBox",     (PyCFunction)PySDL_ShowSimpleMessageBox, METH_VARARGS | METH_KEYWORDS },
    { "ShowMessageBox",           PySDL_ShowMessageBox,           METH_O       },

    { "SetHint",                  PySDL_SetHint,                  METH_VARARGS },
    { "SetHintWithPriority",      PySDL_SetHintWithPriority,      METH_VARARGS },
    { "GetHint",                  PySDL_GetHint,                  METH_O       },
    { "GetHintBoolean",           PySDL_GetHintBoolean,           METH_VARARGS },
    { "ClearHints",               PySDL_ClearHints,               METH_NOARGS  },
    { "AddHintCallback",          PySDL_AddHintCallback,          METH_VARARGS },
    { "DelHintCallback",          PySDL_DelHintCallback,          METH_VARARGS },
#if SDL_VERSION_ATLEAST(2,26,0)
    { "ResetHints",               PySDL_ResetHints,               METH_NOARGS  },
#endif
#if SDL_VERSION_ATLEAST(2,0,14)
    { "Metal_GetLayer",           PySDL_Metal_GetLayer,           METH_O       },
    { "Metal_DestroyView",        PySDL_Metal_DestroyView,        METH_O       },
#endif
#if SDL_VERSION_ATLEAST(2,0,14)
    { "OpenURL",                  PySDL_OpenURL,                  METH_O       },
    { "GetPreferredLocales",      PySDL_GetPreferredLocales,      METH_NOARGS  },
#endif
#if SDL_VERSION_ATLEAST(2,0,16)
    { "GetGrabbedWindow",         PySDL_GetGrabbedWindow,         METH_NOARGS  },
#endif
#if SDL_VERSION_ATLEAST(2,24,0)
    { "GetPointDisplayIndex",     PySDL_GetPointDisplayIndex,     METH_O       },
    { "GetRectDisplayIndex",      PySDL_GetRectDisplayIndex,      METH_O       },
    { "ResetHint",                PySDL_ResetHint,                METH_O       },
#endif
    { "GL_LoadLibrary",           PySDL_GL_LoadLibrary,           METH_VARARGS },
    { "GL_UnloadLibrary",         PySDL_GL_UnloadLibrary,         METH_NOARGS  },
    { "GL_GetProcAddress",        PySDL_GL_GetProcAddress,        METH_O       },
    { "GL_GetCurrentWindow",      PySDL_GL_GetCurrentWindow,      METH_NOARGS  },
    { "GL_GetCurrentContext",     PySDL_GL_GetCurrentContext,     METH_NOARGS  },

    { "Vulkan_LoadLibrary",       PySDL_Vulkan_LoadLibrary,       METH_VARARGS },
    { "Vulkan_UnloadLibrary",     PySDL_Vulkan_UnloadLibrary,     METH_NOARGS  },
    { "Vulkan_GetVkGetInstanceProcAddr", PySDL_Vulkan_GetVkGetInstanceProcAddr, METH_NOARGS },
    { "Vulkan_GetInstanceExtensions",    PySDL_Vulkan_GetInstanceExtensions,    METH_VARARGS },

    { NULL }
};
