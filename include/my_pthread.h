#ifndef my_pthread_h
#define my_pthread_h

#include <pthread.h>

#include <X11/Xlib.h>

#include "../cef/include/capi/cef_browser_capi.h"

typedef struct _my_pthread_xevent_arg {
    Display *display;
    cef_browser_t *browser;
    Atom wm_delete;
    int wake_fd;
    pthread_t thread;
} my_pthread_xevent_arg;

extern bool my_pthread_xevent_init(
    my_pthread_xevent_arg *self,
    Display *display,
    Window window,
    cef_browser_t *browser
);

extern bool my_pthread_xevent_deinit(my_pthread_xevent_arg *self);

extern bool my_pthread_xevent_start(my_pthread_xevent_arg *self);

extern bool my_pthread_xevent_stop(my_pthread_xevent_arg *self);

#endif // my_pthread_h
