#ifndef __PYSDL_H__
#define __PYSDL_H__

#include <Python.h>
#include <structmember.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define  DOC_MOD  "Python wrapper for SDL."


#if PY_MAJOR_VERSION >= 3
    #define IS_PY3K
    #define RETURNNULL return NULL
    #define PY_TOSTR   PyUnicode_AsUTF8
    #define PY_FROMSTR PyUnicode_FromString
#else
    #define RETURNNULL return
    #define PY_TOSTR   PyString_AsString
    #define PY_FROMSTR PyString_FromString
#endif


extern PyObject *pysdl_Error;


typedef struct {
    PyObject_HEAD
    SDL_Window *window;
    SDL_GLContext glContext;
} PySDL_Window;

extern PyTypeObject PySDL_Window_Type;


typedef struct {
    PyObject_HEAD
    SDL_Renderer *renderer;
} PySDL_Renderer;

extern PyTypeObject PySDL_Renderer_Type;


typedef struct {
    PyObject_HEAD
    SDL_Surface *surface;
} PySDL_Surface;

extern PyTypeObject PySDL_Surface_Type;


typedef struct {
    PyObject_HEAD
    SDL_Texture *texture;
} PySDL_Texture;

extern PyTypeObject PySDL_Texture_Type;


#endif // __PYSDL_H__
