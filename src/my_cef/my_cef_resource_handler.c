#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "../../cef/include/capi/cef_resource_handler_capi.h"

#include "../../include/my_cef.h"

static int open(
    struct _cef_resource_handler_t *_self,
    struct _cef_request_t *request,
    int *handle_request,
    struct _cef_callback_t *callback
) {
    (void)request;
    (void)callback;

    my_cef_resource_handler_t *self = (void *)_self;

    if (self->resource != NULL) {
        self->resource->offset = 0;
        *handle_request = 1;
        return 1;
    }

    *handle_request = 0;
    return 0;
}

static void get_response_headers(
    struct _cef_resource_handler_t *_self,
    struct _cef_response_t *response,
    int64_t *response_length,
    cef_string_t *redirectUrl
) {
    (void)redirectUrl;

    my_cef_resource_handler_t *self = (void *)_self;

    const char ok[] = "OK";
    cef_string_utf16_t ok16 = {0};
    cef_string_ascii_to_utf16(ok, sizeof(ok) - 1, &ok16);

    response->set_status(response, 200);
    response->set_status_text(response, &ok16);
    response->set_mime_type(response, self->resource->mime);

    *response_length = self->resource->data_len;
}

static int read_(
    struct _cef_resource_handler_t *_self,
    void *data_out,
    int bytes_to_read,
    int *bytes_read,
    struct _cef_resource_read_callback_t *callback
) {
    (void)callback;

    my_cef_resource_handler_t *self = (void *)_self;

    int64_t remaining = self->resource->data_len - self->resource->offset;

    if (remaining <= 0) {
        *bytes_read = 0;
        return 0;
    }

    int bytes = remaining < bytes_to_read ? remaining : bytes_to_read;
    memcpy(data_out, self->resource->arena.data + self->resource->offset, bytes);

    self->resource->offset += bytes;
    *bytes_read = bytes;

    return 1;
}

static void cancel(struct _cef_resource_handler_t *self) {
    (void)self;

    (void)0;
}

static void destory(my_cef_resource_handler_t *self) {
    assert(self != NULL);

    free(self);
}

extern my_cef_resource_handler_t *my_cef_resource_handler_create(void) {
    my_cef_resource_handler_t *self = calloc(1, sizeof(*self));

    static_assert(sizeof(self->handler) == sizeof(cef_resource_handler_t));

    if (self == NULL) {
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
        perror("calloc");
        return NULL;
    }

    self->handler.base.size = sizeof(self->handler);
    self->handler.get_response_headers = get_response_headers;
    self->handler.open = open;
    self->handler.read = read_;
    self->handler.cancel = cancel;

    self->resource = NULL;

    my_cef_base_ref_counted_init(&self->handler.base, &self->ref, (void (*)(void *))destory);

    return self;
}
