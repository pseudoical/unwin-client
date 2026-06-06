#ifndef MyX_h
#define MyX_h

#include <X11/Xlib.h>

extern int MyXErrorHandler(Display *display, XErrorEvent *event);

extern int MyXIOErrorHandler(Display *display);

extern Window MyXCreateWindow(Display *display, unsigned int width, unsigned int height, const char *name);

extern void MyXMaximizeWindow(Display *display, Window window);

#endif // MyX_h
