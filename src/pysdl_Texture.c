#include "pysdl.h"

static int        PySDL_Texture_Type_init    (PySDL_Texture*, PyObject*, PyObject*);
static void       PySDL_Texture_Type_dealloc (PySDL_Texture* );

PyObject * PySDL_Texture_getter (PyObject*, void*);
int        PySDL_Texture_setter (PyObject*, PyObject*, void*);

static PyObject * PySDL_Texture_LockTexture   (PySDL_Texture*, PyObject*);
static PyObject * PySDL_Texture_QueryTexture  (PySDL_Texture*, PyObject*);
static PyObject * PySDL_Texture_UnlockTexture (PySDL_Texture*, PyObject*);

static PyMethodDef PySDL_Texture_methods[] = {
    { "LockTexture",   (PyCFunction)PySDL_Texture_LockTexture,   METH_NOARGS  },
    { "QueryTexture",  (PyCFunction)PySDL_Texture_QueryTexture,  METH_NOARGS  },
    { "UnlockTexture", (PyCFunction)PySDL_Texture_UnlockTexture, METH_NOARGS  },
    { NULL }
};


PyTypeObject PySDL_Texture_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "SDL2.Texture",                    // name
    sizeof(PySDL_Texture),             // basicsize
    0,                                 // itemsize
    (destructor)PySDL_Texture_Type_dealloc, // dealloc
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
    "SDL2.Texture Class",              // doc
    NULL,                              // traverse
    NULL,                              // clear
    NULL,                              // richcompare
    0,                                 // weaklistoffset
    NULL,                              // iter
    NULL,                              // iternext
    PySDL_Texture_methods,             // methods
    NULL,                              // members
    NULL, //PySDL_Texture_getset,                  // getset
    NULL,                              // base
    NULL,                              // dict
    NULL,                              // descr_get
    NULL,                              // descr_set
    0,                                 // dictoffset
    (initproc)PySDL_Texture_Type_init, // init
    NULL,                              // alloc
    PyType_GenericNew
};


static int PySDL_Texture_Type_init(PySDL_Texture *self, PyObject *args, PyObject *kwds) {
    self->texture = NULL;
    return 0;
}


static void PySDL_Texture_Type_dealloc(PySDL_Texture *self) {
    if(NULL != self->texture) {
        SDL_DestroyTexture(self->texture);
        self->texture = NULL;
    }
    Py_TYPE(self)->tp_free((PyObject*)self);
}


static PyObject * PySDL_Texture_LockTexture(PySDL_Texture *self, PyObject *args) {
    void *pixels;
    int pitch;

    SDL_LockTexture(self->texture, NULL, &pixels, &pitch);
    Py_RETURN_NONE;
}


static PyObject * PySDL_Texture_QueryTexture(PySDL_Texture *self, PyObject *args) {
    PyObject *result;
    uint32_t format = 0;
    int access = 0;
    int width = 0;
    int height = 0;
    int ok;

    ok = SDL_QueryTexture(self->texture, &format, &access, &width, &height);
    if(0 > ok) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }

    result = PyTuple_New(4);
    PyTuple_SetItem(result, 0, PyLong_FromLong(format));
    PyTuple_SetItem(result, 1, PyLong_FromLong(access));
    PyTuple_SetItem(result, 2, PyLong_FromLong(width));
    PyTuple_SetItem(result, 3, PyLong_FromLong(height));
    return result;
}


static PyObject * PySDL_Texture_UnlockTexture(PySDL_Texture *self, PyObject *args) {
    SDL_UnlockTexture(self->texture);
    Py_RETURN_NONE;
}
