#ifndef __PYSDL_H__
#define __PYSDL_H__

#include <Python.h>
#include <structmember.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define  DOC_MOD  "Python wrapper for SDL."

extern PyObject *pysdl_Error;


//=========================================================
// Types
//=========================================================


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
    int shouldFree;
} PySDL_Surface;

extern PyTypeObject PySDL_Surface_Type;


typedef struct {
    PyObject_HEAD
    SDL_Texture *texture;
} PySDL_Texture;

extern PyTypeObject PySDL_Texture_Type;


#endif // __PYSDL_H__
