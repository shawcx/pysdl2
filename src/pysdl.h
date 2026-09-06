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
    PyObject *target;  // Texture currently set as render target, or NULL
} PySDL_Renderer;
extern PyTypeObject PySDL_Renderer_Type;

typedef struct {
    PyObject_HEAD
    SDL_Surface *surface;
    int shouldFree;
    Py_buffer pixels;  // backing buffer for a *...From surface; pixels.obj == NULL otherwise
} PySDL_Surface;
extern PyTypeObject PySDL_Surface_Type;

typedef struct {
    PyObject_HEAD
    SDL_Texture *texture;
} PySDL_Texture;
extern PyTypeObject PySDL_Texture_Type;

typedef struct {
    PyObject_HEAD
    SDL_PixelFormat *format;
} PySDL_PixelFormat;
extern PyTypeObject PySDL_PixelFormat_Type;

typedef struct {
    PyObject_HEAD
    SDL_Palette *palette;
} PySDL_Palette;
extern PyTypeObject PySDL_Palette_Type;

typedef struct {
    PyObject_HEAD
    SDL_AudioDeviceID deviceId;
    PyObject *pycallback;  // (callback, userdata) tuple passed to SDL_OpenAudioDevice
} PySDL_Audio;
extern PyTypeObject PySDL_Audio_Type;

// massive list of SDL2 constants
void _constants(PyObject *module);

//=========================================================
// Helpers (pysdl_util.c)
//=========================================================

// Allocate a wrapper instance of `type`; sets a TypeError and returns NULL on
// failure. Replaces the repeated PyObject_CallObject boilerplate.
PyObject * PySDL_New(PyTypeObject *type);

// Enter/leave the interpreter from an SDL-owned thread (audio, timer, ...).
// PySDL_ThreadEnter returns 0 without acquiring the GIL if Python is shutting
// down, in which case the caller must not touch any Python object.
int  PySDL_ThreadEnter(PyGILState_STATE *state);
void PySDL_ThreadLeave(PyGILState_STATE state);

// Convert a Python tuple/list to an SDL struct. Return 1 on success, or 0 with
// an exception set (usable directly as an "O&" converter).
int PyToRect(PyObject *src, SDL_Rect *dst);     // 2 items -> w/h = -1, or 4 items
int PyToPoint(PyObject *src, SDL_Point *dst);   // 2 items
int PyToColor(PyObject *src, SDL_Color *dst);   // 3 items -> a = 255, or 4 items
int PyToFRect(PyObject *src, SDL_FRect *dst);   // 2 items -> w/h = -1, or 4 items
int PyToFPoint(PyObject *src, SDL_FPoint *dst); // 2 items

// A pixel value: an int is taken verbatim, a 3/4-item sequence is mapped
// through `format`.
int PyToPixel(PyObject *src, const SDL_PixelFormat *format, Uint32 *out);

PyObject * RectToPy(const SDL_Rect *rect);      // -> (x, y, w, h)
PyObject * PointToPy(const SDL_Point *point);   // -> (x, y)

#endif // __PYSDL_H__
