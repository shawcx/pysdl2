#include "pysdl.h"

static int        PySDL_Renderer_Type_init    (PySDL_Renderer*, PyObject*, PyObject*);
static void       PySDL_Renderer_Type_dealloc (PySDL_Renderer*);

static PyObject * PySDL_Renderer_CreateTextureFromSurface (PySDL_Renderer*, PyObject*);
static PyObject * PySDL_Renderer_RenderClear              (PySDL_Renderer*, PyObject*);
static PyObject * PySDL_Renderer_RenderCopy               (PySDL_Renderer*, PyObject*);
static PyObject * PySDL_Renderer_RenderPresent            (PySDL_Renderer*, PyObject*);
static PyObject * PySDL_Renderer_SetRenderDrawColor       (PySDL_Renderer*, PyObject*);

static PyObject * PySDL_Renderer_LoadTexture (PySDL_Renderer*,PyObject*);

static PyMethodDef PySDL_Renderer_methods[] = {
    { "CreateTextureFromSurface", (PyCFunction)PySDL_Renderer_CreateTextureFromSurface, METH_O       },
    { "RenderClear",              (PyCFunction)PySDL_Renderer_RenderClear,              METH_NOARGS  },
    { "RenderCopy",               (PyCFunction)PySDL_Renderer_RenderCopy,               METH_O       },
    { "RenderPresent",            (PyCFunction)PySDL_Renderer_RenderPresent,            METH_NOARGS  },
    { "SetRenderDrawColor",       (PyCFunction)PySDL_Renderer_SetRenderDrawColor,       METH_VARARGS },
    //
    { "LoadTexture", (PyCFunction)PySDL_Renderer_LoadTexture, METH_O },
    //
    { NULL }
};

PyTypeObject PySDL_Renderer_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "SDL2.Renderer",                   // name
    sizeof(PySDL_Renderer),            // basicsize
    0,                                 // itemsize
    (destructor)PySDL_Renderer_Type_dealloc, // dealloc
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
    "SDL2.Renderer Class",             // doc
    NULL,                              // traverse
    NULL,                              // clear
    NULL,                              // richcompare
    0,                                 // weaklistoffset
    NULL,                              // iter
    NULL,                              // iternext
    PySDL_Renderer_methods,            // methods
    NULL,                              // members
    NULL,                              // getset
    NULL,                              // base
    NULL,                              // dict
    NULL,                              // descr_get
    NULL,                              // descr_set
    0,                                 // dictoffset
    (initproc)PySDL_Renderer_Type_init,// init
    NULL,                              // alloc
    PyType_GenericNew
};


static int PySDL_Renderer_Type_init(PySDL_Renderer *self, PyObject *args, PyObject *kwds) {
    self->renderer = NULL;
    return 0;
}


static void PySDL_Renderer_Type_dealloc(PySDL_Renderer *self) {
    if(NULL != self->renderer) {
            SDL_DestroyRenderer(self->renderer);
        self->renderer = NULL;
    }
    Py_TYPE(self)->tp_free((PyObject*)self);
}


static PyObject * PySDL_Renderer_CreateTextureFromSurface(PySDL_Renderer *self, PyObject *args) {
    PySDL_Surface *pysdl_Surface = (PySDL_Surface *)args;
    PySDL_Texture *pysdl_Texture;

    pysdl_Texture = (PySDL_Texture *)PyObject_CallObject((PyObject *)&PySDL_Texture_Type, NULL);
    if(NULL == pysdl_Texture) {
        PyErr_SetString(PyExc_TypeError, "Could not create pysdl_.Texture object");
        return NULL;
    }

    pysdl_Texture->texture = SDL_CreateTextureFromSurface(self->renderer, pysdl_Surface->surface);
    if(NULL == pysdl_Texture->texture) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }

    return (PyObject *)pysdl_Texture;
}


static PyObject * PySDL_Renderer_RenderClear(PySDL_Renderer *self, PyObject *args) {
    SDL_RenderClear(self->renderer);
    Py_RETURN_NONE;
}


static PyObject * PySDL_Renderer_RenderCopy(PySDL_Renderer *self, PyObject *args) {
    PySDL_Texture *pysdl_Texture = (PySDL_Texture *)args;
    //SDL_Rect dst;
    int ok;

    //dst.x = 0;
    //dst.y = 0;
    //dst.w = 320;
    //dst.h = 240;

    ok = SDL_RenderCopy(self->renderer, pysdl_Texture->texture, NULL, NULL);
    if(0 > ok) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject * PySDL_Renderer_RenderPresent(PySDL_Renderer *self, PyObject *args) {
    SDL_RenderPresent(self->renderer);
    Py_RETURN_NONE;
}

static PyObject * PySDL_Renderer_SetRenderDrawColor(PySDL_Renderer *self, PyObject *args) {
    int r = 0;
    int g = 0;
    int b = 0;
    int a = 255;
    int ok;

    ok = PyArg_ParseTuple(args, "bbb|b", &r, &g, &b, &a);
    if(!ok) {
        PyErr_SetString(PyExc_TypeError, "Integers between 0-255 expected");
        return NULL;
    }

    SDL_SetRenderDrawColor(self->renderer, r, g, b, a);

    Py_RETURN_NONE;
}


static PyObject * PySDL_Renderer_LoadTexture(PySDL_Renderer *self, PyObject *args) {
    PySDL_Texture *pysdl_Texture;

    pysdl_Texture = (PySDL_Texture *)PyObject_CallObject((PyObject *)&PySDL_Texture_Type, NULL);
    if(NULL == pysdl_Texture) {
        PyErr_SetString(PyExc_TypeError, "Could not create pysdl_.Texture object");
        return NULL;
    }

    pysdl_Texture->texture = IMG_LoadTexture(self->renderer, PyUnicode_AsUTF8(args));
    if(NULL == pysdl_Texture->texture) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }

    return (PyObject *)pysdl_Texture;
}
