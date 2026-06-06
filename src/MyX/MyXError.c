#include <stdio.h>

#include <X11/Xlib.h>

#include "../../include/MyX.h"

extern int MyXErrorHandler(Display *display, XErrorEvent *event) {
    char error_message[1024];

    XGetErrorText(display, event->error_code, error_message, sizeof(error_message));

    fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
    fprintf(stderr,
        "MyXErrorHandler:\n"
        "  error_code    = %hhu\n"
        "  error_message = %s\n"
        "  request_code  = %hhu\n"
        "  minor_code    = %hhu\n"
        "  resourceid    = 0x%lx\n",
        event->error_code,
        error_message,
        event->request_code,
        event->minor_code,
        event->resourceid
    );

    return 0;
}

extern int MyXIOErrorHandler(Display *display) {
    (void)display;

    fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
    fprintf(stderr, "MyXIOErrorHandler: Fatal I/O error\n");

    return 0;
}
