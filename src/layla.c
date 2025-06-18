#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#include "config.h"

/* Window manager state */
typedef struct {
    Display *display;
    Window root;
    int screen;
    Window focused;
    int running;
} WM;

static WM wm = {0};

/* Function prototypes */
static int wm_init(void);
static void wm_cleanup(void);
static void wm_run(void);
static void handle_key_press(XKeyEvent *e);
static void handle_map_request(XMapRequestEvent *e);
static void handle_configure_request(XConfigureRequestEvent *e);
static void handle_unmap_notify(XUnmapEvent *e);
static void handle_destroy_notify(XDestroyWindowEvent *e);
static void handle_enter_notify(XCrossingEvent *e);
static void handle_button_press(XButtonEvent *e);
static void focus_window(Window w);
static void grab_keys(void);
static void grab_buttons(void);
static int xerror_handler(Display *display, XErrorEvent *e);
static void signal_handler(int sig);

/* Key binding function implementations */
void spawn(const char *cmd);
void quit_wm(const char *arg);
void close_window(const char *arg);
void toggle_fullscreen(const char *arg);
void next_window(const char *arg);
void prev_window(const char *arg);

int main(int argc, char *argv[]) {
    /* Handle command line arguments */
    if (argc > 1) {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            printf("Layla Window Manager\n");
            printf("Usage: %s [options]\n", argv[0]);
            printf("Options:\n");
            printf("  -h, --help     Show this help message\n");
            printf("  -v, --version  Show version information\n");
            return 0;
        }
        if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
            printf("Layla Window Manager v0.1.0\n");
            return 0;
        }
    }
    
    /* Set up signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGCHLD, SIG_IGN); /* Ignore child processes */
    
    if (wm_init() != 0) {
        fprintf(stderr, "Failed to initialize window manager\n");
        return 1;
    }
    
    DEBUG_PRINT("Layla window manager started");
    printf("Layla window manager started\n");
    wm_run();
    
    wm_cleanup();
    printf("Layla window manager stopped\n");
    return 0;
}

static int wm_init(void) {
    /* Connect to X server */
    wm.display = XOpenDisplay(NULL);
    if (!wm.display) {
        fprintf(stderr, "Cannot open display\n");
        return -1;
    }
    
    wm.screen = DefaultScreen(wm.display);
    wm.root = RootWindow(wm.display, wm.screen);
    wm.running = 1;
    
    /* Set error handler */
    XSetErrorHandler(xerror_handler);
    
    /* Select events on root window */
    XSelectInput(wm.display, wm.root,
                 SubstructureRedirectMask | SubstructureNotifyMask |
                 PointerMotionMask | EnterWindowMask | LeaveWindowMask);
    
    /* Grab keys and buttons */
    grab_keys();
    grab_buttons();
    
    XSync(wm.display, False);
    
    /* Check if another window manager is running */
    XSetErrorHandler(xerror_handler);
    XSelectInput(wm.display, wm.root, SubstructureRedirectMask);
    XSync(wm.display, False);
    
    return 0;
}

static void wm_cleanup(void) {
    if (wm.display) {
        XCloseDisplay(wm.display);
    }
}

static void wm_run(void) {
    XEvent e;
    
    while (wm.running) {
        XNextEvent(wm.display, &e);
        
        switch (e.type) {
            case KeyPress:
                handle_key_press(&e.xkey);
                break;
            case MapRequest:
                handle_map_request(&e.xmaprequest);
                break;
            case ConfigureRequest:
                handle_configure_request(&e.xconfigurerequest);
                break;
            case UnmapNotify:
                handle_unmap_notify(&e.xunmap);
                break;
            case DestroyNotify:
                handle_destroy_notify(&e.xdestroywindow);
                break;
            case EnterNotify:
                handle_enter_notify(&e.xcrossing);
                break;
            case ButtonPress:
                handle_button_press(&e.xbutton);
                break;
        }
    }
}

static void handle_key_press(XKeyEvent *e) {
    KeySym keysym = XLookupKeysym(e, 0);
    unsigned int cleanmask = e->state & ~(LockMask | Mod2Mask);
    
    DEBUG_PRINT("Key pressed: %lu, mask: %u", keysym, cleanmask);
    
    /* Check against configured key bindings */
    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
        if (keysym == keys[i].keysym && cleanmask == keys[i].mod) {
            if (keys[i].func) {
                keys[i].func(keys[i].arg);
            }
            return;
        }
    }
}

static void handle_map_request(XMapRequestEvent *e) {
    Window w = e->window;
    
    /* Set border */
    XSetWindowBorderWidth(wm.display, w, BORDER_WIDTH);
    XSetWindowBorder(wm.display, w, BORDER_COLOR);
    
    /* Map the window */
    XMapWindow(wm.display, w);
    
    /* Focus the new window */
    focus_window(w);
    
    /* Select events for this window */
    XSelectInput(wm.display, w, EnterWindowMask | FocusChangeMask);
}

static void handle_configure_request(XConfigureRequestEvent *e) {
    XWindowChanges changes;
    
    changes.x = e->x;
    changes.y = e->y;
    changes.width = e->width;
    changes.height = e->height;
    changes.border_width = e->border_width;
    changes.sibling = e->above;
    changes.stack_mode = e->detail;
    
    XConfigureWindow(wm.display, e->window, e->value_mask, &changes);
}

static void handle_unmap_notify(XUnmapEvent *e) {
    if (e->window == wm.focused) {
        wm.focused = None;
    }
}

static void handle_destroy_notify(XDestroyWindowEvent *e) {
    if (e->window == wm.focused) {
        wm.focused = None;
    }
}

static void handle_enter_notify(XCrossingEvent *e) {
    if (FOCUS_FOLLOWS_MOUSE && e->window != wm.root && e->window != wm.focused) {
        focus_window(e->window);
    }
}

static void handle_button_press(XButtonEvent *e) {
    if (e->state & MOD_KEY) {
        if (e->button == MOVE_BUTTON && e->subwindow != None) {
            /* Start moving window */
            XRaiseWindow(wm.display, e->subwindow);
            focus_window(e->subwindow);
            
            XWindowAttributes attr;
            XGetWindowAttributes(wm.display, e->subwindow, &attr);
            
            XEvent ev;
            int orig_x = attr.x;
            int orig_y = attr.y;
            
            do {
                XMaskEvent(wm.display, PointerMotionMask | ButtonReleaseMask, &ev);
                if (ev.type == MotionNotify) {
                    int new_x = orig_x + (ev.xmotion.x_root - e->x_root);
                    int new_y = orig_y + (ev.xmotion.y_root - e->y_root);
                    XMoveWindow(wm.display, e->subwindow, new_x, new_y);
                }
            } while (ev.type != ButtonRelease);
            
        } else if (e->button == RESIZE_BUTTON && e->subwindow != None) {
            /* Start resizing window */
            XRaiseWindow(wm.display, e->subwindow);
            focus_window(e->subwindow);
            
            XWindowAttributes attr;
            XGetWindowAttributes(wm.display, e->subwindow, &attr);
            
            XEvent ev;
            int orig_width = attr.width;
            int orig_height = attr.height;
            
            do {
                XMaskEvent(wm.display, PointerMotionMask | ButtonReleaseMask, &ev);
                if (ev.type == MotionNotify) {
                    int new_width = orig_width + (ev.xmotion.x_root - e->x_root);
                    int new_height = orig_height + (ev.xmotion.y_root - e->y_root);
                    
                    if (new_width > 50 && new_height > 50) {
                        XResizeWindow(wm.display, e->subwindow, new_width, new_height);
                    }
                }
            } while (ev.type != ButtonRelease);
        }
    } else if (CLICK_TO_FOCUS && e->window != wm.root) {
        focus_window(e->window);
    }
}

static void focus_window(Window w) {
    if (w == wm.focused || w == None || w == wm.root) {
        return;
    }
    
    /* Unfocus previous window */
    if (wm.focused != None) {
        XSetWindowBorder(wm.display, wm.focused, BORDER_COLOR);
    }
    
    /* Focus new window */
    wm.focused = w;
    XSetWindowBorder(wm.display, w, BORDER_FOCUS_COLOR);
    XSetInputFocus(wm.display, w, RevertToPointerRoot, CurrentTime);
    XRaiseWindow(wm.display, w);
}

static void grab_keys(void) {
    /* Grab all configured key combinations */
    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
        KeyCode code = XKeysymToKeycode(wm.display, keys[i].keysym);
        if (code) {
            XGrabKey(wm.display, code, keys[i].mod, wm.root, True,
                     GrabModeAsync, GrabModeAsync);
            /* Also grab with numlock and capslock */
            XGrabKey(wm.display, code, keys[i].mod | LockMask, wm.root, True,
                     GrabModeAsync, GrabModeAsync);
            XGrabKey(wm.display, code, keys[i].mod | Mod2Mask, wm.root, True,
                     GrabModeAsync, GrabModeAsync);
            XGrabKey(wm.display, code, keys[i].mod | LockMask | Mod2Mask, wm.root, True,
                     GrabModeAsync, GrabModeAsync);
        }
    }
}

static void grab_buttons(void) {
    /* Grab mouse buttons for window management */
    XGrabButton(wm.display, MOVE_BUTTON, MOD_KEY, wm.root, True,
                ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                GrabModeAsync, GrabModeAsync, None, None);
    
    XGrabButton(wm.display, RESIZE_BUTTON, MOD_KEY, wm.root, True,
                ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                GrabModeAsync, GrabModeAsync, None, None);
}

/* Key binding function implementations */
void spawn(const char *cmd) {
    if (fork() == 0) {
        if (wm.display) {
            close(ConnectionNumber(wm.display));
        }
        setsid();
        execl("/bin/sh", "sh", "-c", cmd, NULL);
        fprintf(stderr, "Failed to spawn: %s\n", cmd);
        exit(1);
    }
}

void quit_wm(const char *arg) {
    (void)arg; /* Unused parameter */
    wm.running = 0;
}

void close_window(const char *arg) {
    (void)arg; /* Unused parameter */
    if (wm.focused != None && wm.focused != wm.root) {
        XEvent ke;
        ke.type = ClientMessage;
        ke.xclient.window = wm.focused;
        ke.xclient.message_type = XInternAtom(wm.display, "WM_PROTOCOLS", True);
        ke.xclient.format = 32;
        ke.xclient.data.l[0] = XInternAtom(wm.display, "WM_DELETE_WINDOW", True);
        ke.xclient.data.l[1] = CurrentTime;
        
        XSendEvent(wm.display, wm.focused, False, NoEventMask, &ke);
    }
}

void toggle_fullscreen(const char *arg) {
    (void)arg; /* Unused parameter */
    if (wm.focused != None && wm.focused != wm.root) {
        /* Basic fullscreen toggle - move to 0,0 and resize to screen size */
        XWindowAttributes attr;
        XGetWindowAttributes(wm.display, wm.focused, &attr);
        
        static int saved_x, saved_y, saved_width, saved_height;
        static Window fullscreen_window = None;
        
        if (fullscreen_window == wm.focused) {
            /* Restore window */
            XMoveResizeWindow(wm.display, wm.focused, saved_x, saved_y, 
                             saved_width, saved_height);
            fullscreen_window = None;
        } else {
            /* Save current geometry and go fullscreen */
            saved_x = attr.x;
            saved_y = attr.y;
            saved_width = attr.width;
            saved_height = attr.height;
            
            int screen_width = DisplayWidth(wm.display, wm.screen);
            int screen_height = DisplayHeight(wm.display, wm.screen);
            
            XMoveResizeWindow(wm.display, wm.focused, 0, 0, 
                             screen_width, screen_height);
            fullscreen_window = wm.focused;
        }
    }
}

void next_window(const char *arg) {
    (void)arg; /* Unused parameter */
    /* Simple window cycling - this is a basic implementation */
    Window root_return, parent_return;
    Window *children;
    unsigned int nchildren;
    
    if (XQueryTree(wm.display, wm.root, &root_return, &parent_return,
                   &children, &nchildren)) {
        for (unsigned int i = 0; i < nchildren; i++) {
            if (children[i] == wm.focused && i + 1 < nchildren) {
                focus_window(children[i + 1]);
                break;
            } else if (i == nchildren - 1 && nchildren > 0) {
                focus_window(children[0]);
                break;
            }
        }
        XFree(children);
    }
}

void prev_window(const char *arg) {
    (void)arg; /* Unused parameter */
    /* Simple window cycling - this is a basic implementation */
    Window root_return, parent_return;
    Window *children;
    unsigned int nchildren;
    
    if (XQueryTree(wm.display, wm.root, &root_return, &parent_return,
                   &children, &nchildren)) {
        for (unsigned int i = 0; i < nchildren; i++) {
            if (children[i] == wm.focused && i > 0) {
                focus_window(children[i - 1]);
                break;
            } else if (i == 0 && nchildren > 0) {
                focus_window(children[nchildren - 1]);
                break;
            }
        }
        XFree(children);
    }
}

static int xerror_handler(Display *display, XErrorEvent *e) {
    char error_text[256];
    XGetErrorText(display, e->error_code, error_text, sizeof(error_text));
    fprintf(stderr, "X Error: %s\n", error_text);
    return 0;
}

static void signal_handler(int sig) {
    wm.running = 0;
}