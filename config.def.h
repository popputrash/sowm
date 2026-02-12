#ifndef CONFIG_H
#define CONFIG_H

#define MOD Mod4Mask

const char* menu[]    = {"dmenu_run",      0};
const char* term[]    = {"st",             0};
const char* scrot[]   = {"scr",            0};
const char* briup[]   = {"bri", "10", "+", 0};
const char* bridown[] = {"bri", "10", "-", 0};
const char* voldown[] = {"amixer", "sset", "Master", "5%-",         0};
const char* volup[]   = {"amixer", "sset", "Master", "5%+",         0};
const char* volmute[] = {"amixer", "sset", "Master", "toggle",      0};
const char* colors[]  = {"bud", "/home/goldie/Pictures/Wallpapers", 0};

static struct key keys[] = {
    {MOD,      XK_q,   win_kill,       {0}},
    {MOD,      XK_c,   win_center,     {0}},
    {MOD,      XK_f,   win_fullscreen, {0}},

    {Mod1Mask,           XK_Tab, win_next,   {0}},
    {Mod1Mask|ShiftMask, XK_Tab, win_prev,   {0}},

    {MOD, XK_d,      spawn, {.command = menu}},
    {MOD, XK_w,      spawn, {.command = colors}},
    {MOD, XK_p,      spawn, {.command = scrot}},
    {MOD, XK_Return, spawn, {.command = term}},

    {0,   XF86XK_AudioLowerVolume,  spawn, {.command = voldown}},
    {0,   XF86XK_AudioRaiseVolume,  spawn, {.command = volup}},
    {0,   XF86XK_AudioMute,         spawn, {.command = volmute}},
    {0,   XF86XK_MonBrightnessUp,   spawn, {.command = briup}},
    {0,   XF86XK_MonBrightnessDown, spawn, {.command = bridown}},

    {MOD,           XK_1, ws_go,     {.workspace = 1}},
    {MOD|ShiftMask, XK_1, win_to_ws, {.workspace = 1}},
    {MOD,           XK_2, ws_go,     {.workspace = 2}},
    {MOD|ShiftMask, XK_2, win_to_ws, {.workspace = 2}},
    {MOD,           XK_3, ws_go,     {.workspace = 3}},
    {MOD|ShiftMask, XK_3, win_to_ws, {.workspace = 3}},
    {MOD,           XK_4, ws_go,     {.workspace = 4}},
    {MOD|ShiftMask, XK_4, win_to_ws, {.workspace = 4}},
    {MOD,           XK_5, ws_go,     {.workspace = 5}},
    {MOD|ShiftMask, XK_5, win_to_ws, {.workspace = 5}},
    {MOD,           XK_6, ws_go,     {.workspace = 6}},
    {MOD|ShiftMask, XK_6, win_to_ws, {.workspace = 6}},
};

#endif
