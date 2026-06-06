#include <assert.h>
#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "../../include/MyX.h"

extern Window MyXCreateWindow(Display *display, unsigned int width, unsigned int height, const char *name) {
    assert(display != NULL);

    int screen = DefaultScreen(display);

    XSizeHints hints = {
        .flags = USPosition | USSize,
        .x = (DisplayWidth(display, screen) - width) / 2,
        .y = (DisplayHeight(display, screen) - height) / 2,
        .width = width,
        .height = height,
    };

    Window window = XCreateSimpleWindow(
        display,
        RootWindow(display, screen),
        hints.x,
        hints.y,
        hints.width,
        hints.height,
        0,
        BlackPixel(display, screen),
        BlackPixel(display, screen)
    );

    XSetWMNormalHints(display, window, &hints);
    XStoreName(display, window, name);
    XMapWindow(display, window);
    XFlush(display);

    return window;
}

extern void MyXMaximizeWindow(Display *display, Window window) {
    Atom wm_state = XInternAtom(display, "_NET_WM_STATE", False);
    Atom max_horz = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
    Atom max_vert = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", False);

    XEvent event_send = {
        .xclient = {
            .type = ClientMessage,
            .window = window,
            .message_type = wm_state,
            .format = 32,
            .data.l = {
                [0] = 1, // _NET_WM_STATE_ADD
                [1] = max_horz, // _NET_WM_STATE_MAXIMIZED_HORZ
                [2] = max_vert, // _NET_WM_STATE_MAXIMIZED_VERT
                [3] = 1, // application
                [4] = 0, // reserved
            },
        },
    };

    long event_mask = SubstructureRedirectMask | SubstructureNotifyMask;
    XSendEvent(display, DefaultRootWindow(display), False, event_mask, &event_send);
    XFlush(display);
}
