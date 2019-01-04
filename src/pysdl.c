#include "pysdl.h"

PyObject *pysdl_Error;

static PyObject * PySDL_GetCurrentVideoDriver (PyObject*, PyObject*);
static PyObject * PySDL_GetTicks              (PyObject*, PyObject*);
static PyObject * PySDL_GetVideoDrivers       (PyObject*, PyObject*);
static PyObject * PySDL_Init                  (PyObject*, PyObject*);
static PyObject * PySDL_LoadBMP               (PyObject*, PyObject*);
static PyObject * PySDL_Quit                  (PyObject*, PyObject*);
static PyObject * PySDL_ShowCursor            (PyObject*, PyObject*);
static PyObject * PySDL_Version               (PyObject*, PyObject*);
static PyObject * PySDL_GetPlatform           (PyObject*, PyObject*);
static PyObject * PySDL_WasInit               (PyObject*, PyObject*);

static PyObject * PySDL_PollEvent        (PyObject*, PyObject*);
static PyObject * PySDL_WaitEvent        (PyObject*, PyObject*);
static PyObject * PySDL_GetKeyboardState (PyObject*, PyObject*);
static PyObject * PySDL_GetModState      (PyObject*, PyObject*);

static PyObject * PySDL_GetCPUCount         (PyObject*, PyObject*);
static PyObject * PySDL_GetCPUCacheLineSize (PyObject*, PyObject*);
static PyObject * PySDL_Has3DNow            (PyObject*, PyObject*);
static PyObject * PySDL_HasAVX              (PyObject*, PyObject*);
static PyObject * PySDL_HasAVX2             (PyObject*, PyObject*);
static PyObject * PySDL_HasAltiVec          (PyObject*, PyObject*);
static PyObject * PySDL_HasMMX              (PyObject*, PyObject*);
static PyObject * PySDL_HasSSE              (PyObject*, PyObject*);
static PyObject * PySDL_HasSSE2             (PyObject*, PyObject*);
static PyObject * PySDL_HasSSE3             (PyObject*, PyObject*);
static PyObject * PySDL_HasSSE41            (PyObject*, PyObject*);
static PyObject * PySDL_HasSSE42            (PyObject*, PyObject*);


static PyMethodDef pysdl_PyMethodDefs[] = {
    { "GetCurrentVideoDriver", PySDL_GetCurrentVideoDriver, METH_NOARGS  },
    { "GetTicks",              PySDL_GetTicks,              METH_NOARGS  },
    { "GetVideoDrivers",       PySDL_GetVideoDrivers,       METH_NOARGS  },
    { "Init",                  PySDL_Init,                  METH_VARARGS },
    { "LoadBMP",               PySDL_LoadBMP,               METH_O       },
    { "Quit",                  PySDL_Quit,                  METH_NOARGS  },
    { "ShowCursor",            PySDL_ShowCursor,            METH_O       },
    { "Version",               PySDL_Version,               METH_NOARGS  },
    { "GetPlatform",           PySDL_GetPlatform,           METH_NOARGS  },
    { "WasInit",               PySDL_WasInit,               METH_VARARGS },

    { "PollEvent",        PySDL_PollEvent,        METH_NOARGS },
    { "WaitEvent",        PySDL_WaitEvent,        METH_NOARGS },
    { "GetKeyboardState", PySDL_GetKeyboardState, METH_NOARGS },
    { "GetModState",      PySDL_GetModState,      METH_NOARGS },

    { "GetCPUCount",         PySDL_GetCPUCount,         METH_NOARGS },
    { "GetCPUCacheLineSize", PySDL_GetCPUCacheLineSize, METH_NOARGS },
    { "Has3DNow",            PySDL_Has3DNow,            METH_NOARGS },
    { "HasAVX",              PySDL_HasAVX,              METH_NOARGS },
    { "HasAVX2",             PySDL_HasAVX2,             METH_NOARGS },
    { "HasAltiVec",          PySDL_HasAltiVec,          METH_NOARGS },
    { "HasMMX",              PySDL_HasMMX,              METH_NOARGS },
    { "HasSSE",              PySDL_HasSSE,              METH_NOARGS },
    { "HasSSE2",             PySDL_HasSSE2,             METH_NOARGS },
    { "HasSSE3",             PySDL_HasSSE3,             METH_NOARGS },
    { "HasSSE41",            PySDL_HasSSE41,            METH_NOARGS },
    { "HasSSE42",            PySDL_HasSSE42,            METH_NOARGS },
    { NULL }
};

static PyModuleDef pysdl_module = {
    PyModuleDef_HEAD_INIT,
    "SDL2",
    NULL,
    -1,
    pysdl_PyMethodDefs
};


PyMODINIT_FUNC PyInit_SDL2(void) {
    PyObject *module;
    int ok;

    module = PyModule_Create(&pysdl_module);
    if(NULL == module) {
        return NULL;
    }

    pysdl_Error = PyErr_NewException("SDL2.error", NULL, NULL);
    Py_INCREF(pysdl_Error);
    PyModule_AddObject(module, "error", pysdl_Error);

    ok = PyType_Ready(&PySDL_Window_Type);
    if(0 > ok) {
        return NULL;
    }
    Py_INCREF(&PySDL_Window_Type);
    PyModule_AddObject(module, "Window", (PyObject *)&PySDL_Window_Type);

    ok = PyType_Ready(&PySDL_Renderer_Type);
    if(0 > ok) {
        return NULL;
    }
    Py_INCREF(&PySDL_Renderer_Type);

    ok = PyType_Ready(&PySDL_Surface_Type);
    if(0 > ok) {
        return NULL;
    }
    Py_INCREF(&PySDL_Surface_Type);

    ok = PyType_Ready(&PySDL_Texture_Type);
    if(0 > ok) {
        return NULL;
    }
    Py_INCREF(&PySDL_Texture_Type);

    PyModule_AddIntConstant( module, "ENABLE",  SDL_ENABLE  );
    PyModule_AddIntConstant( module, "DISABLE", SDL_DISABLE );
    PyModule_AddIntConstant( module, "QUERY",   SDL_QUERY   );

    PyModule_AddIntConstant( module, "INIT_TIMER",          SDL_INIT_TIMER          );
    PyModule_AddIntConstant( module, "INIT_AUDIO",          SDL_INIT_AUDIO          );
    PyModule_AddIntConstant( module, "INIT_VIDEO",          SDL_INIT_VIDEO          );
    PyModule_AddIntConstant( module, "INIT_JOYSTICK",       SDL_INIT_JOYSTICK       );
    PyModule_AddIntConstant( module, "INIT_HAPTIC",         SDL_INIT_HAPTIC         );
    PyModule_AddIntConstant( module, "INIT_GAMECONTROLLER", SDL_INIT_GAMECONTROLLER );
    PyModule_AddIntConstant( module, "INIT_EVENTS",         SDL_INIT_EVENTS         );
    PyModule_AddIntConstant( module, "INIT_NOPARACHUTE",    SDL_INIT_NOPARACHUTE    );
    PyModule_AddIntConstant( module, "INIT_EVERYTHING",     SDL_INIT_EVERYTHING     );

    PyModule_AddIntConstant( module, "WINDOW_FULLSCREEN",         SDL_WINDOW_FULLSCREEN         );
    PyModule_AddIntConstant( module, "WINDOW_FULLSCREEN_DESKTOP", SDL_WINDOW_FULLSCREEN_DESKTOP );
    PyModule_AddIntConstant( module, "WINDOW_OPENGL",             SDL_WINDOW_OPENGL             );
    PyModule_AddIntConstant( module, "WINDOW_SHOWN",              SDL_WINDOW_SHOWN              );
    PyModule_AddIntConstant( module, "WINDOW_HIDDEN",             SDL_WINDOW_HIDDEN             );
    PyModule_AddIntConstant( module, "WINDOW_BORDERLESS",         SDL_WINDOW_BORDERLESS         );
    PyModule_AddIntConstant( module, "WINDOW_RESIZABLE",          SDL_WINDOW_RESIZABLE          );
    PyModule_AddIntConstant( module, "WINDOW_MINIMIZED",          SDL_WINDOW_MINIMIZED          );
    PyModule_AddIntConstant( module, "WINDOW_MAXIMIZED",          SDL_WINDOW_MAXIMIZED          );
    PyModule_AddIntConstant( module, "WINDOW_INPUT_GRABBED",      SDL_WINDOW_INPUT_GRABBED      );
    PyModule_AddIntConstant( module, "WINDOW_INPUT_FOCUS",        SDL_WINDOW_INPUT_FOCUS        );
    PyModule_AddIntConstant( module, "WINDOW_MOUSE_FOCUS",        SDL_WINDOW_MOUSE_FOCUS        );
    PyModule_AddIntConstant( module, "WINDOW_FOREIGN",            SDL_WINDOW_FOREIGN            );
    PyModule_AddIntConstant( module, "WINDOW_ALLOW_HIGHDPI",      SDL_WINDOW_ALLOW_HIGHDPI      );
    PyModule_AddIntConstant( module, "WINDOW_MOUSE_CAPTURE",      SDL_WINDOW_MOUSE_CAPTURE      );
#ifdef SDL_WINDOW_ALWAYS_ON_TOP
    PyModule_AddIntConstant( module, "WINDOW_ALWAYS_ON_TOP",      SDL_WINDOW_ALWAYS_ON_TOP      );
#endif
#ifdef SDL_WINDOW_SKIP_TASKBAR
    PyModule_AddIntConstant( module, "WINDOW_SKIP_TASKBAR",       SDL_WINDOW_SKIP_TASKBAR       );
#endif
#ifdef SDL_WINDOW_UTILITY
    PyModule_AddIntConstant( module, "WINDOW_UTILITY",            SDL_WINDOW_UTILITY            );
#endif
#ifdef SDL_WINDOW_TOOLTIP
    PyModule_AddIntConstant( module, "WINDOW_TOOLTIP",            SDL_WINDOW_TOOLTIP            );
#endif
#ifdef SDL_WINDOW_POPUP_MENU
    PyModule_AddIntConstant( module, "WINDOW_POPUP_MENU",         SDL_WINDOW_POPUP_MENU         );
#endif

    PyModule_AddIntConstant( module, "RENDERER_SOFTWARE",      SDL_RENDERER_SOFTWARE      );
    PyModule_AddIntConstant( module, "RENDERER_ACCELERATED",   SDL_RENDERER_ACCELERATED   );
    PyModule_AddIntConstant( module, "RENDERER_PRESENTVSYNC",  SDL_RENDERER_PRESENTVSYNC  );
    PyModule_AddIntConstant( module, "RENDERER_TARGETTEXTURE", SDL_RENDERER_TARGETTEXTURE );

    PyModule_AddIntConstant( module, "KEYUP",           SDL_KEYUP           );
    PyModule_AddIntConstant( module, "KEYDOWN",         SDL_KEYDOWN         );
    PyModule_AddIntConstant( module, "MOUSEMOTION",     SDL_MOUSEMOTION     );
    PyModule_AddIntConstant( module, "MOUSEBUTTONDOWN", SDL_MOUSEBUTTONDOWN );
    PyModule_AddIntConstant( module, "MOUSEBUTTONUP",   SDL_MOUSEBUTTONUP   );
    PyModule_AddIntConstant( module, "JOYAXISMOTION",   SDL_JOYAXISMOTION   );
    PyModule_AddIntConstant( module, "JOYBALLMOTION",   SDL_JOYBALLMOTION   );
    PyModule_AddIntConstant( module, "JOYHATMOTION",    SDL_JOYHATMOTION    );
    PyModule_AddIntConstant( module, "JOYBUTTONDOWN",   SDL_JOYBUTTONDOWN   );
    PyModule_AddIntConstant( module, "JOYBUTTONUP",     SDL_JOYBUTTONUP     );
    PyModule_AddIntConstant( module, "QUIT",            SDL_QUIT            );
    PyModule_AddIntConstant( module, "USEREVENT",       SDL_USEREVENT       );

    PyModule_AddIntConstant( module, "PIXELFORMAT_UNKNOWN",     SDL_PIXELFORMAT_UNKNOWN     );
    PyModule_AddIntConstant( module, "PIXELFORMAT_INDEX1LSB",   SDL_PIXELFORMAT_INDEX1LSB   );
    PyModule_AddIntConstant( module, "PIXELFORMAT_INDEX1MSB",   SDL_PIXELFORMAT_INDEX1MSB   );
    PyModule_AddIntConstant( module, "PIXELFORMAT_INDEX4LSB",   SDL_PIXELFORMAT_INDEX4LSB   );
    PyModule_AddIntConstant( module, "PIXELFORMAT_INDEX4MSB",   SDL_PIXELFORMAT_INDEX4MSB   );
    PyModule_AddIntConstant( module, "PIXELFORMAT_INDEX8",      SDL_PIXELFORMAT_INDEX8      );
    PyModule_AddIntConstant( module, "PIXELFORMAT_RGB332",      SDL_PIXELFORMAT_RGB332      );
    PyModule_AddIntConstant( module, "PIXELFORMAT_RGB444",      SDL_PIXELFORMAT_RGB444      );
    PyModule_AddIntConstant( module, "PIXELFORMAT_RGB555",      SDL_PIXELFORMAT_RGB555      );
    PyModule_AddIntConstant( module, "PIXELFORMAT_BGR555",      SDL_PIXELFORMAT_BGR555      );
    PyModule_AddIntConstant( module, "PIXELFORMAT_ARGB4444",    SDL_PIXELFORMAT_ARGB4444    );
    PyModule_AddIntConstant( module, "PIXELFORMAT_RGBA4444",    SDL_PIXELFORMAT_RGBA4444    );
    PyModule_AddIntConstant( module, "PIXELFORMAT_ABGR4444",    SDL_PIXELFORMAT_ABGR4444    );
    PyModule_AddIntConstant( module, "PIXELFORMAT_BGRA4444",    SDL_PIXELFORMAT_BGRA4444    );
    PyModule_AddIntConstant( module, "PIXELFORMAT_ARGB1555",    SDL_PIXELFORMAT_ARGB1555    );
    PyModule_AddIntConstant( module, "PIXELFORMAT_RGBA5551",    SDL_PIXELFORMAT_RGBA5551    );
    PyModule_AddIntConstant( module, "PIXELFORMAT_ABGR1555",    SDL_PIXELFORMAT_ABGR1555    );
    PyModule_AddIntConstant( module, "PIXELFORMAT_BGRA5551",    SDL_PIXELFORMAT_BGRA5551    );
    PyModule_AddIntConstant( module, "PIXELFORMAT_RGB565",      SDL_PIXELFORMAT_RGB565      );
    PyModule_AddIntConstant( module, "PIXELFORMAT_BGR565",      SDL_PIXELFORMAT_BGR565      );
    PyModule_AddIntConstant( module, "PIXELFORMAT_RGB24",       SDL_PIXELFORMAT_RGB24       );
    PyModule_AddIntConstant( module, "PIXELFORMAT_BGR24",       SDL_PIXELFORMAT_BGR24       );
    PyModule_AddIntConstant( module, "PIXELFORMAT_RGB888",      SDL_PIXELFORMAT_RGB888      );
    PyModule_AddIntConstant( module, "PIXELFORMAT_RGBX8888",    SDL_PIXELFORMAT_RGBX8888    );
    PyModule_AddIntConstant( module, "PIXELFORMAT_BGR888",      SDL_PIXELFORMAT_BGR888      );
    PyModule_AddIntConstant( module, "PIXELFORMAT_BGRX8888",    SDL_PIXELFORMAT_BGRX8888    );
    PyModule_AddIntConstant( module, "PIXELFORMAT_ARGB8888",    SDL_PIXELFORMAT_ARGB8888    );
    PyModule_AddIntConstant( module, "PIXELFORMAT_RGBA8888",    SDL_PIXELFORMAT_RGBA8888    );
    PyModule_AddIntConstant( module, "PIXELFORMAT_ABGR8888",    SDL_PIXELFORMAT_ABGR8888    );
    PyModule_AddIntConstant( module, "PIXELFORMAT_BGRA8888",    SDL_PIXELFORMAT_BGRA8888    );
    PyModule_AddIntConstant( module, "PIXELFORMAT_ARGB2101010", SDL_PIXELFORMAT_ARGB2101010 );
    //PyModule_AddIntConstant( module, "PIXELFORMAT_RGBA32",      SDL_PIXELFORMAT_RGBA32      );
    //PyModule_AddIntConstant( module, "PIXELFORMAT_ARGB32",      SDL_PIXELFORMAT_ARGB32      );
    //PyModule_AddIntConstant( module, "PIXELFORMAT_BGRA32",      SDL_PIXELFORMAT_BGRA32      );
    //PyModule_AddIntConstant( module, "PIXELFORMAT_ABGR32",      SDL_PIXELFORMAT_ABGR32      );
    PyModule_AddIntConstant( module, "PIXELFORMAT_YV12",        SDL_PIXELFORMAT_YV12        );
    PyModule_AddIntConstant( module, "PIXELFORMAT_IYUV",        SDL_PIXELFORMAT_IYUV        );
    PyModule_AddIntConstant( module, "PIXELFORMAT_YUY2",        SDL_PIXELFORMAT_YUY2        );
    PyModule_AddIntConstant( module, "PIXELFORMAT_UYVY",        SDL_PIXELFORMAT_UYVY        );
    PyModule_AddIntConstant( module, "PIXELFORMAT_YVYU",        SDL_PIXELFORMAT_YVYU        );
    PyModule_AddIntConstant( module, "PIXELFORMAT_NV12",        SDL_PIXELFORMAT_NV12        );
    PyModule_AddIntConstant( module, "PIXELFORMAT_NV21",        SDL_PIXELFORMAT_NV21        );

    // KEYBOARD SYMBOLS
//    PyModule_AddIntConstant( module, "K_UNKNOWN",      SDLK_UNKNOWN );
//    PyModule_AddIntConstant( module, "K_FIRST",        SDLK_FIRST );
    PyModule_AddIntConstant( module, "K_BACKSPACE",    SDLK_BACKSPACE );
    PyModule_AddIntConstant( module, "K_TAB",          SDLK_TAB );
    PyModule_AddIntConstant( module, "K_CLEAR",        SDLK_CLEAR );
    PyModule_AddIntConstant( module, "K_RETURN",       SDLK_RETURN );
    PyModule_AddIntConstant( module, "K_PAUSE",        SDLK_PAUSE );
    PyModule_AddIntConstant( module, "K_ESCAPE",       SDLK_ESCAPE );
    PyModule_AddIntConstant( module, "K_SPACE",        SDLK_SPACE );
    PyModule_AddIntConstant( module, "K_EXCLAIM",      SDLK_EXCLAIM );
    PyModule_AddIntConstant( module, "K_QUOTEDBL",     SDLK_QUOTEDBL );
    PyModule_AddIntConstant( module, "K_HASH",         SDLK_HASH );
    PyModule_AddIntConstant( module, "K_DOLLAR",       SDLK_DOLLAR );
    PyModule_AddIntConstant( module, "K_AMPERSAND",    SDLK_AMPERSAND );
    PyModule_AddIntConstant( module, "K_QUOTE",        SDLK_QUOTE );
    PyModule_AddIntConstant( module, "K_LEFTPAREN",    SDLK_LEFTPAREN );
    PyModule_AddIntConstant( module, "K_RIGHTPAREN",   SDLK_RIGHTPAREN );
    PyModule_AddIntConstant( module, "K_ASTERISK",     SDLK_ASTERISK );
    PyModule_AddIntConstant( module, "K_PLUS",         SDLK_PLUS );
    PyModule_AddIntConstant( module, "K_COMMA",        SDLK_COMMA );
    PyModule_AddIntConstant( module, "K_MINUS",        SDLK_MINUS );
    PyModule_AddIntConstant( module, "K_PERIOD",       SDLK_PERIOD );
    PyModule_AddIntConstant( module, "K_SLASH",        SDLK_SLASH );
    PyModule_AddIntConstant( module, "K_0",            SDLK_0 );
    PyModule_AddIntConstant( module, "K_1",            SDLK_1 );
    PyModule_AddIntConstant( module, "K_2",            SDLK_2 );
    PyModule_AddIntConstant( module, "K_3",            SDLK_3 );
    PyModule_AddIntConstant( module, "K_4",            SDLK_4 );
    PyModule_AddIntConstant( module, "K_5",            SDLK_5 );
    PyModule_AddIntConstant( module, "K_6",            SDLK_6 );
    PyModule_AddIntConstant( module, "K_7",            SDLK_7 );
    PyModule_AddIntConstant( module, "K_8",            SDLK_8 );
    PyModule_AddIntConstant( module, "K_9",            SDLK_9 );
    PyModule_AddIntConstant( module, "K_COLON",        SDLK_COLON );
    PyModule_AddIntConstant( module, "K_SEMICOLON",    SDLK_SEMICOLON );
    PyModule_AddIntConstant( module, "K_LESS",         SDLK_LESS );
    PyModule_AddIntConstant( module, "K_EQUALS",       SDLK_EQUALS );
    PyModule_AddIntConstant( module, "K_GREATER",      SDLK_GREATER );
    PyModule_AddIntConstant( module, "K_QUESTION",     SDLK_QUESTION );
    PyModule_AddIntConstant( module, "K_AT",           SDLK_AT );
    PyModule_AddIntConstant( module, "K_LEFTBRACKET",  SDLK_LEFTBRACKET );
    PyModule_AddIntConstant( module, "K_BACKSLASH",    SDLK_BACKSLASH );
    PyModule_AddIntConstant( module, "K_RIGHTBRACKET", SDLK_RIGHTBRACKET );
    PyModule_AddIntConstant( module, "K_CARET",        SDLK_CARET );
    PyModule_AddIntConstant( module, "K_UNDERSCORE",   SDLK_UNDERSCORE );
    PyModule_AddIntConstant( module, "K_BACKQUOTE",    SDLK_BACKQUOTE );
    PyModule_AddIntConstant( module, "K_a",            SDLK_a );
    PyModule_AddIntConstant( module, "K_b",            SDLK_b );
    PyModule_AddIntConstant( module, "K_c",            SDLK_c );
    PyModule_AddIntConstant( module, "K_d",            SDLK_d );
    PyModule_AddIntConstant( module, "K_e",            SDLK_e );
    PyModule_AddIntConstant( module, "K_f",            SDLK_f );
    PyModule_AddIntConstant( module, "K_g",            SDLK_g );
    PyModule_AddIntConstant( module, "K_h",            SDLK_h );
    PyModule_AddIntConstant( module, "K_i",            SDLK_i );
    PyModule_AddIntConstant( module, "K_j",            SDLK_j );
    PyModule_AddIntConstant( module, "K_k",            SDLK_k );
    PyModule_AddIntConstant( module, "K_l",            SDLK_l );
    PyModule_AddIntConstant( module, "K_m",            SDLK_m );
    PyModule_AddIntConstant( module, "K_n",            SDLK_n );
    PyModule_AddIntConstant( module, "K_o",            SDLK_o );
    PyModule_AddIntConstant( module, "K_p",            SDLK_p );
    PyModule_AddIntConstant( module, "K_q",            SDLK_q );
    PyModule_AddIntConstant( module, "K_r",            SDLK_r );
    PyModule_AddIntConstant( module, "K_s",            SDLK_s );
    PyModule_AddIntConstant( module, "K_t",            SDLK_t );
    PyModule_AddIntConstant( module, "K_u",            SDLK_u );
    PyModule_AddIntConstant( module, "K_v",            SDLK_v );
    PyModule_AddIntConstant( module, "K_w",            SDLK_w );
    PyModule_AddIntConstant( module, "K_x",            SDLK_x );
    PyModule_AddIntConstant( module, "K_y",            SDLK_y );
    PyModule_AddIntConstant( module, "K_z",            SDLK_z );
    PyModule_AddIntConstant( module, "K_DELETE",       SDLK_DELETE );

/*
    PyModule_AddIntConstant( module, "K_WORLD_0",      SDLK_WORLD_0 );
    PyModule_AddIntConstant( module, "K_WORLD_1",      SDLK_WORLD_1 );
    PyModule_AddIntConstant( module, "K_WORLD_2",      SDLK_WORLD_2 );
    PyModule_AddIntConstant( module, "K_WORLD_3",      SDLK_WORLD_3 );
    PyModule_AddIntConstant( module, "K_WORLD_4",      SDLK_WORLD_4 );
    PyModule_AddIntConstant( module, "K_WORLD_5",      SDLK_WORLD_5 );
    PyModule_AddIntConstant( module, "K_WORLD_6",      SDLK_WORLD_6 );
    PyModule_AddIntConstant( module, "K_WORLD_7",      SDLK_WORLD_7 );
    PyModule_AddIntConstant( module, "K_WORLD_8",      SDLK_WORLD_8 );
    PyModule_AddIntConstant( module, "K_WORLD_9",      SDLK_WORLD_9 );
    PyModule_AddIntConstant( module, "K_WORLD_10",     SDLK_WORLD_10 );
    PyModule_AddIntConstant( module, "K_WORLD_11",     SDLK_WORLD_11 );
    PyModule_AddIntConstant( module, "K_WORLD_12",     SDLK_WORLD_12 );
    PyModule_AddIntConstant( module, "K_WORLD_13",     SDLK_WORLD_13 );
    PyModule_AddIntConstant( module, "K_WORLD_14",     SDLK_WORLD_14 );
    PyModule_AddIntConstant( module, "K_WORLD_15",     SDLK_WORLD_15 );
    PyModule_AddIntConstant( module, "K_WORLD_16",     SDLK_WORLD_16 );
    PyModule_AddIntConstant( module, "K_WORLD_17",     SDLK_WORLD_17 );
    PyModule_AddIntConstant( module, "K_WORLD_18",     SDLK_WORLD_18 );
    PyModule_AddIntConstant( module, "K_WORLD_19",     SDLK_WORLD_19 );
    PyModule_AddIntConstant( module, "K_WORLD_20",     SDLK_WORLD_20 );
    PyModule_AddIntConstant( module, "K_WORLD_21",     SDLK_WORLD_21 );
    PyModule_AddIntConstant( module, "K_WORLD_22",     SDLK_WORLD_22 );
    PyModule_AddIntConstant( module, "K_WORLD_23",     SDLK_WORLD_23 );
    PyModule_AddIntConstant( module, "K_WORLD_24",     SDLK_WORLD_24 );
    PyModule_AddIntConstant( module, "K_WORLD_25",     SDLK_WORLD_25 );
    PyModule_AddIntConstant( module, "K_WORLD_26",     SDLK_WORLD_26 );
    PyModule_AddIntConstant( module, "K_WORLD_27",     SDLK_WORLD_27 );
    PyModule_AddIntConstant( module, "K_WORLD_28",     SDLK_WORLD_28 );
    PyModule_AddIntConstant( module, "K_WORLD_29",     SDLK_WORLD_29 );
    PyModule_AddIntConstant( module, "K_WORLD_30",     SDLK_WORLD_30 );
    PyModule_AddIntConstant( module, "K_WORLD_31",     SDLK_WORLD_31 );
    PyModule_AddIntConstant( module, "K_WORLD_32",     SDLK_WORLD_32 );
    PyModule_AddIntConstant( module, "K_WORLD_33",     SDLK_WORLD_33 );
    PyModule_AddIntConstant( module, "K_WORLD_34",     SDLK_WORLD_34 );
    PyModule_AddIntConstant( module, "K_WORLD_35",     SDLK_WORLD_35 );
    PyModule_AddIntConstant( module, "K_WORLD_36",     SDLK_WORLD_36 );
    PyModule_AddIntConstant( module, "K_WORLD_37",     SDLK_WORLD_37 );
    PyModule_AddIntConstant( module, "K_WORLD_38",     SDLK_WORLD_38 );
    PyModule_AddIntConstant( module, "K_WORLD_39",     SDLK_WORLD_39 );
    PyModule_AddIntConstant( module, "K_WORLD_40",     SDLK_WORLD_40 );
    PyModule_AddIntConstant( module, "K_WORLD_41",     SDLK_WORLD_41 );
    PyModule_AddIntConstant( module, "K_WORLD_42",     SDLK_WORLD_42 );
    PyModule_AddIntConstant( module, "K_WORLD_43",     SDLK_WORLD_43 );
    PyModule_AddIntConstant( module, "K_WORLD_44",     SDLK_WORLD_44 );
    PyModule_AddIntConstant( module, "K_WORLD_45",     SDLK_WORLD_45 );
    PyModule_AddIntConstant( module, "K_WORLD_46",     SDLK_WORLD_46 );
    PyModule_AddIntConstant( module, "K_WORLD_47",     SDLK_WORLD_47 );
    PyModule_AddIntConstant( module, "K_WORLD_48",     SDLK_WORLD_48 );
    PyModule_AddIntConstant( module, "K_WORLD_49",     SDLK_WORLD_49 );
    PyModule_AddIntConstant( module, "K_WORLD_50",     SDLK_WORLD_50 );
    PyModule_AddIntConstant( module, "K_WORLD_51",     SDLK_WORLD_51 );
    PyModule_AddIntConstant( module, "K_WORLD_52",     SDLK_WORLD_52 );
    PyModule_AddIntConstant( module, "K_WORLD_53",     SDLK_WORLD_53 );
    PyModule_AddIntConstant( module, "K_WORLD_54",     SDLK_WORLD_54 );
    PyModule_AddIntConstant( module, "K_WORLD_55",     SDLK_WORLD_55 );
    PyModule_AddIntConstant( module, "K_WORLD_56",     SDLK_WORLD_56 );
    PyModule_AddIntConstant( module, "K_WORLD_57",     SDLK_WORLD_57 );
    PyModule_AddIntConstant( module, "K_WORLD_58",     SDLK_WORLD_58 );
    PyModule_AddIntConstant( module, "K_WORLD_59",     SDLK_WORLD_59 );
    PyModule_AddIntConstant( module, "K_WORLD_60",     SDLK_WORLD_60 );
    PyModule_AddIntConstant( module, "K_WORLD_61",     SDLK_WORLD_61 );
    PyModule_AddIntConstant( module, "K_WORLD_62",     SDLK_WORLD_62 );
    PyModule_AddIntConstant( module, "K_WORLD_63",     SDLK_WORLD_63 );
    PyModule_AddIntConstant( module, "K_WORLD_64",     SDLK_WORLD_64 );
    PyModule_AddIntConstant( module, "K_WORLD_65",     SDLK_WORLD_65 );
    PyModule_AddIntConstant( module, "K_WORLD_66",     SDLK_WORLD_66 );
    PyModule_AddIntConstant( module, "K_WORLD_67",     SDLK_WORLD_67 );
    PyModule_AddIntConstant( module, "K_WORLD_68",     SDLK_WORLD_68 );
    PyModule_AddIntConstant( module, "K_WORLD_69",     SDLK_WORLD_69 );
    PyModule_AddIntConstant( module, "K_WORLD_70",     SDLK_WORLD_70 );
    PyModule_AddIntConstant( module, "K_WORLD_71",     SDLK_WORLD_71 );
    PyModule_AddIntConstant( module, "K_WORLD_72",     SDLK_WORLD_72 );
    PyModule_AddIntConstant( module, "K_WORLD_73",     SDLK_WORLD_73 );
    PyModule_AddIntConstant( module, "K_WORLD_74",     SDLK_WORLD_74 );
    PyModule_AddIntConstant( module, "K_WORLD_75",     SDLK_WORLD_75 );
    PyModule_AddIntConstant( module, "K_WORLD_76",     SDLK_WORLD_76 );
    PyModule_AddIntConstant( module, "K_WORLD_77",     SDLK_WORLD_77 );
    PyModule_AddIntConstant( module, "K_WORLD_78",     SDLK_WORLD_78 );
    PyModule_AddIntConstant( module, "K_WORLD_79",     SDLK_WORLD_79 );
    PyModule_AddIntConstant( module, "K_WORLD_80",     SDLK_WORLD_80 );
    PyModule_AddIntConstant( module, "K_WORLD_81",     SDLK_WORLD_81 );
    PyModule_AddIntConstant( module, "K_WORLD_82",     SDLK_WORLD_82 );
    PyModule_AddIntConstant( module, "K_WORLD_83",     SDLK_WORLD_83 );
    PyModule_AddIntConstant( module, "K_WORLD_84",     SDLK_WORLD_84 );
    PyModule_AddIntConstant( module, "K_WORLD_85",     SDLK_WORLD_85 );
    PyModule_AddIntConstant( module, "K_WORLD_86",     SDLK_WORLD_86 );
    PyModule_AddIntConstant( module, "K_WORLD_87",     SDLK_WORLD_87 );
    PyModule_AddIntConstant( module, "K_WORLD_88",     SDLK_WORLD_88 );
    PyModule_AddIntConstant( module, "K_WORLD_89",     SDLK_WORLD_89 );
    PyModule_AddIntConstant( module, "K_WORLD_90",     SDLK_WORLD_90 );
    PyModule_AddIntConstant( module, "K_WORLD_91",     SDLK_WORLD_91 );
    PyModule_AddIntConstant( module, "K_WORLD_92",     SDLK_WORLD_92 );
    PyModule_AddIntConstant( module, "K_WORLD_93",     SDLK_WORLD_93 );
    PyModule_AddIntConstant( module, "K_WORLD_94",     SDLK_WORLD_94 );
    PyModule_AddIntConstant( module, "K_WORLD_95",     SDLK_WORLD_95 );

    PyModule_AddIntConstant( module, "K_KP0",          SDLK_KP0 );
    PyModule_AddIntConstant( module, "K_KP1",          SDLK_KP1 );
    PyModule_AddIntConstant( module, "K_KP2",          SDLK_KP2 );
    PyModule_AddIntConstant( module, "K_KP3",          SDLK_KP3 );
    PyModule_AddIntConstant( module, "K_KP4",          SDLK_KP4 );
    PyModule_AddIntConstant( module, "K_KP5",          SDLK_KP5 );
    PyModule_AddIntConstant( module, "K_KP6",          SDLK_KP6 );
    PyModule_AddIntConstant( module, "K_KP7",          SDLK_KP7 );
    PyModule_AddIntConstant( module, "K_KP8",          SDLK_KP8 );
    PyModule_AddIntConstant( module, "K_KP9",          SDLK_KP9 );
    PyModule_AddIntConstant( module, "K_KP_PERIOD",    SDLK_KP_PERIOD );
    PyModule_AddIntConstant( module, "K_KP_DIVIDE",    SDLK_KP_DIVIDE );
    PyModule_AddIntConstant( module, "K_KP_MULTIPLY",  SDLK_KP_MULTIPLY );
    PyModule_AddIntConstant( module, "K_KP_MINUS",     SDLK_KP_MINUS );
    PyModule_AddIntConstant( module, "K_KP_PLUS",      SDLK_KP_PLUS );
    PyModule_AddIntConstant( module, "K_KP_ENTER",     SDLK_KP_ENTER );
    PyModule_AddIntConstant( module, "K_KP_EQUALS",    SDLK_KP_EQUALS );
    */
    PyModule_AddIntConstant( module, "K_UP",           SDLK_UP );
    PyModule_AddIntConstant( module, "K_DOWN",         SDLK_DOWN );
    PyModule_AddIntConstant( module, "K_RIGHT",        SDLK_RIGHT );
    PyModule_AddIntConstant( module, "K_LEFT",         SDLK_LEFT );
    /*
    PyModule_AddIntConstant( module, "K_INSERT",       SDLK_INSERT );
    PyModule_AddIntConstant( module, "K_HOME",         SDLK_HOME );
    PyModule_AddIntConstant( module, "K_END",          SDLK_END );
    PyModule_AddIntConstant( module, "K_PAGEUP",       SDLK_PAGEUP );
    PyModule_AddIntConstant( module, "K_PAGEDOWN",     SDLK_PAGEDOWN );
    PyModule_AddIntConstant( module, "K_F1",           SDLK_F1 );
    PyModule_AddIntConstant( module, "K_F2",           SDLK_F2 );
    PyModule_AddIntConstant( module, "K_F3",           SDLK_F3 );
    PyModule_AddIntConstant( module, "K_F4",           SDLK_F4 );
    PyModule_AddIntConstant( module, "K_F5",           SDLK_F5 );
    PyModule_AddIntConstant( module, "K_F6",           SDLK_F6 );
    PyModule_AddIntConstant( module, "K_F7",           SDLK_F7 );
    PyModule_AddIntConstant( module, "K_F8",           SDLK_F8 );
    PyModule_AddIntConstant( module, "K_F9",           SDLK_F9 );
    PyModule_AddIntConstant( module, "K_F10",          SDLK_F10 );
    PyModule_AddIntConstant( module, "K_F11",          SDLK_F11 );
    PyModule_AddIntConstant( module, "K_F12",          SDLK_F12 );
    PyModule_AddIntConstant( module, "K_F13",          SDLK_F13 );
    PyModule_AddIntConstant( module, "K_F14",          SDLK_F14 );
    PyModule_AddIntConstant( module, "K_F15",          SDLK_F15 );
    PyModule_AddIntConstant( module, "K_NUMLOCK",      SDLK_NUMLOCK );
    PyModule_AddIntConstant( module, "K_CAPSLOCK",     SDLK_CAPSLOCK );
    PyModule_AddIntConstant( module, "K_SCROLLOCK",    SDLK_SCROLLOCK );
    PyModule_AddIntConstant( module, "K_RSHIFT",       SDLK_RSHIFT );
    PyModule_AddIntConstant( module, "K_LSHIFT",       SDLK_LSHIFT );
    PyModule_AddIntConstant( module, "K_RCTRL",        SDLK_RCTRL );
    PyModule_AddIntConstant( module, "K_LCTRL",        SDLK_LCTRL );
    PyModule_AddIntConstant( module, "K_RALT",         SDLK_RALT );
    PyModule_AddIntConstant( module, "K_LALT",         SDLK_LALT );
    PyModule_AddIntConstant( module, "K_RMETA",        SDLK_RMETA );
    PyModule_AddIntConstant( module, "K_LMETA",        SDLK_LMETA );
    PyModule_AddIntConstant( module, "K_LSUPER",       SDLK_LSUPER );
    PyModule_AddIntConstant( module, "K_RSUPER",       SDLK_RSUPER );
    PyModule_AddIntConstant( module, "K_MODE",         SDLK_MODE );
    PyModule_AddIntConstant( module, "K_COMPOSE",      SDLK_COMPOSE );
    PyModule_AddIntConstant( module, "K_HELP",         SDLK_HELP );
    PyModule_AddIntConstant( module, "K_PRINT",        SDLK_PRINT );
    PyModule_AddIntConstant( module, "K_SYSREQ",       SDLK_SYSREQ );
    PyModule_AddIntConstant( module, "K_BREAK",        SDLK_BREAK );
    PyModule_AddIntConstant( module, "K_MENU",         SDLK_MENU );
    PyModule_AddIntConstant( module, "K_POWER",        SDLK_POWER );
    PyModule_AddIntConstant( module, "K_EURO",         SDLK_EURO );
    PyModule_AddIntConstant( module, "K_UNDO",         SDLK_UNDO );
*/
    // MODIFIER KEYS
    PyModule_AddIntConstant( module, "KMOD_NONE",    KMOD_NONE    );
    PyModule_AddIntConstant( module, "KMOD_LSHIFT",  KMOD_LSHIFT  );
    PyModule_AddIntConstant( module, "KMOD_RSHIFT",  KMOD_RSHIFT  );
    PyModule_AddIntConstant( module, "KMOD_LCTRL",   KMOD_LCTRL   );
    PyModule_AddIntConstant( module, "KMOD_RCTRL",   KMOD_RCTRL   );
    PyModule_AddIntConstant( module, "KMOD_LALT",    KMOD_LALT    );
    PyModule_AddIntConstant( module, "KMOD_RALT",    KMOD_RALT    );
    //PyModule_AddIntConstant( module, "KMOD_LMETA",   KMOD_LMETA   );
//    PyModule_AddIntConstant( module, "KMOD_RMETA",   KMOD_RMETA   );
    PyModule_AddIntConstant( module, "KMOD_NUM",     KMOD_NUM     );
    PyModule_AddIntConstant( module, "KMOD_CAPS",    KMOD_CAPS    );
    PyModule_AddIntConstant( module, "KMOD_MODE",    KMOD_MODE    );
    PyModule_AddIntConstant( module, "KMOD_CTRL",    KMOD_CTRL    );
    PyModule_AddIntConstant( module, "KMOD_SHIFT",   KMOD_SHIFT   );
    PyModule_AddIntConstant( module, "KMOD_ALT",     KMOD_ALT     );
//    PyModule_AddIntConstant( module, "KMOD_META",    KMOD_META    );

    return module;
}


static PyObject * PySDL_GetCurrentVideoDriver(PyObject *self, PyObject *args) {
    return PyUnicode_FromString(SDL_GetCurrentVideoDriver());
}


static PyObject * PySDL_GetVideoDrivers(PyObject *self, PyObject *args) {
    PyObject *list;
    int count;
    int idx;

    count = SDL_GetNumVideoDrivers();
    list = PyList_New(count);
    for(idx = 0; idx < count; ++idx) {
        PyList_SetItem(list, idx, PyUnicode_FromString(SDL_GetVideoDriver(idx)));
    }

    return list;
}


static PyObject * PySDL_Init(PyObject *self, PyObject *args) {
    int flags = SDL_INIT_EVERYTHING;
    int ok;

    ok = PyArg_ParseTuple(args, "|i", &flags);
    if(!ok) {
        PyErr_SetString(PyExc_TypeError, "Integer expected");
        return NULL;
    }

    ok = SDL_Init(flags);
    if(0 > ok) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }

    return PyLong_FromLong(ok);
}


static PyObject * PySDL_LoadBMP(PyObject *self, PyObject *args) {
    PySDL_Surface *pysdl_Surface;

    pysdl_Surface = (PySDL_Surface *)PyObject_CallObject((PyObject *)&PySDL_Surface_Type, NULL);
    if(NULL == pysdl_Surface) {
        PyErr_SetString(PyExc_TypeError, "Could not create pysdl.Renderer object");
        return NULL;
    }

    pysdl_Surface->surface = SDL_LoadBMP(PyUnicode_AsUTF8(args));
    if(NULL == pysdl_Surface->surface) {
        PyErr_SetString(pysdl_Error, SDL_GetError());
        return NULL;
    }

    return (PyObject *)pysdl_Surface;
}


static PyObject * PySDL_Quit(PyObject *self, PyObject *args) {
    SDL_Quit();
    Py_RETURN_NONE;
}


static PyObject * PySDL_ShowCursor(PyObject *self, PyObject *args) {
    SDL_ShowCursor(PyLong_AsLong(args));
    Py_RETURN_NONE;
}


static PyObject * PySDL_Version(PyObject *self, PyObject *args) {
    PyObject *version;
    SDL_version compiled;

    SDL_VERSION(&compiled);

    version = PyTuple_New(3);
    PyTuple_SetItem(version, 0, PyLong_FromLong(compiled.major));
    PyTuple_SetItem(version, 1, PyLong_FromLong(compiled.minor));
    PyTuple_SetItem(version, 2, PyLong_FromLong(compiled.patch));

    return version;
}


static PyObject * PySDL_GetPlatform(PyObject *self, PyObject *args) {
    return PyUnicode_FromString(SDL_GetPlatform());
}


static PyObject * PySDL_WasInit(PyObject *self, PyObject *args) {
    int flags = SDL_INIT_EVERYTHING;
    uint32_t subsystems;
    int ok;

    ok = PyArg_ParseTuple(args, "|i", &flags);
    if(0 > ok) {
        return NULL;
    }

    subsystems = SDL_WasInit(flags);
    return PyLong_FromLong(subsystems);
}

static PyObject * PySDL_GetTicks(PyObject *self, PyObject *args) {
    return PyLong_FromUnsignedLong(SDL_GetTicks());
}


PyObject * _event(SDL_Event *event) {
    PyObject *result;
    PyObject *data;

    result = PyTuple_New(2);
    PyTuple_SetItem(result, 0, PyLong_FromLong(event->type));

    switch(event->type) {
    //case SDL_ACTIVEEVENT: {
    //    Py_INCREF(Py_None);
    //    data = Py_None;
    //    } break;
    case SDL_KEYDOWN:
    case SDL_KEYUP: {
        data = PyTuple_New(4);
        SDL_KeyboardEvent *keyboard = (SDL_KeyboardEvent *)event;
        PyTuple_SetItem(data, 0, PyLong_FromLong(keyboard->state));
        PyTuple_SetItem(data, 1, PyLong_FromLong(keyboard->keysym.scancode));
        PyTuple_SetItem(data, 2, PyLong_FromLong(keyboard->keysym.sym));
        PyTuple_SetItem(data, 3, PyLong_FromLong(keyboard->keysym.mod));
        //PyTuple_SetItem(data, 5, PyLong_FromLong(keyboard->keysym.unicode));
        } break;
    case SDL_MOUSEMOTION: {
        data = PyTuple_New(5);
        SDL_MouseMotionEvent *mousemotion = (SDL_MouseMotionEvent *)event;
        PyTuple_SetItem(data, 0, PyLong_FromLong(mousemotion->state));
        PyTuple_SetItem(data, 1, PyLong_FromLong(mousemotion->x));
        PyTuple_SetItem(data, 2, PyLong_FromLong(mousemotion->y));
        PyTuple_SetItem(data, 3, PyLong_FromLong(mousemotion->xrel));
        PyTuple_SetItem(data, 4, PyLong_FromLong(mousemotion->yrel));
        } break;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP: {
        data = PyTuple_New(5);
        SDL_MouseButtonEvent *mousebutton = (SDL_MouseButtonEvent *)event;
        PyTuple_SetItem(data, 0, PyLong_FromLong(mousebutton->which));
        PyTuple_SetItem(data, 1, PyLong_FromLong(mousebutton->button));
        PyTuple_SetItem(data, 2, PyLong_FromLong(mousebutton->state));
        PyTuple_SetItem(data, 3, PyLong_FromLong(mousebutton->x));
        PyTuple_SetItem(data, 4, PyLong_FromLong(mousebutton->y));
        } break;
    case SDL_JOYAXISMOTION: {
        SDL_JoyAxisEvent *joyaxis = (SDL_JoyAxisEvent *)event;
        data = PyTuple_New(3);
        PyTuple_SetItem(data, 0, PyLong_FromLong(joyaxis->which));
        PyTuple_SetItem(data, 1, PyLong_FromLong(joyaxis->axis));
        PyTuple_SetItem(data, 2, PyLong_FromLong(joyaxis->value));
        } break;
    //case SDL_JOYBALLMOTION:
    //    data = PyTuple_New(1);
    //    break;
    //case SDL_JOYHATMOTION:
    //    data = PyTuple_New(1);
    //    break;
    case SDL_JOYBUTTONDOWN:
    case SDL_JOYBUTTONUP: {
        data = PyTuple_New(3);
        SDL_JoyButtonEvent *joybutton = (SDL_JoyButtonEvent *)event;
        PyTuple_SetItem(data, 0, PyLong_FromLong(joybutton->which));
        PyTuple_SetItem(data, 1, PyLong_FromLong(joybutton->button));
        PyTuple_SetItem(data, 2, PyLong_FromLong(joybutton->state));
        } break;
    //case SDL_VIDEORESIZE: {
    //    data = PyTuple_New(2);
    //    SDL_ResizeEvent *resize = (SDL_ResizeEvent *)event;
    //    PyTuple_SetItem(data, 0, PyLong_FromLong(resize->w));
    //    PyTuple_SetItem(data, 1, PyLong_FromLong(resize->h));
    //    } break;
    //case SDL_VIDEOEXPOSE:
    //    data = PyTuple_New(1);
    //    break;
    //case SDL_QUIT:
    //    data = PyTuple_New(1);
    //    break;
    //case SDL_USEREVENT:
    //    data = PyTuple_New(1);
    //    break;
    //case SDL_SYSWMEVENT:
    //    data = PyTuple_New(1);
    //    break;
    default:
        Py_INCREF(Py_None);
        data = Py_None;
    }

    PyTuple_SetItem(result, 1, data);
    return result;
}

static PyObject * PySDL_PollEvent(PyObject *self, PyObject *args) {
    SDL_Event event;
    int ok;

    ok = SDL_PollEvent(&event);
    if(0 == ok) {
        Py_RETURN_NONE;
    }
    return _event(&event);
}

static PyObject * PySDL_WaitEvent(PyObject *self, PyObject *args) {
    SDL_Event event;
    int ok;

    Py_BEGIN_ALLOW_THREADS
        ok = SDL_WaitEvent(&event);
    Py_END_ALLOW_THREADS
    if(0 == ok) {
        Py_RETURN_NONE;
    }
    return _event(&event);
}

//PyObject * PySDL_PushEvent(PyObject *self, PyObject *data) {
//    SDL_Event event;
//    event.type = (Uint8)PyInt_AsLong(PyTuple_GetItem(data, 0));
//    SDL_PushEvent(&event);
//    Py_RETURN_NONE;
//}

static PyObject * PySDL_GetKeyboardState(PyObject *self, PyObject *args) {
    PyObject *list;
    const uint8_t *keys;
    int len;
    int idx;

    keys = SDL_GetKeyboardState(&len);
    list = PyList_New(len);
    for(idx = 0; idx < len; ++idx) {
        PyList_SetItem(list, idx, PyBool_FromLong(keys[idx]));
    }
    return list;
}

static PyObject * PySDL_GetModState(PyObject *self, PyObject *args) {
    return PyLong_FromLong(SDL_GetModState());
}

static PyObject * PySDL_GetCPUCount(PyObject *self, PyObject *args) {
    return PyLong_FromLong(SDL_GetCPUCount());
}

static PyObject * PySDL_GetCPUCacheLineSize(PyObject *self, PyObject *args) {
    return PyLong_FromLong(SDL_GetCPUCacheLineSize());
}

static PyObject * PySDL_Has3DNow(PyObject *self, PyObject *args) {
    PyObject *hasFeature = SDL_Has3DNow() ? Py_True : Py_False;
    Py_INCREF(hasFeature);
    return hasFeature;
}

static PyObject * PySDL_HasAVX(PyObject *self, PyObject *args) {
    PyObject *hasFeature = SDL_HasAVX() ? Py_True : Py_False;
    Py_INCREF(hasFeature);
    return hasFeature;
}

static PyObject * PySDL_HasAVX2(PyObject *self, PyObject *args) {
    PyObject *hasFeature = SDL_HasAVX2() ? Py_True : Py_False;
    Py_INCREF(hasFeature);
    return hasFeature;
}

static PyObject * PySDL_HasAltiVec(PyObject *self, PyObject *args) {
    PyObject *hasFeature = SDL_HasAltiVec() ? Py_True : Py_False;
    Py_INCREF(hasFeature);
    return hasFeature;
}

static PyObject * PySDL_HasMMX(PyObject *self, PyObject *args) {
    PyObject *hasFeature = SDL_HasMMX() ? Py_True : Py_False;
    Py_INCREF(hasFeature);
    return hasFeature;
}

static PyObject * PySDL_HasSSE(PyObject *self, PyObject *args) {
    PyObject *hasFeature = SDL_HasSSE() ? Py_True : Py_False;
    Py_INCREF(hasFeature);
    return hasFeature;
}

static PyObject * PySDL_HasSSE2(PyObject *self, PyObject *args) {
    PyObject *hasFeature = SDL_HasSSE2() ? Py_True : Py_False;
    Py_INCREF(hasFeature);
    return hasFeature;
}

static PyObject * PySDL_HasSSE3(PyObject *self, PyObject *args) {
    PyObject *hasFeature = SDL_HasSSE3() ? Py_True : Py_False;
    Py_INCREF(hasFeature);
    return hasFeature;
}

static PyObject * PySDL_HasSSE41(PyObject *self, PyObject *args) {
    PyObject *hasFeature = SDL_HasSSE41() ? Py_True : Py_False;
    Py_INCREF(hasFeature);
    return hasFeature;
}

static PyObject * PySDL_HasSSE42(PyObject *self, PyObject *args) {
    PyObject *hasFeature = SDL_HasSSE42() ? Py_True : Py_False;
    Py_INCREF(hasFeature);
    return hasFeature;
}
