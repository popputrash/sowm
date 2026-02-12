#include <X11/Xlib.h>

#define FOR_EACH_CLIENT (client *_prev=0, *c=client_list; c && _prev!=client_list->prev; _prev=c, c=c->next)
#define ws_save(W) ws_list[W] = client_list
#define ws_sel(W)  client_list = ws_list[current_workspace = W]
#define MAX(a, b)  ((a) > (b) ? (a) : (b))

#define win_size(W, out_x, out_y, out_w, out_h) \
    XGetGeometry(display, W, &(Window){0}, out_x, out_y, out_w, out_h, \
                 &(unsigned int){0}, &(unsigned int){0})

// Taken from DWM. Many thanks. https://git.suckless.org/dwm
#define mod_clean(mask) (mask & ~(numlock|LockMask) & \
        (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))

typedef struct {
    const char** command;
    const int workspace;
    const Window window;
    const int side;
} Arg;

struct key {
    unsigned int mod;
    KeySym keysym;
    void (*function)(const Arg arg);
    const Arg arg;
};

typedef struct client {
    struct client *next, *prev;
    int fullscreen, saved_x, saved_y, tiled;
    unsigned int saved_width, saved_height;
    unsigned char fs;
    Window window;
    Bool istiled, isfloating;
} client;

int multimonitor_action(int action);

unsigned long get_color(const char *color_name);
void button_press(XEvent *e);
void button_release(XEvent *e);
void configure_request(XEvent *e);
void input_grab(Window root);
void key_press(XEvent *e);
void map_request(XEvent *e);
void mapping_notify(XEvent *e);
void notify_destroy(XEvent *e);
void notify_enter(XEvent *e);
void notify_unmap(XEvent *e);
void notify_motion(XEvent *e);
void spawn(const Arg arg);
void win_add(Window window);
void win_center(const Arg arg);
void win_del(Window window);
void win_fullscreen(const Arg arg);
void win_tile(const Arg arg);
void win_focus(client *target);
void win_kill(const Arg arg);
void win_prev(const Arg arg);
void win_next(const Arg arg);
void win_to_ws(const Arg arg);
void ws_go(const Arg arg);

static int xerror() { return 0; }
