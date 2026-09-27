#include "pysdl.h"

#ifdef PYSDL_HAVE_TTF

// SDL_ttf: the SDL2.Font type plus the TTF_* module functions (registered in
// PyInit_SDL2 via PyModule_AddFunctions(module, pysdl_ttf_methods)). Built only
// when setup.py finds SDL2_ttf.
//
// Text is always a Python str and goes through the UTF-8 entry points, so the
// Latin-1 (TTF_*Text*) and UCS-2 (TTF_*UNICODE*) variants are not exposed;
// glyphs use the 32-bit codepoint functions. Each Render* method takes an
// optional wrap_length in place of the separate *_Wrapped functions.
//
// Lifetime: fonts must be closed before SDL_ttf shuts down (TTF_CloseFont after
// the final TTF_Quit touches freed FreeType state). Every Font records the init
// "session" it was opened in; once TTF_Quit really shuts SDL_ttf down the
// session advances, and older fonts raise on use and are dropped, not closed.
//
// The GIL is held throughout: a TTF_Font (and FreeType itself) is not safe to
// use from two threads at once.

static unsigned _session = 1;  // advances each time SDL_ttf fully shuts down

static int _alive(PySDL_Font *self) {
    return NULL != self->font && TTF_WasInit() > 0 && self->session == _session;
}

static PyObject * _raise(void) {
    PyErr_SetString(pysdl_Error, TTF_GetError());
    return NULL;
}

static void _close(PySDL_Font *self) {
    if(_alive(self)) {
        TTF_CloseFont(self->font);  // also closes the RWops (freesrc = 1)
    }
    self->font = NULL;
    if(NULL != self->data.obj) {
        PyBuffer_Release(&self->data);
        self->data.obj = NULL;
    }
}

static TTF_Font * _font(PySDL_Font *self) {
    if(NULL == self->font) {
        PyErr_SetString(pysdl_Error, "Font is not open");
        return NULL;
    }
    if(!_alive(self)) {
        PyErr_SetString(pysdl_Error, "Font is no longer valid: SDL2.TTF_Quit() was called");
        return NULL;
    }
    return self->font;
}

// str -> UTF-8 without embedded NULs (SDL_ttf stops at the first one).
static const char * _utf8(PyObject *text) {
    Py_ssize_t len;
    const char *utf8 = PyUnicode_AsUTF8AndSize(text, &len);
    if(NULL != utf8 && (Py_ssize_t)strlen(utf8) != len) {
        PyErr_SetString(PyExc_ValueError, "text must not contain NUL characters");
        return NULL;
    }
    return utf8;
}

// A one-character str or an int codepoint.
static int _codepoint(PyObject *obj, Uint32 *out) {
    if(PyUnicode_Check(obj)) {
        if(1 != PyUnicode_GET_LENGTH(obj)) {
            PyErr_SetString(PyExc_ValueError, "expected a single character");
            return 0;
        }
        *out = PyUnicode_READ_CHAR(obj, 0);
        return 1;
    }
    unsigned long value = PyLong_AsUnsignedLong(obj);
    if((unsigned long)-1 == value && PyErr_Occurred()) {
        return 0;
    }
    *out = (Uint32)value;
#if !SDL_TTF_VERSION_ATLEAST(2,0,18)
    if(value > 0xFFFF) {
        PyErr_SetString(PyExc_ValueError, "codepoints above U+FFFF need SDL_ttf >= 2.0.18");
        return 0;
    }
#endif
    return 1;
}

static PyObject * _wrap_surface(SDL_Surface *surface) {
    if(NULL == surface) {
        return _raise();
    }
    PySDL_Surface *wrapper = (PySDL_Surface *)PySDL_New(&PySDL_Surface_Type);
    if(NULL == wrapper) {
        SDL_FreeSurface(surface);
        return NULL;
    }
    wrapper->surface = surface;
    return (PyObject *)wrapper;
}

//=========================================================
// type
//=========================================================

static int PySDL_Font_Type_init(PySDL_Font *self, PyObject *args, PyObject *kwds) {
    PyObject *src = Py_None;
    int ptsize = 12;
    long index = 0;
    unsigned int hdpi = 0, vdpi = 0;

    static char *kwlist[] = {"src", "ptsize", "index", "hdpi", "vdpi", NULL};
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "|Oil$II", kwlist, &src, &ptsize, &index, &hdpi, &vdpi)) {
        return -1;
    }

    _close(self);  // re-running __init__ on an open font
    self->session = _session;

    // No source: internal allocation, nothing to open.
    if(src == Py_None) {
        return 0;
    }
    if(0 == TTF_WasInit()) {
        PyErr_SetString(pysdl_Error, "call SDL2.TTF_Init() before opening a font");
        return -1;
    }

    SDL_RWops *rw = PySDL_RWFromObject(src, &self->data);
    if(NULL == rw) {
        return -1;
    }
#if SDL_TTF_VERSION_ATLEAST(2,0,18)
    self->font = TTF_OpenFontIndexDPIRW(rw, 1, ptsize, index, hdpi, vdpi);
#else
    if(hdpi || vdpi) {
        SDL_RWclose(rw);
        PyBuffer_Release(&self->data);
        self->data.obj = NULL;
        PyErr_SetString(PyExc_ValueError, "hdpi / vdpi need SDL_ttf >= 2.0.18");
        return -1;
    }
    self->font = TTF_OpenFontIndexRW(rw, 1, ptsize, index);
#endif
    if(NULL == self->font) {  // SDL_ttf has already closed rw
        PyBuffer_Release(&self->data);
        self->data.obj = NULL;
        _raise();
        return -1;
    }
    return 0;
}

static void PySDL_Font_Type_dealloc(PySDL_Font *self) {
    _close(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject * PySDL_Font_Close(PySDL_Font *self, PyObject *ign) {
    _close(self);
    Py_RETURN_NONE;
}

//=========================================================
// attributes
//=========================================================

#define FONT_GET_INT(NAME, CALL) \
    static PyObject * PySDL_Font_##NAME(PySDL_Font *self, PyObject *ign) { \
        TTF_Font *font = _font(self); \
        return font ? PyLong_FromLong(CALL(font)) : NULL; \
    }

FONT_GET_INT(GetStyle,   TTF_GetFontStyle)
FONT_GET_INT(GetOutline, TTF_GetFontOutline)
FONT_GET_INT(GetHinting, TTF_GetFontHinting)
FONT_GET_INT(Height,     TTF_FontHeight)
FONT_GET_INT(Ascent,     TTF_FontAscent)
FONT_GET_INT(Descent,    TTF_FontDescent)
FONT_GET_INT(LineSkip,   TTF_FontLineSkip)
FONT_GET_INT(Faces,      TTF_FontFaces)
#if SDL_TTF_VERSION_ATLEAST(2,20,0)
FONT_GET_INT(GetWrappedAlign, TTF_GetFontWrappedAlign)
#endif

#define FONT_SET_INT(NAME, CALL) \
    static PyObject * PySDL_Font_##NAME(PySDL_Font *self, PyObject *arg) { \
        TTF_Font *font = _font(self); \
        long value = PyLong_AsLong(arg); \
        if(NULL == font || (-1 == value && PyErr_Occurred())) { \
            return NULL; \
        } \
        CALL(font, (int)value); \
        Py_RETURN_NONE; \
    }

FONT_SET_INT(SetStyle,   TTF_SetFontStyle)
FONT_SET_INT(SetOutline, TTF_SetFontOutline)
FONT_SET_INT(SetHinting, TTF_SetFontHinting)
#if SDL_TTF_VERSION_ATLEAST(2,20,0)
FONT_SET_INT(SetWrappedAlign, TTF_SetFontWrappedAlign)
#endif

static PyObject * PySDL_Font_GetKerning(PySDL_Font *self, PyObject *ign) {
    TTF_Font *font = _font(self);
    return font ? PyBool_FromLong(TTF_GetFontKerning(font)) : NULL;
}

static PyObject * PySDL_Font_SetKerning(PySDL_Font *self, PyObject *arg) {
    TTF_Font *font = _font(self);
    int allowed = PyObject_IsTrue(arg);
    if(NULL == font || -1 == allowed) {
        return NULL;
    }
    TTF_SetFontKerning(font, allowed);
    Py_RETURN_NONE;
}

static PyObject * PySDL_Font_FaceIsFixedWidth(PySDL_Font *self, PyObject *ign) {
    TTF_Font *font = _font(self);
    return font ? PyBool_FromLong(TTF_FontFaceIsFixedWidth(font) > 0) : NULL;
}

static PyObject * _str_or_none(const char *text) {
    if(NULL == text) {
        Py_RETURN_NONE;
    }
    return PyUnicode_FromString(text);
}

static PyObject * PySDL_Font_FaceFamilyName(PySDL_Font *self, PyObject *ign) {
    TTF_Font *font = _font(self);
    return font ? _str_or_none(TTF_FontFaceFamilyName(font)) : NULL;
}

static PyObject * PySDL_Font_FaceStyleName(PySDL_Font *self, PyObject *ign) {
    TTF_Font *font = _font(self);
    return font ? _str_or_none(TTF_FontFaceStyleName(font)) : NULL;
}

#if SDL_TTF_VERSION_ATLEAST(2,0,18)
static PyObject * PySDL_Font_SetSize(PySDL_Font *self, PyObject *arg) {
    TTF_Font *font = _font(self);
    long ptsize = PyLong_AsLong(arg);
    if(NULL == font || (-1 == ptsize && PyErr_Occurred())) {
        return NULL;
    }
    if(0 > TTF_SetFontSize(font, (int)ptsize)) {
        return _raise();
    }
    Py_RETURN_NONE;
}

static PyObject * PySDL_Font_SetSizeDPI(PySDL_Font *self, PyObject *args) {
    int ptsize;
    unsigned int hdpi, vdpi;
    TTF_Font *font = _font(self);
    if(NULL == font || !PyArg_ParseTuple(args, "iII", &ptsize, &hdpi, &vdpi)) {
        return NULL;
    }
    if(0 > TTF_SetFontSizeDPI(font, ptsize, hdpi, vdpi)) {
        return _raise();
    }
    Py_RETURN_NONE;
}

static PyObject * PySDL_Font_GetSDF(PySDL_Font *self, PyObject *ign) {
    TTF_Font *font = _font(self);
    return font ? PyBool_FromLong(TTF_GetFontSDF(font)) : NULL;
}

static PyObject * PySDL_Font_SetSDF(PySDL_Font *self, PyObject *arg) {
    TTF_Font *font = _font(self);
    int on = PyObject_IsTrue(arg);
    if(NULL == font || -1 == on) {
        return NULL;
    }
    if(0 > TTF_SetFontSDF(font, on ? SDL_TRUE : SDL_FALSE)) {
        return _raise();
    }
    Py_RETURN_NONE;
}
#endif

#if SDL_TTF_VERSION_ATLEAST(2,20,0)
static PyObject * PySDL_Font_SetDirection(PySDL_Font *self, PyObject *arg) {
    TTF_Font *font = _font(self);
    long direction = PyLong_AsLong(arg);
    if(NULL == font || (-1 == direction && PyErr_Occurred())) {
        return NULL;
    }
    if(0 > TTF_SetFontDirection(font, (TTF_Direction)direction)) {
        return _raise();  // e.g. SDL_ttf built without HarfBuzz
    }
    Py_RETURN_NONE;
}

// SetScriptName("Latn"): the ISO 15924 script code used for shaping.
static PyObject * PySDL_Font_SetScriptName(PySDL_Font *self, PyObject *arg) {
    TTF_Font *font = _font(self);
    const char *script = font ? PyUnicode_AsUTF8(arg) : NULL;
    if(NULL == script) {
        return NULL;
    }
    if(0 > TTF_SetFontScriptName(font, script)) {
        return _raise();
    }
    Py_RETURN_NONE;
}
#endif

//=========================================================
// glyphs and measuring
//=========================================================

static PyObject * PySDL_Font_GlyphIsProvided(PySDL_Font *self, PyObject *arg) {
    TTF_Font *font = _font(self);
    Uint32 ch;
    if(NULL == font || !_codepoint(arg, &ch)) {
        return NULL;
    }
#if SDL_TTF_VERSION_ATLEAST(2,0,18)
    return PyBool_FromLong(0 != TTF_GlyphIsProvided32(font, ch));
#else
    return PyBool_FromLong(0 != TTF_GlyphIsProvided(font, (Uint16)ch));
#endif
}

// GlyphMetrics(ch) -> (minx, maxx, miny, maxy, advance)
static PyObject * PySDL_Font_GlyphMetrics(PySDL_Font *self, PyObject *arg) {
    TTF_Font *font = _font(self);
    Uint32 ch;
    if(NULL == font || !_codepoint(arg, &ch)) {
        return NULL;
    }
    int minx, maxx, miny, maxy, advance;
#if SDL_TTF_VERSION_ATLEAST(2,0,18)
    int rc = TTF_GlyphMetrics32(font, ch, &minx, &maxx, &miny, &maxy, &advance);
#else
    int rc = TTF_GlyphMetrics(font, (Uint16)ch, &minx, &maxx, &miny, &maxy, &advance);
#endif
    if(0 > rc) {
        return _raise();
    }
    return Py_BuildValue("(iiiii)", minx, maxx, miny, maxy, advance);
}

#if SDL_TTF_VERSION_ATLEAST(2,0,14)
// GetKerningSize(previous_ch, ch) -> pixels to adjust between the two glyphs
static PyObject * PySDL_Font_GetKerningSize(PySDL_Font *self, PyObject *args) {
    PyObject *a_py, *b_py;
    Uint32 a, b;
    TTF_Font *font = _font(self);
    if(NULL == font || !PyArg_ParseTuple(args, "OO", &a_py, &b_py)
        || !_codepoint(a_py, &a) || !_codepoint(b_py, &b)) {
        return NULL;
    }
#if SDL_TTF_VERSION_ATLEAST(2,0,18)
    int kerning = TTF_GetFontKerningSizeGlyphs32(font, a, b);
#else
    int kerning = TTF_GetFontKerningSizeGlyphs(font, (Uint16)a, (Uint16)b);
#endif
    return PyLong_FromLong(kerning);
}
#endif

// Size(text) -> (w, h) the rendered text would take, without rendering it.
static PyObject * PySDL_Font_Size(PySDL_Font *self, PyObject *arg) {
    TTF_Font *font = _font(self);
    const char *text = font ? _utf8(arg) : NULL;
    if(NULL == text) {
        return NULL;
    }
    int w = 0, h = 0;
    if(0 > TTF_SizeUTF8(font, text, &w, &h)) {
        return _raise();
    }
    return Py_BuildValue("(ii)", w, h);
}

#if SDL_TTF_VERSION_ATLEAST(2,0,18)
// Measure(text, width) -> (extent, count): how many characters fit in `width`
// pixels, and how wide they are.
static PyObject * PySDL_Font_Measure(PySDL_Font *self, PyObject *args) {
    PyObject *text_py;
    int width;
    TTF_Font *font = _font(self);
    if(NULL == font || !PyArg_ParseTuple(args, "Oi", &text_py, &width)) {
        return NULL;
    }
    const char *text = _utf8(text_py);
    if(NULL == text) {
        return NULL;
    }
    int extent = 0, count = 0;
    if(0 > TTF_MeasureUTF8(font, text, width, &extent, &count)) {
        return _raise();
    }
    return Py_BuildValue("(ii)", extent, count);
}
#endif

//=========================================================
// rendering -> Surface
//=========================================================

enum { R_SOLID, R_SHADED, R_BLENDED, R_LCD };

// Render{Solid,Shaded,Blended,LCD}(text, fg[, bg], wrap_length=None). With a
// wrap_length the *_Wrapped variant runs (0 = break only at newlines).
static PyObject * _render_text(PySDL_Font *self, PyObject *args, PyObject *kwds, int kind) {
    PyObject *text_py, *fg_py, *bg_py = NULL, *wrap_py = Py_None;
    int with_bg = (R_SHADED == kind || R_LCD == kind);
    static char *kw_fg[] = {"text", "fg", "wrap_length", NULL};
    static char *kw_bg[] = {"text", "fg", "bg", "wrap_length", NULL};

    TTF_Font *font = _font(self);
    if(NULL == font) {
        return NULL;
    }
    int ok = with_bg
        ? PyArg_ParseTupleAndKeywords(args, kwds, "OOO|O", kw_bg, &text_py, &fg_py, &bg_py, &wrap_py)
        : PyArg_ParseTupleAndKeywords(args, kwds, "OO|O", kw_fg, &text_py, &fg_py, &wrap_py);
    if(!ok) {
        return NULL;
    }

    SDL_Color fg, bg = {0, 0, 0, 0};
    const char *text = _utf8(text_py);
    if(NULL == text || !PyToColor(fg_py, &fg) || (with_bg && !PyToColor(bg_py, &bg))) {
        return NULL;
    }

    SDL_Surface *surface = NULL;
    if(wrap_py == Py_None) {
        switch(kind) {
        case R_SOLID:   surface = TTF_RenderUTF8_Solid(font, text, fg);       break;
        case R_SHADED:  surface = TTF_RenderUTF8_Shaded(font, text, fg, bg);  break;
        case R_BLENDED: surface = TTF_RenderUTF8_Blended(font, text, fg);     break;
#if SDL_TTF_VERSION_ATLEAST(2,20,0)
        case R_LCD:     surface = TTF_RenderUTF8_LCD(font, text, fg, bg);     break;
#endif
        }
        return _wrap_surface(surface);
    }

#if SDL_TTF_VERSION_ATLEAST(2,0,18)
    long wrap = PyLong_AsLong(wrap_py);
    if(-1 == wrap && PyErr_Occurred()) {
        return NULL;
    }
    if(wrap < 0) {
        PyErr_SetString(PyExc_ValueError, "wrap_length must be >= 0");
        return NULL;
    }
    Uint32 w = (Uint32)wrap;
    switch(kind) {
    case R_SOLID:   surface = TTF_RenderUTF8_Solid_Wrapped(font, text, fg, w);       break;
    case R_SHADED:  surface = TTF_RenderUTF8_Shaded_Wrapped(font, text, fg, bg, w);  break;
    case R_BLENDED: surface = TTF_RenderUTF8_Blended_Wrapped(font, text, fg, w);     break;
#if SDL_TTF_VERSION_ATLEAST(2,20,0)
    case R_LCD:     surface = TTF_RenderUTF8_LCD_Wrapped(font, text, fg, bg, w);     break;
#endif
    }
    return _wrap_surface(surface);
#else
    PyErr_SetString(PyExc_ValueError, "wrap_length needs SDL_ttf >= 2.0.18");
    return NULL;
#endif
}

static PyObject * PySDL_Font_RenderSolid(PySDL_Font *self, PyObject *args, PyObject *kwds) {
    return _render_text(self, args, kwds, R_SOLID);
}

static PyObject * PySDL_Font_RenderShaded(PySDL_Font *self, PyObject *args, PyObject *kwds) {
    return _render_text(self, args, kwds, R_SHADED);
}

static PyObject * PySDL_Font_RenderBlended(PySDL_Font *self, PyObject *args, PyObject *kwds) {
    return _render_text(self, args, kwds, R_BLENDED);
}

#if SDL_TTF_VERSION_ATLEAST(2,20,0)
static PyObject * PySDL_Font_RenderLCD(PySDL_Font *self, PyObject *args, PyObject *kwds) {
    return _render_text(self, args, kwds, R_LCD);
}
#endif

// RenderGlyph{Solid,Shaded,Blended,LCD}(ch, fg[, bg])
static PyObject * _render_glyph(PySDL_Font *self, PyObject *args, int kind) {
    PyObject *ch_py, *fg_py, *bg_py = NULL;
    int with_bg = (R_SHADED == kind || R_LCD == kind);
    TTF_Font *font = _font(self);
    if(NULL == font) {
        return NULL;
    }
    int ok = with_bg ? PyArg_ParseTuple(args, "OOO", &ch_py, &fg_py, &bg_py)
                     : PyArg_ParseTuple(args, "OO", &ch_py, &fg_py);
    if(!ok) {
        return NULL;
    }
    Uint32 ch;
    SDL_Color fg, bg = {0, 0, 0, 0};
    if(!_codepoint(ch_py, &ch) || !PyToColor(fg_py, &fg) || (with_bg && !PyToColor(bg_py, &bg))) {
        return NULL;
    }

    SDL_Surface *surface = NULL;
#if SDL_TTF_VERSION_ATLEAST(2,0,18)
    switch(kind) {
    case R_SOLID:   surface = TTF_RenderGlyph32_Solid(font, ch, fg);       break;
    case R_SHADED:  surface = TTF_RenderGlyph32_Shaded(font, ch, fg, bg);  break;
    case R_BLENDED: surface = TTF_RenderGlyph32_Blended(font, ch, fg);     break;
#if SDL_TTF_VERSION_ATLEAST(2,20,0)
    case R_LCD:     surface = TTF_RenderGlyph32_LCD(font, ch, fg, bg);     break;
#endif
    }
#else
    switch(kind) {
    case R_SOLID:   surface = TTF_RenderGlyph_Solid(font, (Uint16)ch, fg);      break;
    case R_SHADED:  surface = TTF_RenderGlyph_Shaded(font, (Uint16)ch, fg, bg); break;
    case R_BLENDED: surface = TTF_RenderGlyph_Blended(font, (Uint16)ch, fg);    break;
    }
#endif
    return _wrap_surface(surface);
}

static PyObject * PySDL_Font_RenderGlyphSolid(PySDL_Font *self, PyObject *args) {
    return _render_glyph(self, args, R_SOLID);
}

static PyObject * PySDL_Font_RenderGlyphShaded(PySDL_Font *self, PyObject *args) {
    return _render_glyph(self, args, R_SHADED);
}

static PyObject * PySDL_Font_RenderGlyphBlended(PySDL_Font *self, PyObject *args) {
    return _render_glyph(self, args, R_BLENDED);
}

#if SDL_TTF_VERSION_ATLEAST(2,20,0)
static PyObject * PySDL_Font_RenderGlyphLCD(PySDL_Font *self, PyObject *args) {
    return _render_glyph(self, args, R_LCD);
}
#endif

static PyMethodDef PySDL_Font_methods[] = {
    { "Close",            (PyCFunction)PySDL_Font_Close,            METH_NOARGS  },
    { "GetStyle",         (PyCFunction)PySDL_Font_GetStyle,         METH_NOARGS  },
    { "SetStyle",         (PyCFunction)PySDL_Font_SetStyle,         METH_O       },
    { "GetOutline",       (PyCFunction)PySDL_Font_GetOutline,       METH_NOARGS  },
    { "SetOutline",       (PyCFunction)PySDL_Font_SetOutline,       METH_O       },
    { "GetHinting",       (PyCFunction)PySDL_Font_GetHinting,       METH_NOARGS  },
    { "SetHinting",       (PyCFunction)PySDL_Font_SetHinting,       METH_O       },
    { "GetKerning",       (PyCFunction)PySDL_Font_GetKerning,       METH_NOARGS  },
    { "SetKerning",       (PyCFunction)PySDL_Font_SetKerning,       METH_O       },
    { "Height",           (PyCFunction)PySDL_Font_Height,           METH_NOARGS  },
    { "Ascent",           (PyCFunction)PySDL_Font_Ascent,           METH_NOARGS  },
    { "Descent",          (PyCFunction)PySDL_Font_Descent,          METH_NOARGS  },
    { "LineSkip",         (PyCFunction)PySDL_Font_LineSkip,         METH_NOARGS  },
    { "Faces",            (PyCFunction)PySDL_Font_Faces,            METH_NOARGS  },
    { "FaceIsFixedWidth", (PyCFunction)PySDL_Font_FaceIsFixedWidth, METH_NOARGS  },
    { "FaceFamilyName",   (PyCFunction)PySDL_Font_FaceFamilyName,   METH_NOARGS  },
    { "FaceStyleName",    (PyCFunction)PySDL_Font_FaceStyleName,    METH_NOARGS  },
    { "GlyphIsProvided",  (PyCFunction)PySDL_Font_GlyphIsProvided,  METH_O       },
    { "GlyphMetrics",     (PyCFunction)PySDL_Font_GlyphMetrics,     METH_O       },
    { "Size",             (PyCFunction)PySDL_Font_Size,             METH_O       },
    { "RenderSolid",      (PyCFunction)PySDL_Font_RenderSolid,      METH_VARARGS | METH_KEYWORDS },
    { "RenderShaded",     (PyCFunction)PySDL_Font_RenderShaded,     METH_VARARGS | METH_KEYWORDS },
    { "RenderBlended",    (PyCFunction)PySDL_Font_RenderBlended,    METH_VARARGS | METH_KEYWORDS },
    { "RenderGlyphSolid",   (PyCFunction)PySDL_Font_RenderGlyphSolid,   METH_VARARGS },
    { "RenderGlyphShaded",  (PyCFunction)PySDL_Font_RenderGlyphShaded,  METH_VARARGS },
    { "RenderGlyphBlended", (PyCFunction)PySDL_Font_RenderGlyphBlended, METH_VARARGS },
#if SDL_TTF_VERSION_ATLEAST(2,0,14)
    { "GetKerningSize",   (PyCFunction)PySDL_Font_GetKerningSize,   METH_VARARGS },
#endif
#if SDL_TTF_VERSION_ATLEAST(2,0,18)
    { "SetSize",          (PyCFunction)PySDL_Font_SetSize,          METH_O       },
    { "SetSizeDPI",       (PyCFunction)PySDL_Font_SetSizeDPI,       METH_VARARGS },
    { "GetSDF",           (PyCFunction)PySDL_Font_GetSDF,           METH_NOARGS  },
    { "SetSDF",           (PyCFunction)PySDL_Font_SetSDF,           METH_O       },
    { "Measure",          (PyCFunction)PySDL_Font_Measure,          METH_VARARGS },
#endif
#if SDL_TTF_VERSION_ATLEAST(2,20,0)
    { "GetWrappedAlign",  (PyCFunction)PySDL_Font_GetWrappedAlign,  METH_NOARGS  },
    { "SetWrappedAlign",  (PyCFunction)PySDL_Font_SetWrappedAlign,  METH_O       },
    { "SetDirection",     (PyCFunction)PySDL_Font_SetDirection,     METH_O       },
    { "SetScriptName",    (PyCFunction)PySDL_Font_SetScriptName,    METH_O       },
    { "RenderLCD",        (PyCFunction)PySDL_Font_RenderLCD,        METH_VARARGS | METH_KEYWORDS },
    { "RenderGlyphLCD",   (PyCFunction)PySDL_Font_RenderGlyphLCD,   METH_VARARGS },
#endif
    { NULL }
};

PyTypeObject PySDL_Font_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name      = "SDL2.Font",
    .tp_basicsize = sizeof(PySDL_Font),
    .tp_dealloc   = (destructor)PySDL_Font_Type_dealloc,
    .tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc       = "SDL2.Font(src, ptsize=12, index=0, *, hdpi=0, vdpi=0): src is a path or the font file's bytes",
    .tp_methods   = PySDL_Font_methods,
    .tp_init      = (initproc)PySDL_Font_Type_init,
    .tp_new       = PyType_GenericNew
};

//=========================================================
// module functions
//=========================================================

static PyObject * PySDL_TTF_Init(PyObject *self, PyObject *ign) {
    if(0 > TTF_Init()) {
        return _raise();
    }
    Py_RETURN_NONE;
}

// TTF_Init / TTF_Quit are counted; the last TTF_Quit invalidates open fonts.
static PyObject * PySDL_TTF_Quit(PyObject *self, PyObject *ign) {
    if(TTF_WasInit() > 0) {
        TTF_Quit();
        if(0 == TTF_WasInit()) {
            ++_session;
        }
    }
    Py_RETURN_NONE;
}

static PyObject * PySDL_TTF_WasInit(PyObject *self, PyObject *ign) {
    return PyLong_FromLong(TTF_WasInit());
}

static PyObject * _version(const SDL_version *v) {
    return Py_BuildValue("(iii)", v->major, v->minor, v->patch);
}

static PyObject * PySDL_TTF_Linked_Version(PyObject *self, PyObject *ign) {
    return _version(TTF_Linked_Version());
}

#if SDL_TTF_VERSION_ATLEAST(2,0,18)
static PyObject * PySDL_TTF_GetFreeTypeVersion(PyObject *self, PyObject *ign) {
    int major, minor, patch;
    TTF_GetFreeTypeVersion(&major, &minor, &patch);
    return Py_BuildValue("(iii)", major, minor, patch);
}

// (0, 0, 0) when SDL_ttf was built without HarfBuzz.
static PyObject * PySDL_TTF_GetHarfBuzzVersion(PyObject *self, PyObject *ign) {
    int major, minor, patch;
    TTF_GetHarfBuzzVersion(&major, &minor, &patch);
    return Py_BuildValue("(iii)", major, minor, patch);
}
#endif

PyMethodDef pysdl_ttf_methods[] = {
    { "TTF_Init",               PySDL_TTF_Init,               METH_NOARGS },
    { "TTF_Quit",               PySDL_TTF_Quit,               METH_NOARGS },
    { "TTF_WasInit",            PySDL_TTF_WasInit,            METH_NOARGS },
    { "TTF_Linked_Version",     PySDL_TTF_Linked_Version,     METH_NOARGS },
#if SDL_TTF_VERSION_ATLEAST(2,0,18)
    { "TTF_GetFreeTypeVersion", PySDL_TTF_GetFreeTypeVersion, METH_NOARGS },
    { "TTF_GetHarfBuzzVersion", PySDL_TTF_GetHarfBuzzVersion, METH_NOARGS },
#endif
    { NULL }
};

#endif // PYSDL_HAVE_TTF
