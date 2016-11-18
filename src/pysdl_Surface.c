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

#define PySDL__SURFACE_WIDTH  0
#define PySDL__SURFACE_HEIGHT 1
#define PySDL__SURFACE_BPP    2
#define PySDL__SURFACE_PIXELS 3

static PyGetSetDef PySDL_Surface_getset[] = {
    { "w",      PySDL_Surface_getter, NULL, "", (void*)PySDL__SURFACE_WIDTH  },
    { "h",      PySDL_Surface_getter, NULL, "", (void*)PySDL__SURFACE_HEIGHT },
    { "bpp",    PySDL_Surface_getter, NULL, "", (void*)PySDL__SURFACE_BPP    },
    { "pixels", PySDL_Surface_getter, PySDL_Surface_setter, "", (void*)PySDL__SURFACE_PIXELS },
    { NULL }
};

PyTypeObject PySDL_Surface_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pysdl.Surface",                      // name
    sizeof(PySDL_Surface),                 // basicsize
    0,                                 // itemsize
    (destructor)PySDL_Surface_Type_dealloc, // dealloc
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
    "pysdl.Surface Class",                // doc
    NULL,                              // traverse
    NULL,                              // clear
    NULL,                              // richcompare
    0,                                 // weaklistoffset
    NULL,                              // iter
    NULL,                              // iternext
    PySDL_Surface_methods,                 // methods
    NULL,                              // members
    PySDL_Surface_getset,                  // getset
    NULL,                              // base
    NULL,                              // dict
    NULL,                              // descr_get
    NULL,                              // descr_set
    0,                                 // dictoffset
    (initproc)PySDL_Surface_Type_init,     // init
    NULL,                              // alloc
    PyType_GenericNew
};


static int PySDL_Surface_Type_init(PySDL_Surface *self, PyObject *args, PyObject *kwds) {
    self->surface = NULL;
    return 0;
}


static void PySDL_Surface_Type_dealloc(PySDL_Surface *self) {
    if(NULL != self->surface) {
        SDL_FreeSurface(self->surface);
        self->surface = NULL;
    }
    Py_TYPE(self)->tp_free((PyObject*)self);
}


PyObject * PySDL_Surface_getter(PyObject *self, void *param) {
    PySDL_Surface *pysdl_Surface = (PySDL_Surface *)self;
    int field;
    int size;

    field = (int)param;

    switch(field)
    {
    case PySDL__SURFACE_WIDTH:
        return PyLong_FromLong(pysdl_Surface->surface->w);
    case PySDL__SURFACE_HEIGHT:
        return PyLong_FromLong(pysdl_Surface->surface->h);
    case PySDL__SURFACE_BPP:
        return PyLong_FromLong(pysdl_Surface->surface->format->BitsPerPixel);
    case PySDL__SURFACE_PIXELS:
        size = pysdl_Surface->surface->pitch * pysdl_Surface->surface->h;
        return PyBytes_FromStringAndSize(pysdl_Surface->surface->pixels, size);
    }

    return NULL;
}


int PySDL_Surface_setter(PyObject *self, PyObject *value, void *param) {
    PySDL_Surface *pysdl_Surface = (PySDL_Surface *)self;

    char *buff;
    Py_ssize_t len;
    int size;
    int field;

    field = (int)param;

    switch(field) {
    case PySDL__SURFACE_PIXELS:
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
    ok = SDL_SaveBMP(self->surface, PY_TOSTR(args));
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
