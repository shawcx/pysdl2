#include "pysdl.h"

#ifdef PYSDL_HAVE_MIXER

// SDL_mixer: the SDL2.Chunk (sample) and SDL2.Music types plus the Mix_*
// module functions (registered in PyInit_SDL2 via
// PyModule_AddFunctions(module, pysdl_mixer_methods)). Built only when setup.py
// finds SDL2_mixer. SDL functions whose first argument is a chunk / music become
// methods (Mix_VolumeChunk -> Chunk.Volume, Mix_PlayMusic -> Music.Play);
// everything else keeps the Mix_ prefix, as IMG_ / TTF_ do.
//
// GIL rule: SDL_mixer runs its channel-finished / music-finished / post-mix /
// music-hook / effect callbacks on the audio thread *with the audio device
// locked*, and those take the GIL. So every Mix_* call that can lock the
// device runs with the GIL released (UNLOCKED below) - in practice almost all
// of them - or a callback firing at the same moment deadlocks against us.
// Nothing inside an UNLOCKED block may touch a Python object.
//
// Lifetimes:
// - A chunk / music that is playing is kept alive by the binding (one
//   reference per channel, one for the current music), so dropping the last
//   Python reference doesn't cut the sound off. The reference is replaced when
//   the channel is reused and released when audio is fully closed.
// - Music streams from its source for its whole life, so bytes-backed Music
//   pins the buffer (like Font). Mix_QuickLoad_RAW chunks pin theirs.
// - Mix_Quit may unload codec libraries, so Music from before the last
//   Mix_Quit (an older session) raises on use and is never freed (leaked, like
//   Font after TTF_Quit).
// - Mix_QuickLoad_WAV is not wrapped: it takes no length and trusts the WAV
//   header, so a short buffer reads out of bounds. SDL2.Chunk(bytes) is the
//   safe equivalent.

#define UNLOCKED(stmt) do { Py_BEGIN_ALLOW_THREADS stmt; Py_END_ALLOW_THREADS } while(0)

static unsigned _session = 1;          // advances on every Mix_Quit
static PyObject *_channel_chunks;      // list: channel -> Chunk playing (or last played) there
static PyObject *_playing_music;       // Music last started with Play / FadeIn

static PyObject * _raise(void) {
    PyErr_SetString(pysdl_Error, Mix_GetError());
    return NULL;
}

static int _int_arg(PyObject *arg, int *out) {
    long value = PyLong_AsLong(arg);
    if(-1 == value && PyErr_Occurred()) {
        return 0;
    }
    *out = (int)value;
    return 1;
}

//=========================================================
// keeping playing objects alive
//=========================================================

static int _remember_chunk(int channel, PyObject *chunk) {
    if(channel < 0) {
        return 0;
    }
    if(NULL == _channel_chunks && NULL == (_channel_chunks = PyList_New(0))) {
        return -1;
    }
    while(PyList_GET_SIZE(_channel_chunks) <= channel) {
        if(0 > PyList_Append(_channel_chunks, Py_None)) {
            return -1;
        }
    }
    return PyList_SetItem(_channel_chunks, channel, Py_NewRef(chunk));
}

// Channels beyond `count` no longer exist (AllocateChannels shrank, or audio
// closed with count 0): SDL halted them, so their chunks can go.
static void _forget_chunks_from(int count) {
    if(NULL != _channel_chunks && PyList_GET_SIZE(_channel_chunks) > count) {
        PyList_SetSlice(_channel_chunks, count, PY_SSIZE_T_MAX, NULL);
    }
}

//=========================================================
// Chunk
//=========================================================

static Mix_Chunk * _chunk(PySDL_Chunk *self) {
    if(NULL == self->chunk) {
        PyErr_SetString(pysdl_Error, "Chunk is not loaded");
    }
    return self->chunk;
}

static void _free_chunk(PySDL_Chunk *self) {
    if(NULL != self->chunk) {
        Mix_Chunk *chunk = self->chunk;
        self->chunk = NULL;
        UNLOCKED(Mix_FreeChunk(chunk));  // halts any channel playing it
    }
    if(NULL != self->data.obj) {
        PyBuffer_Release(&self->data);
        self->data.obj = NULL;
    }
}

// SDL2.Chunk(src): a sample decoded into the open device's format; `src` is a
// path or the file's bytes (any format SDL_mixer's chunk decoders know).
static int PySDL_Chunk_Type_init(PySDL_Chunk *self, PyObject *args, PyObject *kwds) {
    PyObject *src = Py_None;
    static char *kwlist[] = {"src", NULL};
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "|O", kwlist, &src)) {
        return -1;
    }
    _free_chunk(self);
    if(src == Py_None) {
        return 0;  // internal allocation
    }

    Py_buffer view;
    SDL_RWops *rw = PySDL_RWFromObject(src, &view);
    if(NULL == rw) {
        return -1;
    }
    Mix_Chunk *chunk;
    UNLOCKED(chunk = Mix_LoadWAV_RW(rw, 1));  // decodes (copies) the data
    PyBuffer_Release(&view);
    if(NULL == chunk) {
        _raise();
        return -1;
    }
    self->chunk = chunk;
    return 0;
}

static void PySDL_Chunk_Type_dealloc(PySDL_Chunk *self) {
    _free_chunk(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

// Volume(volume=-1) -> previous volume (0..MIX_MAX_VOLUME); -1 only queries.
static PyObject * PySDL_Chunk_Volume(PySDL_Chunk *self, PyObject *args) {
    int volume = -1;
    Mix_Chunk *chunk = _chunk(self);
    if(NULL == chunk || !PyArg_ParseTuple(args, "|i", &volume)) {
        return NULL;
    }
    int previous;
    UNLOCKED(previous = Mix_VolumeChunk(chunk, volume));
    return PyLong_FromLong(previous);
}

static PyObject * PySDL_Chunk_Free(PySDL_Chunk *self, PyObject *ign) {
    _free_chunk(self);
    Py_RETURN_NONE;
}

static PyMethodDef PySDL_Chunk_methods[] = {
    { "Volume", (PyCFunction)PySDL_Chunk_Volume, METH_VARARGS },
    { "Free",   (PyCFunction)PySDL_Chunk_Free,   METH_NOARGS  },
    { NULL }
};

PyTypeObject PySDL_Chunk_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name      = "SDL2.Chunk",
    .tp_basicsize = sizeof(PySDL_Chunk),
    .tp_dealloc   = (destructor)PySDL_Chunk_Type_dealloc,
    .tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc       = "SDL2.Chunk(src): a sound sample from a path or the file's bytes (audio must be open)",
    .tp_methods   = PySDL_Chunk_methods,
    .tp_init      = (initproc)PySDL_Chunk_Type_init,
    .tp_new       = PyType_GenericNew
};

static PySDL_Chunk * _chunk_arg(PyObject *obj) {
    if(!PyObject_TypeCheck(obj, &PySDL_Chunk_Type)) {
        PyErr_SetString(PyExc_TypeError, "expected an SDL2.Chunk");
        return NULL;
    }
    return _chunk((PySDL_Chunk *)obj) ? (PySDL_Chunk *)obj : NULL;
}

// Mix_QuickLoad_RAW(data) -> Chunk playing `data` in place: raw samples already
// in the device's format. The buffer is pinned for the Chunk's lifetime.
static PyObject * PySDL_Mix_QuickLoad_RAW(PyObject *self, PyObject *arg) {
    PySDL_Chunk *wrapper = (PySDL_Chunk *)PySDL_New(&PySDL_Chunk_Type);
    if(NULL == wrapper) {
        return NULL;
    }
    if(0 > PyObject_GetBuffer(arg, &wrapper->data, PyBUF_SIMPLE)) {
        wrapper->data.obj = NULL;
        Py_DECREF(wrapper);
        return NULL;
    }
    if(wrapper->data.len > (Py_ssize_t)0xFFFFFFFF) {
        Py_DECREF(wrapper);
        PyErr_SetString(PyExc_OverflowError, "sample data too large");
        return NULL;
    }
    Mix_Chunk *chunk;
    Uint8 *mem = wrapper->data.buf;
    Uint32 len = (Uint32)wrapper->data.len;
    UNLOCKED(chunk = Mix_QuickLoad_RAW(mem, len));
    if(NULL == chunk) {
        Py_DECREF(wrapper);  // releases the pinned buffer
        return _raise();
    }
    wrapper->chunk = chunk;
    return (PyObject *)wrapper;
}

//=========================================================
// Music
//=========================================================

static int _music_alive(PySDL_Music *self) {
    return NULL != self->music && self->session == _session;
}

static Mix_Music * _music(PySDL_Music *self) {
    if(NULL == self->music) {
        PyErr_SetString(pysdl_Error, "Music is not loaded");
        return NULL;
    }
    if(self->session != _session) {
        PyErr_SetString(pysdl_Error, "Music is no longer valid: SDL2.Mix_Quit() was called");
        return NULL;
    }
    return self->music;
}

static void _free_music(PySDL_Music *self) {
    if(_music_alive(self)) {
        Mix_Music *music = self->music;
        self->music = NULL;
        UNLOCKED(Mix_FreeMusic(music));  // halts it if playing; closes its RWops
    } else if(NULL != self->music) {
        // From before a Mix_Quit: its decoder may be gone, so leak it - and
        // keep the buffer pinned, since the leaked music still points into it.
        self->music = NULL;
        self->data.obj = NULL;
    }
    if(NULL != self->data.obj) {
        PyBuffer_Release(&self->data);
        self->data.obj = NULL;
    }
}

// SDL2.Music(src, type=MUS_NONE): `src` is a path or the file's bytes (pinned:
// music is decoded as it plays). `type` forces a decoder (MUS_OGG, ...);
// MUS_NONE detects it, from the file extension too for a path.
static int PySDL_Music_Type_init(PySDL_Music *self, PyObject *args, PyObject *kwds) {
    PyObject *src = Py_None;
    int type = MUS_NONE;
    static char *kwlist[] = {"src", "type", NULL};
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "|Oi", kwlist, &src, &type)) {
        return -1;
    }
    _free_music(self);
    self->session = _session;
    if(src == Py_None) {
        return 0;
    }

    Mix_Music *music;
    if(MUS_NONE == type && PyUnicode_Check(src)) {
        const char *path = PyUnicode_AsUTF8(src);
        if(NULL == path) {
            return -1;
        }
        UNLOCKED(music = Mix_LoadMUS(path));
    } else {
        SDL_RWops *rw = PySDL_RWFromObject(src, &self->data);
        if(NULL == rw) {
            return -1;
        }
        UNLOCKED(music = Mix_LoadMUSType_RW(rw, (Mix_MusicType)type, 1));
    }
    if(NULL == music) {
        if(NULL != self->data.obj) {
            PyBuffer_Release(&self->data);
            self->data.obj = NULL;
        }
        _raise();
        return -1;
    }
    self->music = music;
    return 0;
}

static void PySDL_Music_Type_dealloc(PySDL_Music *self) {
    _free_music(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject * PySDL_Music_Free(PySDL_Music *self, PyObject *ign) {
    _free_music(self);
    Py_RETURN_NONE;
}

// Play(loops=0): -1 loops forever. Replaces whatever music is playing.
static PyObject * PySDL_Music_Play(PySDL_Music *self, PyObject *args) {
    int loops = 0;
    Mix_Music *music = _music(self);
    if(NULL == music || !PyArg_ParseTuple(args, "|i", &loops)) {
        return NULL;
    }
    int rc;
    UNLOCKED(rc = Mix_PlayMusic(music, loops));
    if(0 > rc) {
        return _raise();
    }
    Py_XSETREF(_playing_music, Py_NewRef((PyObject *)self));
    Py_RETURN_NONE;
}

// FadeIn(ms, loops=0, position=0.0): fade in over `ms`, starting at `position`
// seconds (Mix_FadeInMusicPos).
static PyObject * PySDL_Music_FadeIn(PySDL_Music *self, PyObject *args, PyObject *kwds) {
    int ms, loops = 0;
    double position = 0.0;
    static char *kwlist[] = {"ms", "loops", "position", NULL};
    Mix_Music *music = _music(self);
    if(NULL == music || !PyArg_ParseTupleAndKeywords(args, kwds, "i|id", kwlist, &ms, &loops, &position)) {
        return NULL;
    }
    int rc;
    UNLOCKED(rc = Mix_FadeInMusicPos(music, loops, ms, position));
    if(0 > rc) {
        return _raise();
    }
    Py_XSETREF(_playing_music, Py_NewRef((PyObject *)self));
    Py_RETURN_NONE;
}

static PyObject * PySDL_Music_GetType(PySDL_Music *self, PyObject *ign) {
    Mix_Music *music = _music(self);
    if(NULL == music) return NULL;
    Mix_MusicType type;
    UNLOCKED(type = Mix_GetMusicType(music));
    return PyLong_FromLong(type);
}

#if SDL_MIXER_VERSION_ATLEAST(2,6,0)
#define MUSIC_STR(NAME, CALL) \
    static PyObject * PySDL_Music_##NAME(PySDL_Music *self, PyObject *ign) { \
        Mix_Music *music = _music(self); \
        if(NULL == music) return NULL; \
        const char *text; \
        UNLOCKED(text = CALL(music)); \
        return PyUnicode_FromString(text ? text : ""); \
    }

MUSIC_STR(GetTitle,        Mix_GetMusicTitle)
MUSIC_STR(GetTitleTag,     Mix_GetMusicTitleTag)
MUSIC_STR(GetArtistTag,    Mix_GetMusicArtistTag)
MUSIC_STR(GetAlbumTag,     Mix_GetMusicAlbumTag)
MUSIC_STR(GetCopyrightTag, Mix_GetMusicCopyrightTag)

// Times in seconds; -1.0 means unknown / not supported by the format.
#define MUSIC_TIME(NAME, CALL) \
    static PyObject * PySDL_Music_##NAME(PySDL_Music *self, PyObject *ign) { \
        Mix_Music *music = _music(self); \
        if(NULL == music) return NULL; \
        double seconds; \
        UNLOCKED(seconds = CALL(music)); \
        return PyFloat_FromDouble(seconds); \
    }

MUSIC_TIME(Duration,          Mix_MusicDuration)
MUSIC_TIME(GetPosition,       Mix_GetMusicPosition)
MUSIC_TIME(GetLoopStartTime,  Mix_GetMusicLoopStartTime)
MUSIC_TIME(GetLoopEndTime,    Mix_GetMusicLoopEndTime)
MUSIC_TIME(GetLoopLengthTime, Mix_GetMusicLoopLengthTime)

static PyObject * PySDL_Music_GetVolume(PySDL_Music *self, PyObject *ign) {
    Mix_Music *music = _music(self);
    if(NULL == music) return NULL;
    int volume;
    UNLOCKED(volume = Mix_GetMusicVolume(music));
    return PyLong_FromLong(volume);
}
#endif

#if SDL_MIXER_VERSION_ATLEAST(2,8,0)
static PyObject * PySDL_Music_StartTrack(PySDL_Music *self, PyObject *arg) {
    int track;
    Mix_Music *music = _music(self);
    if(NULL == music || !_int_arg(arg, &track)) {
        return NULL;
    }
    int rc;
    UNLOCKED(rc = Mix_StartTrack(music, track));
    if(0 > rc) {
        return _raise();
    }
    Py_RETURN_NONE;
}

static PyObject * PySDL_Music_GetNumTracks(PySDL_Music *self, PyObject *ign) {
    Mix_Music *music = _music(self);
    if(NULL == music) return NULL;
    int count;
    UNLOCKED(count = Mix_GetNumTracks(music));
    return PyLong_FromLong(count);
}
#endif

static PyMethodDef PySDL_Music_methods[] = {
    { "Free",    (PyCFunction)PySDL_Music_Free,    METH_NOARGS  },
    { "Play",    (PyCFunction)PySDL_Music_Play,    METH_VARARGS },
    { "FadeIn",  (PyCFunction)PySDL_Music_FadeIn,  METH_VARARGS | METH_KEYWORDS },
    { "GetType", (PyCFunction)PySDL_Music_GetType, METH_NOARGS  },
#if SDL_MIXER_VERSION_ATLEAST(2,6,0)
    { "GetTitle",          (PyCFunction)PySDL_Music_GetTitle,          METH_NOARGS },
    { "GetTitleTag",       (PyCFunction)PySDL_Music_GetTitleTag,       METH_NOARGS },
    { "GetArtistTag",      (PyCFunction)PySDL_Music_GetArtistTag,      METH_NOARGS },
    { "GetAlbumTag",       (PyCFunction)PySDL_Music_GetAlbumTag,       METH_NOARGS },
    { "GetCopyrightTag",   (PyCFunction)PySDL_Music_GetCopyrightTag,   METH_NOARGS },
    { "Duration",          (PyCFunction)PySDL_Music_Duration,          METH_NOARGS },
    { "GetPosition",       (PyCFunction)PySDL_Music_GetPosition,       METH_NOARGS },
    { "GetLoopStartTime",  (PyCFunction)PySDL_Music_GetLoopStartTime,  METH_NOARGS },
    { "GetLoopEndTime",    (PyCFunction)PySDL_Music_GetLoopEndTime,    METH_NOARGS },
    { "GetLoopLengthTime", (PyCFunction)PySDL_Music_GetLoopLengthTime, METH_NOARGS },
    { "GetVolume",         (PyCFunction)PySDL_Music_GetVolume,         METH_NOARGS },
#endif
#if SDL_MIXER_VERSION_ATLEAST(2,8,0)
    { "StartTrack",   (PyCFunction)PySDL_Music_StartTrack,   METH_O      },
    { "GetNumTracks", (PyCFunction)PySDL_Music_GetNumTracks, METH_NOARGS },
#endif
    { NULL }
};

PyTypeObject PySDL_Music_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name      = "SDL2.Music",
    .tp_basicsize = sizeof(PySDL_Music),
    .tp_dealloc   = (destructor)PySDL_Music_Type_dealloc,
    .tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc       = "SDL2.Music(src, type=MUS_NONE): music from a path or the file's bytes",
    .tp_methods   = PySDL_Music_methods,
    .tp_init      = (initproc)PySDL_Music_Type_init,
    .tp_new       = PyType_GenericNew
};

//=========================================================
// init / device
//=========================================================

// Mix_Init(flags=0) -> the decoder flags now loaded; raises if any requested
// one could not be (0 just queries).
static PyObject * PySDL_Mix_Init(PyObject *self, PyObject *args) {
    int flags = 0;
    if(!PyArg_ParseTuple(args, "|i", &flags)) {
        return NULL;
    }
    int got;
    UNLOCKED(got = Mix_Init(flags));
    if((got & flags) != flags) {
        return _raise();
    }
    return PyLong_FromLong(got);
}

// Unloads decoder libraries: Music loaded before this becomes invalid.
static PyObject * PySDL_Mix_Quit(PyObject *self, PyObject *ign) {
    UNLOCKED(Mix_Quit());
    ++_session;
    Py_RETURN_NONE;
}

static PyObject * PySDL_Mix_Linked_Version(PyObject *self, PyObject *ign) {
    const SDL_version *v = Mix_Linked_Version();
    return Py_BuildValue("(iii)", v->major, v->minor, v->patch);
}

// Mix_OpenAudio(frequency=MIX_DEFAULT_FREQUENCY, format=MIX_DEFAULT_FORMAT,
//               channels=2, chunksize=2048, device=None, allowed_changes=0)
static PyObject * PySDL_Mix_OpenAudio(PyObject *self, PyObject *args, PyObject *kwds) {
    int frequency = MIX_DEFAULT_FREQUENCY, channels = MIX_DEFAULT_CHANNELS, chunksize = 2048;
    int allowed_changes = 0;
    unsigned int format = MIX_DEFAULT_FORMAT;
    const char *device = NULL;
    static char *kwlist[] = {"frequency", "format", "channels", "chunksize", "device", "allowed_changes", NULL};
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "|iIiizi", kwlist,
        &frequency, &format, &channels, &chunksize, &device, &allowed_changes)) {
        return NULL;
    }
    int rc;
#if SDL_MIXER_VERSION_ATLEAST(2,0,2)
    UNLOCKED(rc = Mix_OpenAudioDevice(frequency, (Uint16)format, channels, chunksize, device, allowed_changes));
#else
    UNLOCKED(rc = Mix_OpenAudio(frequency, (Uint16)format, channels, chunksize));
#endif
    if(0 > rc) {
        return _raise();
    }
    Py_RETURN_NONE;
}

// Opens are counted; once the last one closes, playing objects are released.
static PyObject * PySDL_Mix_CloseAudio(PyObject *self, PyObject *ign) {
    int still_open;
    UNLOCKED(Mix_CloseAudio(); still_open = Mix_QuerySpec(NULL, NULL, NULL));
    if(!still_open) {
        _forget_chunks_from(0);
        Py_CLEAR(_playing_music);
    }
    Py_RETURN_NONE;
}

// -> (frequency, format, channels) of the open device
static PyObject * PySDL_Mix_QuerySpec(PyObject *self, PyObject *ign) {
    int frequency = 0, channels = 0;
    Uint16 format = 0;
    int opened;
    UNLOCKED(opened = Mix_QuerySpec(&frequency, &format, &channels));
    if(0 == opened) {
        PyErr_SetString(pysdl_Error, "audio is not open (call SDL2.Mix_OpenAudio)");
        return NULL;
    }
    return Py_BuildValue("(iii)", frequency, format, channels);
}

#if SDL_MIXER_VERSION_ATLEAST(2,8,0)
static PyObject * PySDL_Mix_PauseAudio(PyObject *self, PyObject *arg) {
    int pause = PyObject_IsTrue(arg);
    if(-1 == pause) {
        return NULL;
    }
    UNLOCKED(Mix_PauseAudio(pause));
    Py_RETURN_NONE;
}
#endif

// Mix_AllocateChannels(count=-1) -> the number of channels (-1 only queries).
static PyObject * PySDL_Mix_AllocateChannels(PyObject *self, PyObject *args) {
    int count = -1;
    if(!PyArg_ParseTuple(args, "|i", &count)) {
        return NULL;
    }
    int now;
    UNLOCKED(now = Mix_AllocateChannels(count));
    _forget_chunks_from(now);
    return PyLong_FromLong(now);
}

//=========================================================
// decoders / sound fonts / misc
//=========================================================

#define DECODER_FUNCS(KIND) \
    static PyObject * PySDL_Mix_GetNum##KIND##Decoders(PyObject *self, PyObject *ign) { \
        return PyLong_FromLong(Mix_GetNum##KIND##Decoders()); \
    } \
    static PyObject * PySDL_Mix_Get##KIND##Decoder(PyObject *self, PyObject *arg) { \
        int index; \
        if(!_int_arg(arg, &index)) return NULL; \
        const char *name = Mix_Get##KIND##Decoder(index); \
        if(NULL == name) Py_RETURN_NONE; \
        return PyUnicode_FromString(name); \
    }

DECODER_FUNCS(Chunk)
DECODER_FUNCS(Music)

#if SDL_MIXER_VERSION_ATLEAST(2,0,2)
static PyObject * PySDL_Mix_HasChunkDecoder(PyObject *self, PyObject *arg) {
    const char *name = PyUnicode_AsUTF8(arg);
    return name ? PyBool_FromLong(Mix_HasChunkDecoder(name)) : NULL;
}
#endif

#if SDL_MIXER_VERSION_ATLEAST(2,6,0)
static PyObject * PySDL_Mix_HasMusicDecoder(PyObject *self, PyObject *arg) {
    const char *name = PyUnicode_AsUTF8(arg);
    return name ? PyBool_FromLong(Mix_HasMusicDecoder(name)) : NULL;
}
#endif

// Mix_SetSoundFonts(paths): a ';'-separated list for MIDI, or None to reset.
static PyObject * PySDL_Mix_SetSoundFonts(PyObject *self, PyObject *arg) {
    const char *paths = NULL;
    if(arg != Py_None && NULL == (paths = PyUnicode_AsUTF8(arg))) {
        return NULL;
    }
    int ok;
    UNLOCKED(ok = Mix_SetSoundFonts(paths));
    if(!ok) {
        return _raise();
    }
    Py_RETURN_NONE;
}

static PyObject * PySDL_Mix_GetSoundFonts(PyObject *self, PyObject *ign) {
    const char *paths = Mix_GetSoundFonts();
    if(NULL == paths) {
        Py_RETURN_NONE;
    }
    return PyUnicode_FromString(paths);
}

static int SDLCALL _c_each_soundfont(const char *path, void *data) {
    // Synchronous, on the calling thread, which holds the GIL.
    PyObject *result = PyObject_CallFunction((PyObject *)data, "s", path);
    if(NULL == result) {
        return 1;  // stop; the exception propagates
    }
    int stop = PyObject_IsTrue(result);
    Py_DECREF(result);
    return stop != 0;  // -1 (error) also stops
}

// Mix_EachSoundFont(callback): callback(path) for each sound font until it
// returns true; -> whether it ever did.
static PyObject * PySDL_Mix_EachSoundFont(PyObject *self, PyObject *arg) {
    if(!PyCallable_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "expected a callable");
        return NULL;
    }
    int found = Mix_EachSoundFont(_c_each_soundfont, arg);
    if(PyErr_Occurred()) {
        return NULL;
    }
    return PyBool_FromLong(found);
}

#if SDL_MIXER_VERSION_ATLEAST(2,6,0)
static PyObject * PySDL_Mix_SetTimidityCfg(PyObject *self, PyObject *arg) {
    const char *path = PyUnicode_AsUTF8(arg);
    if(NULL == path) {
        return NULL;
    }
    int ok;
    UNLOCKED(ok = Mix_SetTimidityCfg(path));
    if(!ok) {
        return _raise();
    }
    Py_RETURN_NONE;
}

static PyObject * PySDL_Mix_GetTimidityCfg(PyObject *self, PyObject *ign) {
    const char *path = Mix_GetTimidityCfg();
    if(NULL == path) {
        Py_RETURN_NONE;
    }
    return PyUnicode_FromString(path);
}
#endif

// Mix_SetMusicCMD(command): play music through an external command, or None.
static PyObject * PySDL_Mix_SetMusicCMD(PyObject *self, PyObject *arg) {
    const char *command = NULL;
    if(arg != Py_None && NULL == (command = PyUnicode_AsUTF8(arg))) {
        return NULL;
    }
    int rc;
    UNLOCKED(rc = Mix_SetMusicCMD(command));
    if(0 > rc) {
        return _raise();
    }
    Py_RETURN_NONE;
}

static PyObject * PySDL_Mix_SetSynchroValue(PyObject *self, PyObject *arg) {
    int value;
    if(!_int_arg(arg, &value)) {
        return NULL;
    }
    int rc;
    UNLOCKED(rc = Mix_SetSynchroValue(value));
    if(0 > rc) {
        return _raise();
    }
    Py_RETURN_NONE;
}

static PyObject * PySDL_Mix_GetSynchroValue(PyObject *self, PyObject *ign) {
    int value;
    UNLOCKED(value = Mix_GetSynchroValue());
    return PyLong_FromLong(value);
}

//=========================================================
// channels
//=========================================================

// Mix_PlayChannel(channel, chunk, loops=0, ticks=-1) -> the channel used
// (channel -1 picks a free one). ticks limits the play time in ms.
static PyObject * PySDL_Mix_PlayChannel(PyObject *self, PyObject *args, PyObject *kwds) {
    int channel, loops = 0, ticks = -1;
    PyObject *chunk_py;
    static char *kwlist[] = {"channel", "chunk", "loops", "ticks", NULL};
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "iO|ii", kwlist, &channel, &chunk_py, &loops, &ticks)) {
        return NULL;
    }
    PySDL_Chunk *chunk = _chunk_arg(chunk_py);
    if(NULL == chunk) {
        return NULL;
    }
    Mix_Chunk *mc = chunk->chunk;
    int used;
    UNLOCKED(used = Mix_PlayChannelTimed(channel, mc, loops, ticks));
    if(0 > used) {
        return _raise();
    }
    if(0 > _remember_chunk(used, chunk_py)) {
        return NULL;
    }
    return PyLong_FromLong(used);
}

// Mix_FadeInChannel(channel, chunk, loops, ms, ticks=-1) -> the channel used
static PyObject * PySDL_Mix_FadeInChannel(PyObject *self, PyObject *args, PyObject *kwds) {
    int channel, loops, ms, ticks = -1;
    PyObject *chunk_py;
    static char *kwlist[] = {"channel", "chunk", "loops", "ms", "ticks", NULL};
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "iOii|i", kwlist, &channel, &chunk_py, &loops, &ms, &ticks)) {
        return NULL;
    }
    PySDL_Chunk *chunk = _chunk_arg(chunk_py);
    if(NULL == chunk) {
        return NULL;
    }
    Mix_Chunk *mc = chunk->chunk;
    int used;
    UNLOCKED(used = Mix_FadeInChannelTimed(channel, mc, loops, ms, ticks));
    if(0 > used) {
        return _raise();
    }
    if(0 > _remember_chunk(used, chunk_py)) {
        return NULL;
    }
    return PyLong_FromLong(used);
}

// Mix_GetChunk(channel) -> the Chunk playing (or last played) there, or None
static PyObject * PySDL_Mix_GetChunk(PyObject *self, PyObject *arg) {
    int channel;
    if(!_int_arg(arg, &channel)) {
        return NULL;
    }
    Mix_Chunk *chunk;
    UNLOCKED(chunk = Mix_GetChunk(channel));
    if(NULL != chunk && NULL != _channel_chunks && channel >= 0 && channel < PyList_GET_SIZE(_channel_chunks)) {
        PyObject *known = PyList_GET_ITEM(_channel_chunks, channel);
        if(known != Py_None && ((PySDL_Chunk *)known)->chunk == chunk) {
            return Py_NewRef(known);
        }
    }
    Py_RETURN_NONE;
}

// Mix_Volume(channel, volume=-1) -> previous volume (channel -1: average of all)
static PyObject * PySDL_Mix_Volume(PyObject *self, PyObject *args) {
    int channel, volume = -1;
    if(!PyArg_ParseTuple(args, "i|i", &channel, &volume)) {
        return NULL;
    }
    int previous;
    UNLOCKED(previous = Mix_Volume(channel, volume));
    return PyLong_FromLong(previous);
}

// One-int-argument channel / group calls returning an int.
#define INT_FUNC(NAME, CALL) \
    static PyObject * PySDL_##NAME(PyObject *self, PyObject *arg) { \
        int value; \
        if(!_int_arg(arg, &value)) return NULL; \
        int result; \
        UNLOCKED(result = CALL(value)); \
        return PyLong_FromLong(result); \
    }

INT_FUNC(Mix_Playing,         Mix_Playing)
INT_FUNC(Mix_Paused,          Mix_Paused)
INT_FUNC(Mix_FadingChannel,   Mix_FadingChannel)
INT_FUNC(Mix_ReserveChannels, Mix_ReserveChannels)
INT_FUNC(Mix_GroupAvailable,  Mix_GroupAvailable)
INT_FUNC(Mix_GroupCount,      Mix_GroupCount)
INT_FUNC(Mix_GroupOldest,     Mix_GroupOldest)
INT_FUNC(Mix_GroupNewer,      Mix_GroupNewer)
INT_FUNC(Mix_HaltGroup,       Mix_HaltGroup)

#define VOID_FUNC(NAME, CALL) \
    static PyObject * PySDL_##NAME(PyObject *self, PyObject *arg) { \
        int value; \
        if(!_int_arg(arg, &value)) return NULL; \
        UNLOCKED(CALL(value)); \
        Py_RETURN_NONE; \
    }

VOID_FUNC(Mix_HaltChannel, Mix_HaltChannel)
VOID_FUNC(Mix_Pause,       Mix_Pause)
VOID_FUNC(Mix_Resume,      Mix_Resume)

// Two-int-argument calls returning an int.
#define INT2_FUNC(NAME, CALL) \
    static PyObject * PySDL_##NAME(PyObject *self, PyObject *args) { \
        int a, b; \
        if(!PyArg_ParseTuple(args, "ii", &a, &b)) return NULL; \
        int result; \
        UNLOCKED(result = CALL(a, b)); \
        return PyLong_FromLong(result); \
    }

INT2_FUNC(Mix_ExpireChannel,  Mix_ExpireChannel)   // (channel, ticks)
INT2_FUNC(Mix_FadeOutChannel, Mix_FadeOutChannel)  // (channel, ms)
INT2_FUNC(Mix_FadeOutGroup,   Mix_FadeOutGroup)    // (tag, ms)

static PyObject * PySDL_Mix_GroupChannel(PyObject *self, PyObject *args) {
    int which, tag;
    if(!PyArg_ParseTuple(args, "ii", &which, &tag)) {
        return NULL;
    }
    int ok;
    UNLOCKED(ok = Mix_GroupChannel(which, tag));
    return PyBool_FromLong(ok);
}

// -> the number of channels tagged
static PyObject * PySDL_Mix_GroupChannels(PyObject *self, PyObject *args) {
    int from, to, tag;
    if(!PyArg_ParseTuple(args, "iii", &from, &to, &tag)) {
        return NULL;
    }
    int count;
    UNLOCKED(count = Mix_GroupChannels(from, to, tag));
    return PyLong_FromLong(count);
}

//=========================================================
// positional effects
//=========================================================

static PyObject * _effect_result(int ok) {
    if(!ok) {
        return _raise();
    }
    Py_RETURN_NONE;
}

static PyObject * PySDL_Mix_SetPanning(PyObject *self, PyObject *args) {
    int channel, left, right;
    if(!PyArg_ParseTuple(args, "iii", &channel, &left, &right)) {
        return NULL;
    }
    int ok;
    UNLOCKED(ok = Mix_SetPanning(channel, (Uint8)left, (Uint8)right));
    return _effect_result(ok);
}

// Mix_SetPosition(channel, angle, distance): angle in degrees (0 = front),
// distance 0 (near) .. 255 (far).
static PyObject * PySDL_Mix_SetPosition(PyObject *self, PyObject *args) {
    int channel, angle, distance;
    if(!PyArg_ParseTuple(args, "iii", &channel, &angle, &distance)) {
        return NULL;
    }
    int ok;
    UNLOCKED(ok = Mix_SetPosition(channel, (Sint16)angle, (Uint8)distance));
    return _effect_result(ok);
}

static PyObject * PySDL_Mix_SetDistance(PyObject *self, PyObject *args) {
    int channel, distance;
    if(!PyArg_ParseTuple(args, "ii", &channel, &distance)) {
        return NULL;
    }
    int ok;
    UNLOCKED(ok = Mix_SetDistance(channel, (Uint8)distance));
    return _effect_result(ok);
}

static PyObject * PySDL_Mix_SetReverseStereo(PyObject *self, PyObject *args) {
    int channel, flip;
    if(!PyArg_ParseTuple(args, "ip", &channel, &flip)) {
        return NULL;
    }
    int ok;
    UNLOCKED(ok = Mix_SetReverseStereo(channel, flip));
    return _effect_result(ok);
}

//=========================================================
// music (global)
//=========================================================

static PyObject * PySDL_Mix_VolumeMusic(PyObject *self, PyObject *args) {
    int volume = -1;
    if(!PyArg_ParseTuple(args, "|i", &volume)) {
        return NULL;
    }
    int previous;
    UNLOCKED(previous = Mix_VolumeMusic(volume));
    return PyLong_FromLong(previous);
}

#if SDL_MIXER_VERSION_ATLEAST(2,6,0)
static PyObject * PySDL_Mix_MasterVolume(PyObject *self, PyObject *args) {
    int volume = -1;
    if(!PyArg_ParseTuple(args, "|i", &volume)) {
        return NULL;
    }
    int previous;
    UNLOCKED(previous = Mix_MasterVolume(volume));
    return PyLong_FromLong(previous);
}

static PyObject * PySDL_Mix_ModMusicJumpToOrder(PyObject *self, PyObject *arg) {
    int order;
    if(!_int_arg(arg, &order)) {
        return NULL;
    }
    int rc;
    UNLOCKED(rc = Mix_ModMusicJumpToOrder(order));
    if(0 > rc) {
        return _raise();
    }
    Py_RETURN_NONE;
}
#endif

#define NOARG_INT_FUNC(NAME, CALL) \
    static PyObject * PySDL_##NAME(PyObject *self, PyObject *ign) { \
        int result; \
        UNLOCKED(result = CALL()); \
        return PyLong_FromLong(result); \
    }

NOARG_INT_FUNC(Mix_HaltMusic,    Mix_HaltMusic)
NOARG_INT_FUNC(Mix_FadingMusic,  Mix_FadingMusic)
NOARG_INT_FUNC(Mix_PausedMusic,  Mix_PausedMusic)
NOARG_INT_FUNC(Mix_PlayingMusic, Mix_PlayingMusic)

#define NOARG_VOID_FUNC(NAME, CALL) \
    static PyObject * PySDL_##NAME(PyObject *self, PyObject *ign) { \
        UNLOCKED(CALL()); \
        Py_RETURN_NONE; \
    }

NOARG_VOID_FUNC(Mix_PauseMusic,  Mix_PauseMusic)
NOARG_VOID_FUNC(Mix_ResumeMusic, Mix_ResumeMusic)
NOARG_VOID_FUNC(Mix_RewindMusic, Mix_RewindMusic)

static PyObject * PySDL_Mix_FadeOutMusic(PyObject *self, PyObject *arg) {
    int ms;
    if(!_int_arg(arg, &ms)) {
        return NULL;
    }
    int ok;
    UNLOCKED(ok = Mix_FadeOutMusic(ms));
    return PyBool_FromLong(ok);
}

// Mix_SetMusicPosition(position): seconds for most formats (pattern order for MOD)
static PyObject * PySDL_Mix_SetMusicPosition(PyObject *self, PyObject *arg) {
    double position = PyFloat_AsDouble(arg);
    if(-1.0 == position && PyErr_Occurred()) {
        return NULL;
    }
    int rc;
    UNLOCKED(rc = Mix_SetMusicPosition(position));
    if(0 > rc) {
        return _raise();
    }
    Py_RETURN_NONE;
}

//=========================================================
// callbacks (audio thread)
//=========================================================

static PyObject *_channel_finished;  // callback(channel)
static PyObject *_music_finished;    // callback()
static PyObject *_postmix;           // callback(data: bytes) -> bytes | None
static PyObject *_music_hook;        // callback(nbytes) -> bytes

// Call `slot`'s callable with a reference of our own (it may replace itself).
static PyObject * _call_slot(PyObject *slot, PyObject *args) {
    Py_INCREF(slot);
    PyObject *result = PyObject_CallObject(slot, args);
    Py_DECREF(slot);
    return result;
}

// Copy a callback's bytes result over `stream` when it is exactly `len` long.
static void _copy_back(PyObject *result, Uint8 *stream, int len, const char *who) {
    if(result == Py_None) {
        return;
    }
    if(PyBytes_Check(result) && PyBytes_GET_SIZE(result) == len) {
        SDL_memcpy(stream, PyBytes_AS_STRING(result), (size_t)len);
    } else {
        PyErr_Format(PyExc_ValueError, "%s must return None or bytes of the same length (%d)", who, len);
        PyErr_Print();
    }
}

static void SDLCALL _c_channel_finished(int channel) {
    PyGILState_STATE gil;
    if(!PySDL_ThreadEnter(&gil)) {
        return;
    }
    if(NULL != _channel_finished) {
        PyObject *args = Py_BuildValue("(i)", channel);
        PyObject *result = args ? _call_slot(_channel_finished, args) : NULL;
        Py_XDECREF(args);
        if(NULL == result) {
            PyErr_Print();
        }
        Py_XDECREF(result);
    }
    PySDL_ThreadLeave(gil);
}

static void SDLCALL _c_music_finished(void) {
    PyGILState_STATE gil;
    if(!PySDL_ThreadEnter(&gil)) {
        return;
    }
    if(NULL != _music_finished) {
        PyObject *result = _call_slot(_music_finished, NULL);
        if(NULL == result) {
            PyErr_Print();
        }
        Py_XDECREF(result);
    }
    PySDL_ThreadLeave(gil);
}

static void SDLCALL _c_postmix(void *udata, Uint8 *stream, int len) {
    PyGILState_STATE gil;
    if(!PySDL_ThreadEnter(&gil)) {
        return;
    }
    if(NULL != _postmix) {
        PyObject *args = Py_BuildValue("(y#)", (const char *)stream, (Py_ssize_t)len);
        PyObject *result = args ? _call_slot(_postmix, args) : NULL;
        Py_XDECREF(args);
        if(NULL == result) {
            PyErr_Print();
        } else {
            _copy_back(result, stream, len, "post-mix callback");
            Py_DECREF(result);
        }
    }
    PySDL_ThreadLeave(gil);
}

// The stream arrives pre-filled with silence; a short result leaves the rest silent.
static void SDLCALL _c_music_hook(void *udata, Uint8 *stream, int len) {
    PyGILState_STATE gil;
    if(!PySDL_ThreadEnter(&gil)) {
        return;
    }
    if(NULL != _music_hook) {
        PyObject *args = Py_BuildValue("(i)", len);
        PyObject *result = args ? _call_slot(_music_hook, args) : NULL;
        Py_XDECREF(args);
        if(NULL == result) {
            PyErr_Print();
        } else {
            if(PyBytes_Check(result)) {
                Py_ssize_t n = PyBytes_GET_SIZE(result);
                SDL_memcpy(stream, PyBytes_AS_STRING(result), (size_t)(n < len ? n : len));
            } else if(result != Py_None) {
                PyErr_SetString(PyExc_TypeError, "music hook must return bytes");
                PyErr_Print();
            }
            Py_DECREF(result);
        }
    }
    PySDL_ThreadLeave(gil);
}

// Store `arg` (a callable or None) in *slot, then install / remove the C
// trampoline with the GIL released: SDL_mixer takes the audio lock to swap it,
// so once `install` returns no call to the old callable is in flight and it
// can be released.
static PyObject * _set_callback(PyObject **slot, PyObject *arg, void (*install)(int on)) {
    if(arg != Py_None && !PyCallable_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "expected a callable or None");
        return NULL;
    }
    PyObject *old = *slot;
    *slot = (arg == Py_None) ? NULL : Py_NewRef(arg);
    int on = (NULL != *slot);
    UNLOCKED(install(on));
    Py_XDECREF(old);
    Py_RETURN_NONE;
}

static void _install_channel_finished(int on) { Mix_ChannelFinished(on ? _c_channel_finished : NULL); }
static void _install_music_finished(int on)   { Mix_HookMusicFinished(on ? _c_music_finished : NULL); }
static void _install_postmix(int on)          { Mix_SetPostMix(on ? _c_postmix : NULL, NULL); }
static void _install_music_hook(int on)       { Mix_HookMusic(on ? _c_music_hook : NULL, NULL); }

// Mix_ChannelFinished(callback): callback(channel) when a channel stops
// (on the audio thread, or on the thread that halted it).
static PyObject * PySDL_Mix_ChannelFinished(PyObject *self, PyObject *arg) {
    return _set_callback(&_channel_finished, arg, _install_channel_finished);
}

static PyObject * PySDL_Mix_HookMusicFinished(PyObject *self, PyObject *arg) {
    return _set_callback(&_music_finished, arg, _install_music_finished);
}

// Mix_SetPostMix(callback): callback(data) sees every final mixed buffer and
// may return replacement bytes of the same length.
static PyObject * PySDL_Mix_SetPostMix(PyObject *self, PyObject *arg) {
    return _set_callback(&_postmix, arg, _install_postmix);
}

// Mix_HookMusic(callback): replace music playback with callback(nbytes) ->
// bytes of samples in the device format.
static PyObject * PySDL_Mix_HookMusic(PyObject *self, PyObject *arg) {
    return _set_callback(&_music_hook, arg, _install_music_hook);
}

// The callable installed by Mix_HookMusic, or None.
static PyObject * PySDL_Mix_GetMusicHookData(PyObject *self, PyObject *ign) {
    return Py_NewRef(_music_hook ? _music_hook : Py_None);
}

//=========================================================
// custom effects
//=========================================================

// SDL_mixer can only unregister an effect by its C function, and every Python
// effect shares one. So each channel with Python effects gets a single
// trampoline registration that runs the channel's list (a dict entry:
// channel -> [(effect, done), ...]) in order. SDL_mixer drops a channel's
// effects when it stops playing; the done trampoline then empties the list.
static PyObject *_effects;

static PyObject * _effect_list(int channel, int create) {
    if(NULL == _effects) {
        if(!create || NULL == (_effects = PyDict_New())) {
            return NULL;
        }
    }
    PyObject *key = PyLong_FromLong(channel);
    if(NULL == key) {
        return NULL;
    }
    PyObject *list = PyDict_GetItemWithError(_effects, key);
    if(NULL == list && create && !PyErr_Occurred()) {
        list = PyList_New(0);
        if(NULL != list && 0 > PyDict_SetItem(_effects, key, list)) {
            Py_CLEAR(list);
        }
        Py_XDECREF(list);  // the dict holds it
    }
    Py_DECREF(key);
    return list;  // borrowed
}

static void SDLCALL _c_effect(int channel, void *stream, int len, void *udata) {
    PyGILState_STATE gil;
    if(!PySDL_ThreadEnter(&gil)) {
        return;
    }
    PyObject *list = _effect_list(channel, 0);
    PyObject *snapshot = list ? PySequence_List(list) : NULL;  // effects may unregister themselves
    for(Py_ssize_t idx = 0; snapshot && idx < PyList_GET_SIZE(snapshot); ++idx) {
        PyObject *fn = PyTuple_GET_ITEM(PyList_GET_ITEM(snapshot, idx), 0);
        PyObject *result = PyObject_CallFunction(fn, "iy#", channel, (const char *)stream, (Py_ssize_t)len);
        if(NULL == result) {
            PyErr_Print();
            continue;
        }
        _copy_back(result, stream, len, "effect callback");
        Py_DECREF(result);
    }
    Py_XDECREF(snapshot);
    if(PyErr_Occurred()) {
        PyErr_Print();
    }
    PySDL_ThreadLeave(gil);
}

// SDL_mixer removed this channel's effects (it stopped, or UnregisterAllEffects).
static void SDLCALL _c_effect_done(int channel, void *udata) {
    PyGILState_STATE gil;
    if(!PySDL_ThreadEnter(&gil)) {
        return;
    }
    PyObject *list = _effect_list(channel, 0);
    PyObject *snapshot = list ? PySequence_List(list) : NULL;
    if(NULL != list) {
        PyList_SetSlice(list, 0, PY_SSIZE_T_MAX, NULL);
    }
    for(Py_ssize_t idx = 0; snapshot && idx < PyList_GET_SIZE(snapshot); ++idx) {
        PyObject *done = PyTuple_GET_ITEM(PyList_GET_ITEM(snapshot, idx), 1);
        if(done != Py_None) {
            PyObject *result = PyObject_CallFunction(done, "i", channel);
            if(NULL == result) {
                PyErr_Print();
            }
            Py_XDECREF(result);
        }
    }
    Py_XDECREF(snapshot);
    if(PyErr_Occurred()) {
        PyErr_Print();
    }
    PySDL_ThreadLeave(gil);
}

// Mix_RegisterEffect(channel, effect, done=None): effect(channel, data) ->
// bytes | None runs on every buffer of that channel (MIX_CHANNEL_POST: the final
// mix); done(channel) when the effect is removed. Effects are dropped when the
// channel stops.
static PyObject * PySDL_Mix_RegisterEffect(PyObject *self, PyObject *args, PyObject *kwds) {
    int channel;
    PyObject *effect, *done = Py_None;
    static char *kwlist[] = {"channel", "effect", "done", NULL};
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "iO|O", kwlist, &channel, &effect, &done)) {
        return NULL;
    }
    if(!PyCallable_Check(effect) || (done != Py_None && !PyCallable_Check(done))) {
        PyErr_SetString(PyExc_TypeError, "effect (and done, if given) must be callable");
        return NULL;
    }
    PyObject *list = _effect_list(channel, 1);
    if(NULL == list) {
        return NULL;
    }
    PyObject *entry = PyTuple_Pack(2, effect, done);
    if(NULL == entry || 0 > PyList_Append(list, entry)) {
        Py_XDECREF(entry);
        return NULL;
    }
    Py_DECREF(entry);
    if(1 == PyList_GET_SIZE(list)) {  // first Python effect here: hook the channel
        int ok;
        UNLOCKED(ok = Mix_RegisterEffect(channel, _c_effect, _c_effect_done, NULL));
        if(!ok) {
            PyList_SetSlice(list, 0, PY_SSIZE_T_MAX, NULL);
            return _raise();
        }
    }
    Py_RETURN_NONE;
}

// Mix_UnregisterEffect(channel, effect): remove one registration (by identity);
// its done(channel) runs now.
static PyObject * PySDL_Mix_UnregisterEffect(PyObject *self, PyObject *args) {
    int channel;
    PyObject *effect;
    if(!PyArg_ParseTuple(args, "iO", &channel, &effect)) {
        return NULL;
    }
    PyObject *list = _effect_list(channel, 0);
    Py_ssize_t count = list ? PyList_GET_SIZE(list) : 0;
    for(Py_ssize_t idx = 0; idx < count; ++idx) {
        PyObject *entry = PyList_GET_ITEM(list, idx);
        if(PyTuple_GET_ITEM(entry, 0) != effect) {
            continue;
        }
        Py_INCREF(entry);
        PySequence_DelItem(list, idx);
        if(0 == PyList_GET_SIZE(list)) {
            int ok;
            UNLOCKED(ok = Mix_UnregisterEffect(channel, _c_effect));  // its done sees an empty list
            (void)ok;
        }
        PyObject *done = PyTuple_GET_ITEM(entry, 1);
        PyObject *result = (done != Py_None) ? PyObject_CallFunction(done, "i", channel) : Py_NewRef(Py_None);
        Py_DECREF(entry);
        if(NULL == result) {
            return NULL;
        }
        Py_DECREF(result);
        Py_RETURN_NONE;
    }
    PyErr_SetString(PyExc_ValueError, "effect is not registered on that channel");
    return NULL;
}

// Mix_UnregisterAllEffects(channel): every effect on the channel, including the
// positional ones (panning, position, distance, reverse stereo).
static PyObject * PySDL_Mix_UnregisterAllEffects(PyObject *self, PyObject *arg) {
    int channel;
    if(!_int_arg(arg, &channel)) {
        return NULL;
    }
    int ok;
    UNLOCKED(ok = Mix_UnregisterAllEffects(channel));
    if(!ok) {
        return _raise();
    }
    Py_RETURN_NONE;
}

//=========================================================
// method table
//=========================================================

PyMethodDef pysdl_mixer_methods[] = {
    { "Mix_Init",               PySDL_Mix_Init,               METH_VARARGS },
    { "Mix_Quit",               PySDL_Mix_Quit,               METH_NOARGS  },
    { "Mix_Linked_Version",     PySDL_Mix_Linked_Version,     METH_NOARGS  },
    { "Mix_OpenAudio",          (PyCFunction)PySDL_Mix_OpenAudio, METH_VARARGS | METH_KEYWORDS },
    { "Mix_CloseAudio",         PySDL_Mix_CloseAudio,         METH_NOARGS  },
    { "Mix_QuerySpec",          PySDL_Mix_QuerySpec,          METH_NOARGS  },
    { "Mix_AllocateChannels",   PySDL_Mix_AllocateChannels,   METH_VARARGS },
    { "Mix_ReserveChannels",    PySDL_Mix_ReserveChannels,    METH_O       },
    { "Mix_QuickLoad_RAW",      PySDL_Mix_QuickLoad_RAW,      METH_O       },

    { "Mix_GetNumChunkDecoders", PySDL_Mix_GetNumChunkDecoders, METH_NOARGS },
    { "Mix_GetChunkDecoder",     PySDL_Mix_GetChunkDecoder,     METH_O      },
    { "Mix_GetNumMusicDecoders", PySDL_Mix_GetNumMusicDecoders, METH_NOARGS },
    { "Mix_GetMusicDecoder",     PySDL_Mix_GetMusicDecoder,     METH_O      },
    { "Mix_SetSoundFonts",      PySDL_Mix_SetSoundFonts,      METH_O       },
    { "Mix_GetSoundFonts",      PySDL_Mix_GetSoundFonts,      METH_NOARGS  },
    { "Mix_EachSoundFont",      PySDL_Mix_EachSoundFont,      METH_O       },
    { "Mix_SetMusicCMD",        PySDL_Mix_SetMusicCMD,        METH_O       },
    { "Mix_SetSynchroValue",    PySDL_Mix_SetSynchroValue,    METH_O       },
    { "Mix_GetSynchroValue",    PySDL_Mix_GetSynchroValue,    METH_NOARGS  },

    { "Mix_PlayChannel",        (PyCFunction)PySDL_Mix_PlayChannel,   METH_VARARGS | METH_KEYWORDS },
    { "Mix_FadeInChannel",      (PyCFunction)PySDL_Mix_FadeInChannel, METH_VARARGS | METH_KEYWORDS },
    { "Mix_GetChunk",           PySDL_Mix_GetChunk,           METH_O       },
    { "Mix_Volume",             PySDL_Mix_Volume,             METH_VARARGS },
    { "Mix_HaltChannel",        PySDL_Mix_HaltChannel,        METH_O       },
    { "Mix_ExpireChannel",      PySDL_Mix_ExpireChannel,      METH_VARARGS },
    { "Mix_FadeOutChannel",     PySDL_Mix_FadeOutChannel,     METH_VARARGS },
    { "Mix_FadingChannel",      PySDL_Mix_FadingChannel,      METH_O       },
    { "Mix_Pause",              PySDL_Mix_Pause,              METH_O       },
    { "Mix_Resume",             PySDL_Mix_Resume,             METH_O       },
    { "Mix_Paused",             PySDL_Mix_Paused,             METH_O       },
    { "Mix_Playing",            PySDL_Mix_Playing,            METH_O       },

    { "Mix_GroupChannel",       PySDL_Mix_GroupChannel,       METH_VARARGS },
    { "Mix_GroupChannels",      PySDL_Mix_GroupChannels,      METH_VARARGS },
    { "Mix_GroupAvailable",     PySDL_Mix_GroupAvailable,     METH_O       },
    { "Mix_GroupCount",         PySDL_Mix_GroupCount,         METH_O       },
    { "Mix_GroupOldest",        PySDL_Mix_GroupOldest,        METH_O       },
    { "Mix_GroupNewer",         PySDL_Mix_GroupNewer,         METH_O       },
    { "Mix_HaltGroup",          PySDL_Mix_HaltGroup,          METH_O       },
    { "Mix_FadeOutGroup",       PySDL_Mix_FadeOutGroup,       METH_VARARGS },

    { "Mix_SetPanning",         PySDL_Mix_SetPanning,         METH_VARARGS },
    { "Mix_SetPosition",        PySDL_Mix_SetPosition,        METH_VARARGS },
    { "Mix_SetDistance",        PySDL_Mix_SetDistance,        METH_VARARGS },
    { "Mix_SetReverseStereo",   PySDL_Mix_SetReverseStereo,   METH_VARARGS },
    { "Mix_RegisterEffect",     (PyCFunction)PySDL_Mix_RegisterEffect, METH_VARARGS | METH_KEYWORDS },
    { "Mix_UnregisterEffect",   PySDL_Mix_UnregisterEffect,   METH_VARARGS },
    { "Mix_UnregisterAllEffects", PySDL_Mix_UnregisterAllEffects, METH_O   },

    { "Mix_VolumeMusic",        PySDL_Mix_VolumeMusic,        METH_VARARGS },
    { "Mix_HaltMusic",          PySDL_Mix_HaltMusic,          METH_NOARGS  },
    { "Mix_FadeOutMusic",       PySDL_Mix_FadeOutMusic,       METH_O       },
    { "Mix_FadingMusic",        PySDL_Mix_FadingMusic,        METH_NOARGS  },
    { "Mix_PauseMusic",         PySDL_Mix_PauseMusic,         METH_NOARGS  },
    { "Mix_ResumeMusic",        PySDL_Mix_ResumeMusic,        METH_NOARGS  },
    { "Mix_RewindMusic",        PySDL_Mix_RewindMusic,        METH_NOARGS  },
    { "Mix_PausedMusic",        PySDL_Mix_PausedMusic,        METH_NOARGS  },
    { "Mix_PlayingMusic",       PySDL_Mix_PlayingMusic,       METH_NOARGS  },
    { "Mix_SetMusicPosition",   PySDL_Mix_SetMusicPosition,   METH_O       },

    { "Mix_ChannelFinished",    PySDL_Mix_ChannelFinished,    METH_O       },
    { "Mix_HookMusicFinished",  PySDL_Mix_HookMusicFinished,  METH_O       },
    { "Mix_SetPostMix",         PySDL_Mix_SetPostMix,         METH_O       },
    { "Mix_HookMusic",          PySDL_Mix_HookMusic,          METH_O       },
    { "Mix_GetMusicHookData",   PySDL_Mix_GetMusicHookData,   METH_NOARGS  },
#if SDL_MIXER_VERSION_ATLEAST(2,0,2)
    { "Mix_HasChunkDecoder",    PySDL_Mix_HasChunkDecoder,    METH_O       },
#endif
#if SDL_MIXER_VERSION_ATLEAST(2,6,0)
    { "Mix_HasMusicDecoder",    PySDL_Mix_HasMusicDecoder,    METH_O       },
    { "Mix_MasterVolume",       PySDL_Mix_MasterVolume,       METH_VARARGS },
    { "Mix_ModMusicJumpToOrder", PySDL_Mix_ModMusicJumpToOrder, METH_O     },
    { "Mix_SetTimidityCfg",     PySDL_Mix_SetTimidityCfg,     METH_O       },
    { "Mix_GetTimidityCfg",     PySDL_Mix_GetTimidityCfg,     METH_NOARGS  },
#endif
#if SDL_MIXER_VERSION_ATLEAST(2,8,0)
    { "Mix_PauseAudio",         PySDL_Mix_PauseAudio,         METH_O       },
#endif
    { NULL }
};

#endif // PYSDL_HAVE_MIXER
