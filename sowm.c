// sowm - An itsy bitsy floating window manager.

#include <X11/XF86keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>
#include <X11/keysym.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include "sowm.h"

static client *client_list = {0}, *ws_list[10] = {0}, *focused;
static int current_workspace = 1, screen_width, screen_height, drag_x, drag_y,
           numlock = 0, ws_switching = 0;
static unsigned int drag_width, drag_height;

static int screen;
static Display *display;
static XButtonEvent mouse;
static Window root;

static void (*event_handlers[LASTEvent])(XEvent *e) = {
    [ButtonPress] = button_press,
    [ButtonRelease] = button_release,
    [ConfigureRequest] = configure_request,
    [KeyPress] = key_press,
    [MapRequest] = map_request,
    [MappingNotify] = mapping_notify,
    [DestroyNotify] = notify_destroy,
    [EnterNotify] = notify_enter,
    [MotionNotify] = notify_motion,
    [UnmapNotify] = notify_unmap};

#include "config.h"

unsigned long get_color(const char *color_name) {
  Colormap colormap = DefaultColormap(display, screen);
  XColor color;
  return (!XAllocNamedColor(display, colormap, color_name, &color, &color))
             ? 0
             : color.pixel;
}

void win_focus(client *target) {
  if (focused)
    XSetWindowBorder(display, focused->window, get_color(BORDER_NORMAL));
  focused = target;

  if (focused->fullscreen) {
    XConfigureWindow(display, focused->window, CWBorderWidth,
                     &(XWindowChanges){.border_width = 0});
  } else {
    XConfigureWindow(display, focused->window, CWBorderWidth,
                     &(XWindowChanges){.border_width = BORDER_WIDTH});
  }

  XSetWindowBorder(display, focused->window, get_color(BORDER_SELECT));

  XSetInputFocus(display, focused->window, RevertToParent, CurrentTime);
}

void notify_destroy(XEvent *e) {
  win_del(e->xdestroywindow.window);

  if (client_list) {
    win_focus(client_list->prev);
  } else {
    focused = 0;
  }
}

void notify_unmap(XEvent *e) {
  if (ws_switching)
    return;

  win_del(e->xunmap.window);

  if (client_list) {
    win_focus(client_list->prev);
  } else {
    focused = 0;
  }
}

void notify_enter(XEvent *e) {
  while (XCheckTypedEvent(display, EnterNotify, e))
    ;
  while (XCheckTypedWindowEvent(display, mouse.subwindow, MotionNotify, e))
    ;

    for
      FOR_EACH_CLIENT if (c->window == e->xcrossing.window) win_focus(c);
}

void notify_motion(XEvent *e) {
  if (!mouse.subwindow || focused->fullscreen || focused->istiled)
    return;
  while (XCheckTypedEvent(display, MotionNotify, e))
    ;

  int delta_x = e->xbutton.x_root - mouse.x_root;
  int delta_y = e->xbutton.y_root - mouse.y_root;

  XMoveResizeWindow(display, mouse.subwindow,
                    drag_x + (mouse.button == 1 ? delta_x : 0),
                    drag_y + (mouse.button == 1 ? delta_y : 0),
                    MAX(1, drag_width + (mouse.button == 3 ? delta_x : 0)),
                    MAX(1, drag_height + (mouse.button == 3 ? delta_y : 0)));

  win_size(focused->window, &focused->saved_x, &focused->saved_y,
           &focused->saved_width, &focused->saved_height);
}

void key_press(XEvent *e) {
  KeySym keysym = XkbKeycodeToKeysym(display, e->xkey.keycode, 0, 0);

  for (unsigned int i = 0; i < sizeof(keys) / sizeof(*keys); ++i)
    if (keys[i].keysym == keysym &&
        mod_clean(keys[i].mod) == mod_clean(e->xkey.state))
      keys[i].function(keys[i].arg);
}

void button_press(XEvent *e) {
  XAllowEvents(display, ReplayPointer, e->xbutton.time);
  XSync(display, 0);
  if (!e->xbutton.subwindow)
    return;

  win_size(e->xbutton.subwindow, &drag_x, &drag_y, &drag_width, &drag_height);
  XRaiseWindow(display, e->xbutton.subwindow);
  mouse = e->xbutton;
}

void button_release(XEvent *e) { mouse.subwindow = 0; }

void win_add(Window window) {
  client *new_client;

  if (!(new_client = (client *)calloc(1, sizeof(client))))
    exit(1);

  new_client->window = window;

  if (client_list) {
    client_list->prev->next = new_client;
    new_client->prev = client_list->prev;
    client_list->prev = new_client;
    new_client->next = client_list;

  } else {
    client_list = new_client;
    client_list->prev = client_list->next = client_list;
  }

  ws_save(current_workspace);
  win_focus(new_client);
}

void win_del(Window window) {
  client *target = 0;

    for
      FOR_EACH_CLIENT if (c->window == window) target = c;

    if (!client_list || !target)
      return;
    if (target->prev == target)
      client_list = 0;
    if (client_list == target)
      client_list = target->next;
    if (target->next)
      target->next->prev = target->prev;
    if (target->prev)
      target->prev->next = target->next;

    free(target);
    ws_save(current_workspace);
}

void win_kill(const Arg arg) {
  if (focused)
    XKillClient(display, focused->window);
}

void win_center(const Arg arg) {
  if (!focused)
    return;

  XMoveWindow(display, focused->window, (screen_width - drag_width) / 2,
              (screen_height - drag_height) / 2);

  win_size(focused->window, &focused->saved_x, &focused->saved_y,
           &focused->saved_width, &focused->saved_height);
}

void win_fullscreen(const Arg arg) {
  if (!focused)
    return;

  if ((focused->fullscreen = focused->fullscreen ? 0 : 1)) {
    win_size(focused->window, &focused->saved_x, &focused->saved_y,
             &focused->saved_width, &focused->saved_height);
    XMoveResizeWindow(display, focused->window, 0, 0, screen_width,
                      screen_height);
  } else {
    XMoveResizeWindow(display, focused->window, focused->saved_x,
                      focused->saved_y, focused->saved_width,
                      focused->saved_height);
    focused->isfloating = true;
  }
  win_focus(focused);
}

void win_tile(const Arg arg) {
  int n = 0, i = 0;
  int master_width;
  int g = TILE_GAP;
  int bw = BORDER_WIDTH;

    for
      FOR_EACH_CLIENT { n++; }
    if (!n)
      return;

    if (n == 1) {
        for
          FOR_EACH_CLIENT {
            XMoveResizeWindow(display, c->window, g, g,
                              screen_width - 2 * (g + bw),
                              screen_height - 2 * (g + bw));
          }
        return;
    }

    master_width = screen_width / 2;
    for
      FOR_EACH_CLIENT {
        int nx, ny, nw, nh;
        if (i == 0) {
          nx = g;
          ny = g;
          nw = master_width - g - g / 2 - 2 * bw;
          nh = screen_height - 2 * (g + bw);
        } else {
          int total = screen_height - 2 * g - (n - 2) * g - (n - 1) * 2 * bw;
          int stack_h = total / (n - 1);
          nx = master_width + g / 2;
          ny = g + (i - 1) * (stack_h + g + 2 * bw);
          nw = screen_width - master_width - g - g / 2 - 2 * bw;
          nh = stack_h;
        }
        XMoveResizeWindow(display, c->window, nx, ny, nw, nh);
        i++;
      }
}

void win_to_ws(const Arg arg) {
  int tmp = current_workspace;

  if (arg.workspace == tmp)
    return;

  ws_switching = 1;
  ws_sel(arg.workspace);
  win_add(focused->window);
  ws_save(arg.workspace);

  ws_sel(tmp);
  win_del(focused->window);
  XUnmapWindow(display, focused->window);
  ws_save(tmp);
  ws_switching = 0;
  if (client_list)
    win_focus(client_list);
}

void win_prev(const Arg arg) {
  if (!focused)
    return;

  XRaiseWindow(display, focused->prev->window);

  win_focus(focused->prev);
}

void win_next(const Arg arg) {
  if (!focused)
    return;

  XRaiseWindow(display, focused->next->window);
  win_focus(focused->next);
}

void ws_go(const Arg arg) {
  int tmp = current_workspace;

  if (arg.workspace == current_workspace)
    return;
  ws_switching = 1;
  ws_save(current_workspace);
  ws_sel(arg.workspace);

    for
      FOR_EACH_CLIENT { XMapWindow(display, c->window); }

    ws_sel(tmp);

    for
      FOR_EACH_CLIENT { XUnmapWindow(display, c->window); }
    ws_sel(arg.workspace);
    ws_switching = 0;
    if (client_list)
      win_focus(client_list);
    else
      focused = 0;
}

void configure_request(XEvent *e) {
  XConfigureRequestEvent *ev = &e->xconfigurerequest;

  XConfigureWindow(display, ev->window, ev->value_mask,
                   &(XWindowChanges){.x = ev->x,
                                     .y = ev->y,
                                     .width = ev->width,
                                     .height = ev->height,
                                     .sibling = ev->above,
                                     .stack_mode = ev->detail});
}

void map_request(XEvent *e) {
  Window window = e->xmaprequest.window;
        for
          FOR_EACH_CLIENT
        if (c->window == window) {
          XMapWindow(display, window);
          win_focus(c);
          return;
        }
        XSelectInput(display, window, StructureNotifyMask | EnterWindowMask);
        win_size(window, &drag_x, &drag_y, &drag_width, &drag_height);
        win_add(window);
        focused = client_list->prev;

        if (drag_x + drag_y == 0)
          win_center((Arg){0});

        XMapWindow(display, window);
        win_focus(client_list->prev);
}

void mapping_notify(XEvent *e) {
  XMappingEvent *ev = &e->xmapping;

  if (ev->request == MappingKeyboard || ev->request == MappingModifier) {
    XRefreshKeyboardMapping(ev);
    input_grab(root);
  }
}

void spawn(const Arg arg) {
  if (fork())
    return;
  if (display)
    close(ConnectionNumber(display));

  setsid();
  execvp((char *)arg.command[0], (char **)arg.command);
}

void input_grab(Window root) {
  unsigned int i, j, modifiers[] = {0, LockMask, numlock, numlock | LockMask};
  XModifierKeymap *modmap = XGetModifierMapping(display);
  KeyCode code;

  XGrabButton(display, 1, AnyModifier, root, True,
              ButtonPressMask | ButtonReleaseMask, GrabModeSync, GrabModeAsync,
              0, 0);

  for (i = 0; i < 8; i++)
    for (int k = 0; k < modmap->max_keypermod; k++)
      if (modmap->modifiermap[i * modmap->max_keypermod + k] ==
          XKeysymToKeycode(display, 0xff7f))
        numlock = (1 << i);

  XUngrabKey(display, AnyKey, AnyModifier, root);

  for (i = 0; i < sizeof(keys) / sizeof(*keys); i++)
    if ((code = XKeysymToKeycode(display, keys[i].keysym)))
      for (j = 0; j < sizeof(modifiers) / sizeof(*modifiers); j++)
        XGrabKey(display, code, keys[i].mod | modifiers[j], root, True,
                 GrabModeAsync, GrabModeAsync);

  for (i = 1; i < 4; i += 2)
    for (j = 0; j < sizeof(modifiers) / sizeof(*modifiers); j++)
      XGrabButton(display, i, MOD | modifiers[j], root, True,
                  ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                  GrabModeAsync, GrabModeAsync, 0, 0);

  XFreeModifiermap(modmap);
}

int main(void) {
  XEvent ev;

  if (!(display = XOpenDisplay(0)))
    exit(1);

  signal(SIGCHLD, SIG_IGN);
  XSetErrorHandler(xerror);

  screen = DefaultScreen(display);
  root = RootWindow(display, screen);
  screen_width = XDisplayWidth(display, screen);   //- (2*BORDER_WIDTH);
  screen_height = XDisplayHeight(display, screen); //- (2*BORDER_WIDTH);

  XSelectInput(display, root, SubstructureRedirectMask);
  XDefineCursor(display, root, XCreateFontCursor(display, 68));
  input_grab(root);

  while (1 && !XNextEvent(display, &ev)) // 1 && will forever be here.
    if (event_handlers[ev.type])
      event_handlers[ev.type](&ev);
}
