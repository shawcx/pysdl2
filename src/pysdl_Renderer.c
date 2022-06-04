#include "pysdl.h"

static int        PySDL_Renderer_Type_init    (PySDL_Renderer*, PyObject*, PyObject*);
static void       PySDL_Renderer_Type_dealloc (PySDL_Renderer*);

static PyObject * PySDL_Renderer_CreateTextureFromSurface (PySDL_Renderer*, PyObject*);
static PyObject * PySDL_Renderer_Clear                    (PySDL_Renderer*, PyObject*);
static PyObject * PySDL_Renderer_Copy                     (PySDL_Renderer*, PyObject*, PyObject*);
static PyObject * PySDL_Renderer_CopyEx                   (PySDL_Renderer*, PyObject*, PyObject*);
static PyObject * PySDL_Renderer_Present                  (PySDL_Renderer*, PyObject*);
static PyObject * PySDL_Renderer_SetRenderDrawColor       (PySDL_Renderer*, PyObject*);

static PyObject * PySDL_Renderer_LoadTexture (PySDL_Renderer*,PyObject*);

static PyMethodDef PySDL_Renderer_methods[] = {
    { "CreateTextureFromSurface", (PyCFunction)PySDL_Renderer_CreateTextureFromSurface, METH_O       },
    { "Clear",                    (PyCFunction)PySDL_Renderer_Clear,                    METH_NOARGS  },
    { "Copy",                     (PyCFunction)PySDL_Renderer_Copy,                     METH_VARARGS | METH_KEYWORDS },
    { "CopyEx",                   (PyCFunction)PySDL_Renderer_CopyEx,                   METH_VARARGS | METH_KEYWORDS },
    { "Present",                  (PyCFunction)PySDL_Renderer_Present,                  METH_NOARGS  },
    { "SetRenderDrawColor",       (PyCFunction)PySDL_Renderer_SetRenderDrawColor,       METH_VARARGS },
    //
    { "LoadTexture", (PyCFunction)PySDL_Renderer_LoadTexture, METH_O },
    //
    { NULL }
};

PyTypeObject PySDL_Renderer_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name      = "SDL2.Renderer",
    .tp_basicsize = sizeof(PySDL_Renderer),
    .tp_dealloc   = (destructor)PySDL_Renderer_Type_dealloc,
    .tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc       = "SDL2.Renderer Class",
    .tp_methods   = PySDL_Renderer_methods,
    .tp_init      = (initproc)PySDL_Renderer_Type_init,
    .tp_new       = PyType_GenericNew
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

static PyObject * PySDL_Renderer_Clear(PySDL_Renderer *self, PyObject *args) {
    SDL_RenderClear(self->renderer);
    Py_RETURN_NONE;
}

static PyObject * PySDL_Renderer_Copy(PySDL_Renderer *self, PyObject *args, PyObject *kwds) {
    PySDL_Texture *texture = NULL;
    PyObject *src_py = NULL;
    PyObject *dst_py = NULL;
    SDL_Rect src_rect;
    SDL_Rect dst_rect;
    SDL_Rect *src = NULL;
    SDL_Rect *dst = NULL;

    int ok;

    static char *kwlist[] = {"texture", "src", "dst", NULL};

    ok = PyArg_ParseTupleAndKeywords(args, kwds, "O|OO", kwlist, &texture, &src_py, &dst_py);
    if(!ok) {
        return NULL;
    }

    if (src_py != Py_None) {
        PyToRect(src_py, &src_rect);
        src = &src_rect;
    }

    if (dst_py != Py_None) {
        PyToRect(dst_py, &dst_rect);
        dst = &dst_rect;
    }

    ok = SDL_RenderCopy(self->renderer, texture->texture, src, dst);
    if(0 > ok) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject * PySDL_Renderer_CopyEx(PySDL_Renderer *self, PyObject *args, PyObject *kwds) {
    PySDL_Texture *texture = NULL;
    PyObject *src_py = NULL;
    PyObject *dst_py = NULL;
    SDL_Rect src_rect;
    SDL_Rect dst_rect;
    SDL_Rect *src = NULL;
    SDL_Rect *dst = NULL;
    double angle;
    PyObject *center_py = NULL;
    SDL_Point center_point;
    SDL_Point *center = NULL;
    SDL_RendererFlip flip = SDL_FLIP_NONE;

    int ok;

    static char *kwlist[] = {"texture", "src", "dst", "angle", "center", "flip", NULL};

    ok = PyArg_ParseTupleAndKeywords(args, kwds, "O|OOdOI", kwlist, &texture, &src_py, &dst_py, &angle, &center_py, &flip);
    if(!ok) {
        return NULL;
    }

    if (src_py != Py_None) {
        PyToRect(src_py, &src_rect);
        src = &src_rect;
    }

    if (dst_py != Py_None) {
        PyToRect(dst_py, &dst_rect);
        dst = &dst_rect;
    }

    if (center_py != Py_None) {
        PyToPoint(center_py, &center_point);
        center = &center_point;
    }

    ok = SDL_RenderCopyEx(self->renderer, texture->texture, src, dst, angle, center, flip);
    if(0 > ok) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject * PySDL_Renderer_Present(PySDL_Renderer *self, PyObject *args) {
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
