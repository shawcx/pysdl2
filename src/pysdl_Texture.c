#include "pysdl.h"

static int        PySDL_Texture_Type_init    (PySDL_Texture*, PyObject*, PyObject*);
static void       PySDL_Texture_Type_dealloc (PySDL_Texture* );

PyObject * PySDL_Texture_getter (PyObject*, void*);
int        PySDL_Texture_setter (PyObject*, PyObject*, void*);

static PyObject * PySDL_Texture_LockTexture    (PySDL_Texture*, PyObject*);
static PyObject * PySDL_Texture_QueryTexture   (PySDL_Texture*, PyObject*);
static PyObject * PySDL_Texture_UnlockTexture  (PySDL_Texture*, PyObject*);
static PyObject * PySDL_Texture_GL_BindTexture (PySDL_Texture*, PyObject*);
static PyObject * PySDL_Texture_GL_UnbindTexture (PySDL_Texture*, PyObject*);

static PyMethodDef PySDL_Texture_methods[] = {
    { "LockTexture",      (PyCFunction)PySDL_Texture_LockTexture,      METH_NOARGS },
    { "QueryTexture",     (PyCFunction)PySDL_Texture_QueryTexture,     METH_NOARGS },
    { "UnlockTexture",    (PyCFunction)PySDL_Texture_UnlockTexture,    METH_NOARGS },
    { "GL_BindTexture",   (PyCFunction)PySDL_Texture_GL_BindTexture,   METH_NOARGS },
    { "GL_UnbindTexture", (PyCFunction)PySDL_Texture_GL_UnbindTexture, METH_NOARGS },
    { NULL }
};


PyTypeObject PySDL_Texture_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name      = "SDL2.Texture",
    .tp_basicsize = sizeof(PySDL_Texture),
    .tp_dealloc   = (destructor)PySDL_Texture_Type_dealloc,
    .tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc       = "SDL2.Texture Class",
    .tp_methods   = PySDL_Texture_methods,
    .tp_init      = (initproc)PySDL_Texture_Type_init,
    .tp_new       = PyType_GenericNew
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
    uint32_t format = 0;
    int access = 0;
    int width  = 0;
    int height = 0;

    int ok = SDL_QueryTexture(self->texture, &format, &access, &width, &height);
    if(0 > ok) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }

    PyObject *result = PyTuple_New(4);
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


static PyObject * PySDL_Texture_GL_BindTexture(PySDL_Texture *self, PyObject *args) {
    float w;
    float h;
    int ok = SDL_GL_BindTexture(self->texture, &w, &h);
    if(0 > ok) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }
    // TODO: return floats
    Py_RETURN_NONE;
}


static PyObject * PySDL_Texture_GL_UnbindTexture(PySDL_Texture *self, PyObject *args) {
    int ok = SDL_GL_UnbindTexture(self->texture);
    if(0 > ok) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }
    Py_RETURN_NONE;
}
