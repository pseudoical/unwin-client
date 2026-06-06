#include <assert.h>
#include <stdio.h>

#include "../../cef/include/capi/cef_request_capi.h"

#include "../../include/my_cef.h"

static struct _cef_resource_request_handler_t *get_resource_request_handler(
    struct _cef_request_handler_t *_self,
    struct _cef_browser_t *browser,
    struct _cef_frame_t *frame,
    struct _cef_request_t *request,
    int is_navigation,
    int is_download,
    const cef_string_t *request_initiator,
    int *disable_default_handling
) {
    (void)browser;
    (void)frame;
    (void)request;
    (void)is_navigation;
    (void)is_download;
    (void)request_initiator;
    (void)disable_default_handling;

    my_cef_request_handler_t *self = (void *)_self;

    return &self->my_resource_request->handler;
}

extern void my_cef_request_handler_init(
    my_cef_request_handler_t *self,
    my_cef_resource_request_handler_t *my_resource_request
) {
    static_assert(sizeof(self->handler) == sizeof(cef_request_handler_t));
    assert(self != NULL);
    assert(my_resource_request != NULL);

    self->my_resource_request = my_resource_request;

    self->handler.base.size = sizeof(self->handler);
    self->handler.get_resource_request_handler = get_resource_request_handler;
}
