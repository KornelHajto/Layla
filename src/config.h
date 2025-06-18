#ifndef CONFIG_H
#define CONFIG_H

#include <X11/keysym.h>

/* Key bindings */
#define MOD_KEY Mod4Mask  /* Super/Windows key - change to Mod1Mask for Alt */

/* Appearance */
#define BORDER_WIDTH 2
#define BORDER_COLOR 0x444444
#define BORDER_FOCUS_COLOR 0x005577
#define BORDER_URGENT_COLOR 0xff0000

/* Window management */
#define FOCUS_FOLLOWS_MOUSE 1
#define CLICK_TO_FOCUS 1
#define AUTO_RAISE 1

/* Gaps (in pixels) */
#define OUTER_GAPS 10
#define INNER_GAPS 5

/* Default programs */
#define TERMINAL_CMD "alacritty"
#define MENU_CMD "dmenu_run"
#define BROWSER_CMD "firefox"
#define FILE_MANAGER_CMD "thunar"

/* Key bindings structure */
typedef struct {
    unsigned int mod;
    KeySym keysym;
    void (*func)(const char *);
    const char *arg;
} Key;

/* Function prototypes for key bindings */
void spawn(const char *cmd);
void quit_wm(const char *arg);
void close_window(const char *arg);
void toggle_fullscreen(const char *arg);
void next_window(const char *arg);
void prev_window(const char *arg);

/* Key bindings configuration */
static Key keys[] = {
    /* modifier          key              function          argument */
    { MOD_KEY,           XK_Return,       spawn,            TERMINAL_CMD },
    { MOD_KEY,           XK_d,            spawn,            MENU_CMD },
    { MOD_KEY,           XK_w,            spawn,            BROWSER_CMD },
    { MOD_KEY,           XK_f,            spawn,            FILE_MANAGER_CMD },
    { MOD_KEY,           XK_q,            close_window,     NULL },
    { MOD_KEY|ShiftMask, XK_q,            quit_wm,          NULL },
    { MOD_KEY,           XK_F11,          toggle_fullscreen, NULL },
    { MOD_KEY,           XK_Tab,          next_window,      NULL },
    { MOD_KEY|ShiftMask, XK_Tab,          prev_window,      NULL },
};

/* Mouse bindings */
#define MOVE_BUTTON Button1
#define RESIZE_BUTTON Button3

/* Window rules */
typedef struct {
    const char *class;
    const char *instance;
    const char *title;
    int floating;
    int fullscreen;
} Rule;

static Rule rules[] = {
    /* class        instance    title           floating    fullscreen */
    { "Gimp",       NULL,       NULL,          1,          0 },
    { "Firefox",    NULL,       NULL,          0,          0 },
    { "mpv",        NULL,       NULL,          1,          1 },
    { "feh",        NULL,       NULL,          1,          0 },
    { "Pavucontrol", NULL,      NULL,          1,          0 },
};

/* Bar configuration */
#define SHOW_BAR 0
#define BAR_HEIGHT 24
#define BAR_POSITION_TOP 1

/* Workspace configuration */
#define WORKSPACE_COUNT 9
static const char *workspace_names[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9"
};

/* Layout configuration */
typedef enum {
    FLOATING,
    TILED,
    MONOCLE
} Layout;

#define DEFAULT_LAYOUT FLOATING

/* Animation settings */
#define ENABLE_ANIMATIONS 0
#define ANIMATION_SPEED 200  /* milliseconds */

/* Debugging */
#ifdef DEBUG
#define DEBUG_PRINT(fmt, ...) \
    do { \
        fprintf(stderr, "[DEBUG] %s:%d: " fmt "\n", \
                __FILE__, __LINE__, ##__VA_ARGS__); \
    } while (0)
#else
#define DEBUG_PRINT(fmt, ...) do {} while (0)
#endif

#endif /* CONFIG_H */