#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>

#include <X11/Xlib.h>

#include "../../include/my_pthread.h"

static void on_client_message_wm_delete(my_pthread_xevent_arg *self) {
    assert(self != NULL);

    cef_browser_t *browser = self->browser;
    cef_browser_host_t *host = browser->get_host(browser);

    host->close_browser(host, 1);
}

static void on_configure_notify(my_pthread_xevent_arg *self, XEvent *event) {
    assert(self != NULL);

    Display *display = self->display;
    cef_browser_t *browser = self->browser;
    cef_browser_host_t *host = browser->get_host(browser);
    cef_window_handle_t window = host->get_window_handle(host);

    XResizeWindow(display, window, event->xconfigure.width, event->xconfigure.height);
    XFlush(display);
}

static void on_xevent(my_pthread_xevent_arg *self, XEvent *event) {
    assert(self != NULL);
    assert(event != NULL);

    switch (event->type) {
    case ClientMessage: {
        Atom client_message = event->xclient.data.l[0];

        if (client_message == self->wm_delete) {
            on_client_message_wm_delete(self);
        }
    } break;
    case ConfigureNotify:
        on_configure_notify(self, event);
        break;
    }
}

static void *on_xevent_routine(void *arg) {
    assert(arg != NULL);

    my_pthread_xevent_arg *self = arg;
    Display *display = self->display;

    int epfd = epoll_create1(0);

    if (epfd == -1) {
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
        perror("epoll_create1");
        return NULL;
    }

    int x11_fd = XConnectionNumber(display);
    struct epoll_event x11_event = { .events = EPOLLIN, .data.fd = x11_fd };

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, x11_fd, &x11_event) == -1) {
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
        perror("epoll_ctl");
        goto cleanup;
    }

    int wake_fd = self->wake_fd;
    struct epoll_event wake_event = { .events = EPOLLIN, .data.fd = wake_fd };

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, wake_fd, &wake_event) == -1) {
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
        perror("epoll_ctl");
        goto cleanup;
    }

    struct epoll_event events[2];
    const int max = sizeof(events) / sizeof(*events);
    XEvent event;

    while (true) {
        int nfds = epoll_wait(epfd, events, max, -1);

        if (nfds == -1) {
            if (errno == EINTR) {
                continue;
            }

            fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
            perror("epoll_wait");
            goto cleanup;
        }

        for (int i = 0; i < nfds; ++i) {
            int fd = events[i].data.fd;

            if (fd == x11_fd) {
                while (XPending(display) > 0) {
                    XNextEvent(display, &event);
                    on_xevent(self, &event);
                }
            } else if (fd == wake_fd) {
                goto cleanup;
            }
        }
    }

cleanup:
    if (close(epfd) == -1) {
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
        perror("close");
    }

    return NULL;
}

extern bool my_pthread_xevent_init(
    my_pthread_xevent_arg *self,
    Display *display,
    Window window,
    cef_browser_t *browser
) {
    assert(self != NULL);
    assert(display != NULL);
    assert(browser != NULL);

    XSelectInput(display, window, StructureNotifyMask);

    Atom wm_delete = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wm_delete, 1);

    XFlush(display);

    self->display = display;
    self->browser = browser;
    self->wm_delete = wm_delete;
    self->wake_fd = eventfd(0, EFD_NONBLOCK);

    if (self->wake_fd == -1) {
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
        perror("eventfd");
        return false;
    }

    return true;
}

extern bool my_pthread_xevent_deinit(my_pthread_xevent_arg *self) {
    assert(self != NULL);

    if (close(self->wake_fd) == -1) {
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
        perror("close");
        return false;
    }

    return true;
}

extern bool my_pthread_xevent_start(my_pthread_xevent_arg *self) {
    assert(self != NULL);

    int err = pthread_create(&self->thread, NULL, on_xevent_routine, (void *)self);

    if (err != 0) {
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
        fprintf(stderr, "pthread_create: %s\n", strerror(err));
        return false;
    }

    return true;
}

extern bool my_pthread_xevent_stop(my_pthread_xevent_arg *self) {
    assert(self != NULL);

    uint64_t buf = 1;

    if (write(self->wake_fd, &buf, sizeof(buf)) == -1) {
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
        perror("write");
    }

    int errnum = pthread_join(self->thread, NULL);

    if (errnum != 0) {
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
        fprintf(stderr, "pthread_join: %s\n", strerror(errnum));
        return false;
    }

    return true;
}
