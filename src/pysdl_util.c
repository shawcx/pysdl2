#include "pysdl.h"

int PySDL_ThreadEnter(PyGILState_STATE *state) {
    // Checked before acquiring: once finalization has run, PyGILState_Ensure
    // is no longer safe to call.
    if(Py_IsFinalizing()) {
        return 0;
    }
    *state = PyGILState_Ensure();
    return 1;
}

void PySDL_ThreadLeave(PyGILState_STATE state) {
    PyGILState_Release(state);
}

static int _seq_to_longs(PyObject *src, long *out, Py_ssize_t lo, Py_ssize_t hi, Py_ssize_t *got) {
    PyObject *fast = PySequence_Fast(src, "expected a tuple or list");
    if(NULL == fast) {
        return 0;
    }

    Py_ssize_t n = PySequence_Fast_GET_SIZE(fast);
    if(n < lo || n > hi) {
        PyErr_Format(PyExc_ValueError, "expected %zd to %zd items, got %zd", lo, hi, n);
        Py_DECREF(fast);
        return 0;
    }

    for(Py_ssize_t idx = 0; idx < n; ++idx) {
        out[idx] = PyLong_AsLong(PySequence_Fast_GET_ITEM(fast, idx));
        if(-1 == out[idx] && PyErr_Occurred()) {
            Py_DECREF(fast);
            return 0;
        }
    }

    Py_DECREF(fast);
    *got = n;
    return 1;
}

int PyToRect(PyObject *src, SDL_Rect *dst) {
    long v[4];
    Py_ssize_t n;

    if(!_seq_to_longs(src, v, 2, 4, &n)) {
        return 0;
    }
    if(3 == n) {
        PyErr_SetString(PyExc_ValueError, "rect takes 2 (x,y) or 4 (x,y,w,h) items");
        return 0;
    }

    dst->x = (int)v[0];
    dst->y = (int)v[1];
    if(4 == n) {
        dst->w = (int)v[2];
        dst->h = (int)v[3];
    } else {
        dst->w = -1;
        dst->h = -1;
    }
    return 1;
}

int PyToPoint(PyObject *src, SDL_Point *dst) {
    long v[2];
    Py_ssize_t n;

    if(!_seq_to_longs(src, v, 2, 2, &n)) {
        return 0;
    }
    dst->x = (int)v[0];
    dst->y = (int)v[1];
    return 1;
}

static int _seq_to_doubles(PyObject *src, double *out, Py_ssize_t lo, Py_ssize_t hi, Py_ssize_t *got) {
    PyObject *fast = PySequence_Fast(src, "expected a tuple or list");
    if(NULL == fast) {
        return 0;
    }

    Py_ssize_t n = PySequence_Fast_GET_SIZE(fast);
    if(n < lo || n > hi) {
        PyErr_Format(PyExc_ValueError, "expected %zd to %zd items, got %zd", lo, hi, n);
        Py_DECREF(fast);
        return 0;
    }

    for(Py_ssize_t idx = 0; idx < n; ++idx) {
        out[idx] = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(fast, idx));
        if(-1.0 == out[idx] && PyErr_Occurred()) {
            Py_DECREF(fast);
            return 0;
        }
    }

    Py_DECREF(fast);
    *got = n;
    return 1;
}

int PyToFRect(PyObject *src, SDL_FRect *dst) {
    double v[4];
    Py_ssize_t n;

    if(!_seq_to_doubles(src, v, 2, 4, &n)) {
        return 0;
    }
    if(3 == n) {
        PyErr_SetString(PyExc_ValueError, "rect takes 2 (x,y) or 4 (x,y,w,h) items");
        return 0;
    }

    dst->x = (float)v[0];
    dst->y = (float)v[1];
    if(4 == n) {
        dst->w = (float)v[2];
        dst->h = (float)v[3];
    } else {
        dst->w = -1.0f;
        dst->h = -1.0f;
    }
    return 1;
}

int PyToFPoint(PyObject *src, SDL_FPoint *dst) {
    double v[2];
    Py_ssize_t n;

    if(!_seq_to_doubles(src, v, 2, 2, &n)) {
        return 0;
    }
    dst->x = (float)v[0];
    dst->y = (float)v[1];
    return 1;
}

int PyToColor(PyObject *src, SDL_Color *dst) {
    long v[4];
    Py_ssize_t n;

    if(!_seq_to_longs(src, v, 3, 4, &n)) {
        return 0;
    }
    dst->r = (Uint8)v[0];
    dst->g = (Uint8)v[1];
    dst->b = (Uint8)v[2];
    dst->a = (4 == n) ? (Uint8)v[3] : 255;
    return 1;
}

int PyToPixel(PyObject *src, const SDL_PixelFormat *format, Uint32 *out) {
    if(PyLong_Check(src)) {
        unsigned long value = PyLong_AsUnsignedLong(src);
        if((unsigned long)-1 == value && PyErr_Occurred()) {
            return 0;
        }
        *out = (Uint32)value;
        return 1;
    }

    SDL_Color color;
    if(!PyToColor(src, &color)) {
        return 0;
    }
    *out = SDL_MapRGBA(format, color.r, color.g, color.b, color.a);
    return 1;
}

PyObject * RectToPy(const SDL_Rect *rect) {
    return Py_BuildValue("(iiii)", rect->x, rect->y, rect->w, rect->h);
}

PyObject * PointToPy(const SDL_Point *point) {
    return Py_BuildValue("(ii)", point->x, point->y);
}

PyObject * PySDL_WrapWindow(SDL_Window *window) {
    if(NULL == window) {
        Py_RETURN_NONE;
    }
    PySDL_Window *wrapper = (PySDL_Window *)PySDL_New(&PySDL_Window_Type);
    if(NULL == wrapper) {
        return NULL;
    }
    wrapper->window = window;
    wrapper->shouldFree = 0;
    return (PyObject *)wrapper;
}

//=========================================================
// RWops helpers
//=========================================================

SDL_RWops * PySDL_RWFromObject(PyObject *src, Py_buffer *view) {
    view->obj = NULL;

    if(PyUnicode_Check(src) || PyObject_HasAttrString(src, "__fspath__")) {
        PyObject *path = PyOS_FSPath(src);
        if(NULL == path) {
            return NULL;
        }
        if(!PyUnicode_Check(path)) {
            Py_DECREF(path);
            PyErr_SetString(PyExc_TypeError, "path must be str (pass bytes for file contents)");
            return NULL;
        }
        const char *text = PyUnicode_AsUTF8(path);
        SDL_RWops *rw = text ? SDL_RWFromFile(text, "rb") : NULL;
        Py_DECREF(path);
        if(NULL == rw && !PyErr_Occurred()) {
            PyErr_SetString(pysdl_Error, SDL_GetError());
        }
        return rw;
    }

    if(0 > PyObject_GetBuffer(src, view, PyBUF_SIMPLE)) {
        PyErr_SetString(PyExc_TypeError, "expected a path (str) or the file's bytes");
        view->obj = NULL;
        return NULL;
    }
    SDL_RWops *rw = SDL_RWFromConstMem(view->buf, (int)view->len);
    if(NULL == rw) {
        PyBuffer_Release(view);
        view->obj = NULL;
        PyErr_SetString(pysdl_Error, SDL_GetError());
    }
    return rw;
}

typedef struct {
    Uint8  *data;
    size_t  len;
    size_t  cap;
    size_t  pos;
} _RWBuffer;

static Sint64 SDLCALL _rwbuf_size(SDL_RWops *rw) {
    return (Sint64)((_RWBuffer *)rw->hidden.unknown.data1)->len;
}

static Sint64 SDLCALL _rwbuf_seek(SDL_RWops *rw, Sint64 offset, int whence) {
    _RWBuffer *b = rw->hidden.unknown.data1;
    Sint64 base = (RW_SEEK_SET == whence) ? 0 : (RW_SEEK_CUR == whence) ? (Sint64)b->pos : (Sint64)b->len;
    if(base + offset < 0) {
        return SDL_SetError("seek before start of buffer");
    }
    b->pos = (size_t)(base + offset);
    return (Sint64)b->pos;
}

static size_t SDLCALL _rwbuf_read(SDL_RWops *rw, void *ptr, size_t size, size_t num) {
    _RWBuffer *b = rw->hidden.unknown.data1;
    if(0 == size || b->pos >= b->len) {
        return 0;
    }
    size_t count = SDL_min(num, (b->len - b->pos) / size);
    SDL_memcpy(ptr, b->data + b->pos, count * size);
    b->pos += count * size;
    return count;
}

static size_t SDLCALL _rwbuf_write(SDL_RWops *rw, const void *ptr, size_t size, size_t num) {
    _RWBuffer *b = rw->hidden.unknown.data1;
    size_t bytes = size * num;
    if(b->pos + bytes > b->cap) {
        size_t cap = b->cap ? b->cap : 4096;
        while(cap < b->pos + bytes) {
            cap *= 2;
        }
        Uint8 *grown = SDL_realloc(b->data, cap);
        if(NULL == grown) {
            SDL_OutOfMemory();
            return 0;
        }
        b->data = grown;
        b->cap = cap;
    }
    if(b->pos > b->len) {
        SDL_memset(b->data + b->len, 0, b->pos - b->len);  // gap from a seek past the end
    }
    SDL_memcpy(b->data + b->pos, ptr, bytes);
    b->pos += bytes;
    if(b->pos > b->len) {
        b->len = b->pos;
    }
    return num;
}

static int SDLCALL _rwbuf_close(SDL_RWops *rw) {
    _RWBuffer *b = rw->hidden.unknown.data1;
    SDL_free(b->data);
    SDL_free(b);
    SDL_FreeRW(rw);
    return 0;
}

SDL_RWops * PySDL_RWBuffer(void) {
    _RWBuffer *b = SDL_calloc(1, sizeof(*b));
    SDL_RWops *rw = b ? SDL_AllocRW() : NULL;
    if(NULL == rw) {
        SDL_free(b);
        PyErr_NoMemory();
        return NULL;
    }
    rw->type  = SDL_RWOPS_UNKNOWN;
    rw->size  = _rwbuf_size;
    rw->seek  = _rwbuf_seek;
    rw->read  = _rwbuf_read;
    rw->write = _rwbuf_write;
    rw->close = _rwbuf_close;
    rw->hidden.unknown.data1 = b;
    return rw;
}

PyObject * PySDL_RWBufferBytes(SDL_RWops *rw) {
    _RWBuffer *b = rw->hidden.unknown.data1;
    PyObject *result = PyBytes_FromStringAndSize((const char *)b->data, (Py_ssize_t)b->len);
    SDL_RWclose(rw);
    return result;
}
