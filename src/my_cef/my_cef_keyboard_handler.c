#include <assert.h>

#include <X11/keysym.h>
#include <X11/Xlib.h>

#include "../../cef/include/capi/cef_keyboard_handler_capi.h"

#include "../../include/my_cef.h"

// https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
typedef enum _VirtualKeyCodes {
    VK_F11 = 0x7A,
} VirtualKeyCodes;

static void on_f11(const my_cef_keyboard_handler_t *self) {
    assert(self != NULL);

    Display *display = self->display;
    Window window = self->window;

    Atom wm_state = XInternAtom(display, "_NET_WM_STATE", False);
    Atom fullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);

    XEvent event_send = {
        .xclient = {
            .type = ClientMessage,
            .window = window,
            .message_type = wm_state,
            .format = 32,
            .data.l = {
                [0] = 2, // _NET_WM_STATE_TOGGLE
                [1] = fullscreen, // _NET_WM_STATE_FULLSCREEN
                [2] = 0, // unused
                [3] = 1, // application
                [4] = 0, // reserved
            },
        },
    };

    long event_mask = SubstructureRedirectMask | SubstructureNotifyMask;
    XSendEvent(display, DefaultRootWindow(display), False, event_mask, &event_send);
    XFlush(display);
}

static int on_pre_key_event(
    struct _cef_keyboard_handler_t *_self,
    struct _cef_browser_t *browser,
    const cef_key_event_t *event,
    cef_event_handle_t os_event,
    int *is_keyboard_shortcut
) {
    (void)browser;
    (void)os_event;
    (void)is_keyboard_shortcut;

    my_cef_keyboard_handler_t *self = (void *)_self;

    if (event->type == KEYEVENT_RAWKEYDOWN) {
        switch ((VirtualKeyCodes)event->windows_key_code) {
        case VK_F11:
            on_f11(self);
            return 1;
        }
    }

    return 0;
}

extern void my_cef_keyboard_handler_init(my_cef_keyboard_handler_t *self, Display *display, Window window) {
    static_assert(sizeof(self->handler) == sizeof(cef_keyboard_handler_t));
    assert(self != NULL);
    assert(display != NULL);

    self->display = display;
    self->window = window;

    self->handler.base.size = sizeof(self->handler);
    self->handler.on_pre_key_event = on_pre_key_event;
}
