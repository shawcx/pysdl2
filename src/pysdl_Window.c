#include "pysdl.h"

static int        PySDL_Window_Type_init    (PySDL_Window*, PyObject*, PyObject*);
static void       PySDL_Window_Type_dealloc (PySDL_Window* );

static PyObject * PySDL_Window_CreateRenderer      (PySDL_Window*, PyObject*);
static PyObject * PySDL_Window_GetWindowSize       (PySDL_Window*, PyObject*);
static PyObject * PySDL_Window_GL_CreateContext    (PySDL_Window*, PyObject*);
static PyObject * PySDL_Window_GL_SwapWindow       (PySDL_Window*, PyObject*);
static PyObject * PySDL_Window_SetWindowFullscreen (PySDL_Window*, PyObject*);
static PyObject * PySDL_Window_SetWindowIcon       (PySDL_Window*, PyObject*);
static PyObject * PySDL_Window_SetWindowTitle      (PySDL_Window*, PyObject*);

static PyMethodDef PySDL_Window_methods[] = {
    { "CreateRenderer",      (PyCFunction)PySDL_Window_CreateRenderer,      METH_NOARGS  },
    { "GetWindowSize",       (PyCFunction)PySDL_Window_GetWindowSize,       METH_NOARGS  },
    { "GL_CreateContext",    (PyCFunction)PySDL_Window_GL_CreateContext,    METH_NOARGS  },
    { "GL_SwapWindow",       (PyCFunction)PySDL_Window_GL_SwapWindow,       METH_NOARGS  },
    { "SetWindowFullscreen", (PyCFunction)PySDL_Window_SetWindowFullscreen, METH_NOARGS  },
    { "SetWindowIcon",       (PyCFunction)PySDL_Window_SetWindowIcon,       METH_O       },
    { "SetWindowTitle",      (PyCFunction)PySDL_Window_SetWindowTitle,      METH_O       },
    { NULL }
};

PyTypeObject PySDL_Window_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pysdl.Window",                       // name
    sizeof(PySDL_Window),                  // basicsize
    0,                                 // itemsize
    (destructor)PySDL_Window_Type_dealloc, // dealloc
    NULL,                              // print
    NULL,                              // getattr
    NULL,                              // setattr
    NULL,                              // reserved
    NULL,                              // repr
    NULL,                              // as_number
    NULL,                              // as_sequence
    NULL,                              // as_mapping
    NULL,                              // hash
    NULL,                              // call
    NULL,                              // str
    NULL,                              // getattro
    NULL,                              // setattro
    NULL,                              // as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    "pysdl.Window Class",                 // doc
    NULL,                              // traverse
    NULL,                              // clear
    NULL,                              // richcompare
    0,                                 // weaklistoffset
    NULL,                              // iter
    NULL,                              // iternext
    PySDL_Window_methods,                  // methods
    NULL,                              // members
    NULL,                              // getset
    NULL,                              // base
    NULL,                              // dict
    NULL,                              // descr_get
    NULL,                              // descr_set
    0,                                 // dictoffset
    (initproc)PySDL_Window_Type_init,      // init
    NULL,                              // alloc
    PyType_GenericNew
};


static int PySDL_Window_Type_init(PySDL_Window *self, PyObject *args, PyObject *kwds) {
    char *title = NULL;
    int w = 0;
    int h = 0;
    int x = SDL_WINDOWPOS_CENTERED;
    int y = SDL_WINDOWPOS_CENTERED;
    int f = 0;
    int ok;

    static char *kwlist[] = {"title", "size", "position", "flags", NULL};

    ok = PyArg_ParseTupleAndKeywords(args, kwds, "s|(ii)(ii)I", kwlist,
        &title, &w, &h, &x, &y, &f);
    if(!ok) {
        PyErr_SetString(PyExc_TypeError, "Something went wrong");
        return -1;
    }

    self->window = SDL_CreateWindow(title, x, y, w, h, f);
    if(NULL == self->window) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return -1;
    }

    self->glContext = NULL;

    return 0;
}


static void PySDL_Window_Type_dealloc(PySDL_Window *self) {
    if(NULL != self->glContext) {
        SDL_GL_DeleteContext(self->glContext);
        self->glContext = NULL;
    }
    if(NULL != self->window) {
        SDL_DestroyWindow(self->window);
        self->window = NULL;
    }
    Py_TYPE(self)->tp_free((PyObject*)self);
}


static PyObject * PySDL_Window_CreateRenderer(PySDL_Window *self, PyObject *args) {
    PySDL_Renderer *pysdl_Renderer;

    int index = -1;
    int flags = 0;

    flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;

    pysdl_Renderer = (PySDL_Renderer *)PyObject_CallObject((PyObject *)&PySDL_Renderer_Type, NULL);
    if(NULL == pysdl_Renderer) {
        PyErr_SetString(PyExc_TypeError, "Could not create pysdl.Renderer object");
        return NULL;
    }

    pysdl_Renderer->renderer = SDL_CreateRenderer(self->window, index, flags);
    if(NULL == pysdl_Renderer->renderer) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }

    return (PyObject *)pysdl_Renderer;
}


static PyObject * PySDL_Window_GetWindowSize(PySDL_Window *self, PyObject *args) {
    PyObject *size;
    int w, h;

    SDL_GetWindowSize(self->window, &w, &h);

    size = PyTuple_New(2);
    PyTuple_SetItem(size, 0, PyLong_FromLong(w));
    PyTuple_SetItem(size, 1, PyLong_FromLong(h));

    return size;
}


static PyObject * PySDL_Window_GL_CreateContext(PySDL_Window *self, PyObject *args) {
    self->glContext = SDL_GL_CreateContext(self->window);
    Py_RETURN_NONE;
}


static PyObject * PySDL_Window_GL_SwapWindow(PySDL_Window *self, PyObject *args) {
    SDL_GL_SwapWindow(self->window);
    Py_RETURN_NONE;
}


static PyObject * PySDL_Window_SetWindowFullscreen(PySDL_Window *self, PyObject *args) {
    int flags = 0;
    int ok;

    ok = PyArg_ParseTuple(args, "|i", &flags);

    ok = SDL_SetWindowFullscreen(self->window, flags);
    if(0 > ok) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject * PySDL_Window_SetWindowIcon(PySDL_Window *self, PyObject *args) {
    PySDL_Surface *pysdl_Surface = (PySDL_Surface *)args;
    SDL_SetWindowIcon(self->window, pysdl_Surface->surface);
    Py_RETURN_NONE;
}


static PyObject * PySDL_Window_SetWindowTitle(PySDL_Window *self, PyObject *args) {
    SDL_SetWindowTitle(self->window, PY_TOSTR(args));
    Py_RETURN_NONE;
}
