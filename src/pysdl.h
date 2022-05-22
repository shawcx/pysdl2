#ifndef __PYSDL_H__
#define __PYSDL_H__

#define PY_SSIZE_T_CLEAN

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

typedef struct {
    PyObject_HEAD
    SDL_AudioDeviceID deviceId;
} PySDL_Audio;
extern PyTypeObject PySDL_Audio_Type;

// massive list of SDL2 constants
void _constants(PyObject *module);

// helper function to convert tuples and lists to SDL_Rect
int PyToRect(PyObject *src, SDL_Rect *dst);

#endif // __PYSDL_H__
