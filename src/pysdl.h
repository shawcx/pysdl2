#ifndef __PYSDL_H__
#define __PYSDL_H__

#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <structmember.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_vulkan.h>
#ifdef PYSDL_HAVE_TTF
    #include <SDL2/SDL_ttf.h>  // optional: setup.py defines PYSDL_HAVE_TTF when found
#endif

// Older SDL_image / SDL_ttf releases lack their *_VERSION_ATLEAST macros.
#ifndef SDL_IMAGE_VERSION_ATLEAST
    #define SDL_IMAGE_VERSION_ATLEAST(X, Y, Z) \
        (SDL_VERSIONNUM(SDL_IMAGE_MAJOR_VERSION, SDL_IMAGE_MINOR_VERSION, SDL_IMAGE_PATCHLEVEL) >= SDL_VERSIONNUM(X, Y, Z))
#endif
#if defined(PYSDL_HAVE_TTF) && !defined(SDL_TTF_VERSION_ATLEAST)
    #define SDL_TTF_VERSION_ATLEAST(X, Y, Z) \
        (SDL_VERSIONNUM(SDL_TTF_MAJOR_VERSION, SDL_TTF_MINOR_VERSION, SDL_TTF_PATCHLEVEL) >= SDL_VERSIONNUM(X, Y, Z))
#endif

#define  DOC_MOD  "Python wrapper for SDL."

#if PY_VERSION_HEX < 0x030D0000
    #define Py_IsFinalizing _Py_IsFinalizing
#endif

extern PyObject *pysdl_Error;

//=========================================================
// Types
//=========================================================

typedef struct {
    PyObject_HEAD
    SDL_Window *window;
    SDL_GLContext glContext;
    int shouldFree;  // 0 for a borrowed window (GetKeyboardFocus, GetMouseFocus, ...)
} PySDL_Window;
extern PyTypeObject PySDL_Window_Type;

typedef struct {
    PyObject_HEAD
    SDL_Renderer *renderer;
    PyObject *target;  // Texture currently set as render target, or NULL
    int shouldFree;    // 0 for a borrowed renderer (Window.GetRenderer)
} PySDL_Renderer;
extern PyTypeObject PySDL_Renderer_Type;

typedef struct {
    PyObject_HEAD
    SDL_Surface *surface;
    int shouldFree;
    Py_buffer pixels;  // backing buffer for a *...From surface; pixels.obj == NULL otherwise
    Uint32 window_id;  // non-zero: this is that window's surface (tracked in pysdl_Window.c)
} PySDL_Surface;
extern PyTypeObject PySDL_Surface_Type;

typedef struct {
    PyObject_HEAD
    SDL_Texture *texture;
    PyObject *locked;        // Surface from LockToSurface (emptied on Unlock), or NULL
    SDL_Renderer *renderer;  // the renderer that owns `texture` (see PySDL_TextureTrack)
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

typedef struct {
    PyObject_HEAD
    SDL_Cursor *cursor;
    int shouldFree;  // 0 for a borrowed cursor (GetCursor, GetDefaultCursor)
} PySDL_Cursor;
extern PyTypeObject PySDL_Cursor_Type;

typedef struct {
    PyObject_HEAD
    SDL_Joystick *joystick;  // always owned: lookups re-open by index (refcounted)
} PySDL_Joystick;
extern PyTypeObject PySDL_Joystick_Type;

typedef struct {
    PyObject_HEAD
    SDL_GameController *controller;
} PySDL_GameController;
extern PyTypeObject PySDL_GameController_Type;

// Joystick GUIDs cross into Python as 32-char hex strings (pysdl_Joystick.c).
// PyToGUID also accepts the raw 16 bytes; returns 0 with an exception set.
PyObject * GUIDToPy(SDL_JoystickGUID guid);
int        PyToGUID(PyObject *obj, SDL_JoystickGUID *guid);
// Device index of the joystick with this instance id, or -1 if none.
int        PySDL_JoystickIndexForInstance(SDL_JoystickID id);

typedef struct {
    PyObject_HEAD
    SDL_TimerID id;
    PyObject *callback;  // owned; released when the timer is removed
} PySDL_Timer;
extern PyTypeObject PySDL_Timer_Type;

typedef struct {
    PyObject_HEAD
    SDL_Haptic *haptic;
} PySDL_Haptic;
extern PyTypeObject PySDL_Haptic_Type;

typedef struct {
    PyObject_HEAD
    SDL_Sensor *sensor;
} PySDL_Sensor;
extern PyTypeObject PySDL_Sensor_Type;

#ifdef PYSDL_HAVE_TTF
typedef struct {
    PyObject_HEAD
    TTF_Font *font;
    Py_buffer data;    // pins the font file's bytes (SDL_ttf reads them lazily); data.obj == NULL for a path
    unsigned session;  // TTF init session the font was opened in (see pysdl_Font.c)
} PySDL_Font;
extern PyTypeObject PySDL_Font_Type;
#endif

typedef struct {
    PyObject_HEAD
    SDL_AudioStream *stream;
} PySDL_AudioStream;
extern PyTypeObject PySDL_AudioStream_Type;

// massive list of SDL2 constants
void _constants(PyObject *module);

// Extra module-function tables registered from their own files.
extern PyMethodDef pysdl_events_methods[];          // pysdl_Events.c  (event queue)
extern PyMethodDef pysdl_input_methods[];           // pysdl_Input.c   (keyboard / mouse / touch / text)
extern PyMethodDef pysdl_cursor_methods[];          // pysdl_Cursor.c
extern PyMethodDef pysdl_joystick_methods[];        // pysdl_Joystick.c
extern PyMethodDef pysdl_gamecontroller_methods[];  // pysdl_GameController.c
extern PyMethodDef pysdl_haptic_methods[];          // pysdl_Haptic.c
extern PyMethodDef pysdl_sensor_methods[];          // pysdl_Sensor.c
extern PyMethodDef pysdl_video_methods[];           // pysdl_Video.c   (display / messagebox / hints / vulkan / metal)
extern PyMethodDef pysdl_audio_methods[];           // pysdl_Audio.c   (drivers / LoadWAV / mixing)
extern PyMethodDef pysdl_rect_methods[];            // pysdl_Rect.c    (rect / point math)
extern PyMethodDef pysdl_image_methods[];           // pysdl_Image.c   (SDL_image loaders / format checks)
#ifdef PYSDL_HAVE_TTF
extern PyMethodDef pysdl_ttf_methods[];             // pysdl_Font.c    (SDL_ttf init / versions)
#endif

//=========================================================
// Helpers (pysdl_util.c)
//=========================================================

// Allocate a wrapper instance of `type` (defined in pysdl.c); sets a TypeError
// and returns NULL on failure. Replaces the repeated PyObject_CallObject boilerplate.
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

// Wrap a window SDL still owns as a non-freeing SDL2.Window, or None for NULL.
PyObject * PySDL_WrapWindow(SDL_Window *window);

// A set of *borrowed* wrapper pointers: each wrapper adds itself when it takes
// an SDL object and removes itself in tp_dealloc, so the set never holds a
// dead object. Used to invalidate wrappers when SDL frees what they point at.
typedef struct {
    PyObject **items;
    Py_ssize_t len, cap;
} PySDL_Registry;
int  PySDL_RegistryAdd(PySDL_Registry *reg, PyObject *obj);  // no-op if present; -1 on OOM
void PySDL_RegistryRemove(PySDL_Registry *reg, PyObject *obj);

// Point a Surface wrapper whose SDL surface was (or is about to be) freed by
// SDL at a fresh empty 0x0 surface of its own, without freeing the old one, so
// later use is harmless (pysdl_Surface.c).
void PySDL_SurfaceDetach(PySDL_Surface *surface);

// Window / window-surface tracking (pysdl_Window.c). SDL frees window surfaces
// on the next SDL_GetWindowSurface after a resize, on SDL_DestroyWindowSurface
// and with the window; it frees every window when video shuts down.
void PySDL_WindowSurfaceForget(PySDL_Surface *surface);  // from Surface tp_dealloc
void PySDL_InvalidateWindows(void);   // video is gone: null every Window and Cursor, empty every window surface
void PySDL_CheckVideoGone(void);      // PySDL_InvalidateWindows() if video is no longer running

// Cursor tracking (pysdl_Cursor.c): SDL frees every cursor when video shuts down.
void PySDL_InvalidateCursors(void);

// Texture / renderer tracking (pysdl_Renderer.c). SDL_DestroyRenderer frees
// every texture of that renderer, so each Texture wrapper records its renderer
// and is invalidated when the owning Renderer wrapper destroys it.
int  PySDL_TextureTrack(PySDL_Texture *texture, SDL_Renderer *renderer);  // after creating ->texture
void PySDL_TextureForget(PySDL_Texture *texture);                         // from Texture tp_dealloc
void PySDL_TextureInvalidate(PySDL_Texture *texture);                     // pysdl_Texture.c

// A readable SDL_RWops over `src`: a str / os.PathLike is opened as a file,
// anything else must support the buffer protocol and is read in place. `view`
// pins that buffer; call PyBuffer_Release(view) once the RWops is closed (a
// no-op release for the file case, where view->obj stays NULL). Returns NULL
// with an exception set on failure.
SDL_RWops * PySDL_RWFromObject(PyObject *src, Py_buffer *view);

// A growable in-memory RWops for SDL to write into, and its contents as bytes.
// PySDL_RWBufferBytes closes the RWops (also on failure).
SDL_RWops * PySDL_RWBuffer(void);
PyObject  * PySDL_RWBufferBytes(SDL_RWops *rw);

#endif // __PYSDL_H__
