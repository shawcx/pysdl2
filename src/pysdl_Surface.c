#include "pysdl.h"

static int        PySDL_Surface_Type_init    (PySDL_Surface*, PyObject*, PyObject*);
static void       PySDL_Surface_Type_dealloc (PySDL_Surface*);

PyObject * PySDL_Surface_getter ( PyObject * self, void * param );
int        PySDL_Surface_setter ( PyObject * self, PyObject * value, void * param );

static PyObject * PySDL_Surface_LockSurface   (PySDL_Surface*, PyObject*);
static PyObject * PySDL_Surface_SaveBMP       (PySDL_Surface*, PyObject*);
static PyObject * PySDL_Surface_UnlockSurface (PySDL_Surface*, PyObject*);

static PyMethodDef PySDL_Surface_methods[] = {
    { "LockSurface",   (PyCFunction)PySDL_Surface_LockSurface,   METH_NOARGS  },
    { "SaveBMP",       (PyCFunction)PySDL_Surface_SaveBMP,       METH_O,      },
    { "UnlockSurface", (PyCFunction)PySDL_Surface_UnlockSurface, METH_NOARGS  },
    { NULL }
};

#define PySDL_SURFACE_WIDTH  0
#define PySDL_SURFACE_HEIGHT 1
#define PySDL_SURFACE_BPP    2
#define PySDL_SURFACE_FORMAT 3
#define PySDL_SURFACE_PIXELS 4

static PyGetSetDef PySDL_Surface_getset[] = {
    { "w",      PySDL_Surface_getter, NULL,                 "", (void*)PySDL_SURFACE_WIDTH  },
    { "h",      PySDL_Surface_getter, NULL,                 "", (void*)PySDL_SURFACE_HEIGHT },
    { "bpp",    PySDL_Surface_getter, NULL,                 "", (void*)PySDL_SURFACE_BPP    },
    { "format", PySDL_Surface_getter, NULL,                 "", (void*)PySDL_SURFACE_FORMAT },
    { "pixels", PySDL_Surface_getter, PySDL_Surface_setter, "", (void*)PySDL_SURFACE_PIXELS },
    { NULL }
};

PyTypeObject PySDL_Surface_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name      = "SDL2.Surface",
    .tp_basicsize = sizeof(PySDL_Surface),
    .tp_dealloc   = (destructor)PySDL_Surface_Type_dealloc,
    .tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc       = "SDL2.Surface Class",
    .tp_methods   = PySDL_Surface_methods,
    .tp_getset    = PySDL_Surface_getset,
    .tp_init      = (initproc)PySDL_Surface_Type_init,
    .tp_new       = PyType_GenericNew
};


static int PySDL_Surface_Type_init(PySDL_Surface *self, PyObject *args, PyObject *kwds) {
    self->surface = NULL;
    self->shouldFree = 1;
    return 0;
}


static void PySDL_Surface_Type_dealloc(PySDL_Surface *self) {
    if(NULL != self->surface) {
        if(self->shouldFree) {
            SDL_FreeSurface(self->surface);
        }
        self->surface = NULL;
    }
    Py_TYPE(self)->tp_free((PyObject*)self);
}


PyObject * PySDL_Surface_getter(PyObject *self, void *param) {
    PySDL_Surface *pysdl_Surface = (PySDL_Surface *)self;
    long field;
    int size;

    field = (long)param;

    switch(field) {
    case PySDL_SURFACE_WIDTH:
        return PyLong_FromLong(pysdl_Surface->surface->w);
    case PySDL_SURFACE_HEIGHT:
        return PyLong_FromLong(pysdl_Surface->surface->h);
    case PySDL_SURFACE_BPP:
        return PyLong_FromLong(pysdl_Surface->surface->format->BitsPerPixel);
    case PySDL_SURFACE_FORMAT:
        return PyLong_FromLong(pysdl_Surface->surface->format->format);
    case PySDL_SURFACE_PIXELS:
        size = pysdl_Surface->surface->pitch * pysdl_Surface->surface->h;
        return PyBytes_FromStringAndSize(pysdl_Surface->surface->pixels, size);
    }

    return NULL;
}


int PySDL_Surface_setter(PyObject *self, PyObject *value, void *param) {
    PySDL_Surface *pysdl_Surface = (PySDL_Surface *)self;

    char *buff;
    Py_ssize_t len;
    long field;
    int size;

    field = (long)param;

    switch(field) {
    case PySDL_SURFACE_PIXELS:
        PyBytes_AsStringAndSize(value, &buff, &len);
        SDL_LockSurface(pysdl_Surface->surface);
        size = pysdl_Surface->surface->pitch * pysdl_Surface->surface->h;
        if(len > size)
            return -1;
        memcpy(pysdl_Surface->surface->pixels, buff, len);
        SDL_UnlockSurface(pysdl_Surface->surface);
        break;
    }

    return 0;
}


static PyObject * PySDL_Surface_LockSurface(PySDL_Surface *self, PyObject *args) {
    SDL_LockSurface(self->surface);
    Py_RETURN_NONE;
}


static PyObject * PySDL_Surface_SaveBMP(PySDL_Surface *self, PyObject *args) {
    int ok;
    ok = SDL_SaveBMP(self->surface, PyUnicode_AsUTF8(args));
    if(0 > ok) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyObject * PySDL_Surface_UnlockSurface(PySDL_Surface *self, PyObject *args) {
    SDL_UnlockSurface(self->surface);
    Py_RETURN_NONE;
}
