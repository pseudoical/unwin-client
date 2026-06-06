#include <assert.h>

#include "../../cef/include/capi/cef_client_capi.h"

#include "../../include/my_cef.h"

static struct _cef_life_span_handler_t *get_life_span_handler(struct _cef_client_t *_self) {
    my_cef_client_t *self = (void *)_self;

    return &self->my_life_span->handler;
}

static struct _cef_keyboard_handler_t *get_keyboard_handler(struct _cef_client_t *_self) {
    my_cef_client_t *self = (void *)_self;

    return &self->my_keyboard->handler;
}

static struct _cef_load_handler_t *get_load_handler(struct _cef_client_t *_self) {
    my_cef_client_t *self = (void *)_self;

    return &self->my_load->handler;
}

static struct _cef_request_handler_t *get_request_handler(struct _cef_client_t *_self) {
    my_cef_client_t *self = (void *)_self;

    return &self->my_request->handler;
}

extern void my_cef_client_init(
    my_cef_client_t *self,
    my_cef_life_span_handler_t *my_life_span,
    my_cef_keyboard_handler_t *my_keyboard,
    my_cef_load_handler_t *my_load,
    my_cef_request_handler_t *my_request
) {
    static_assert(sizeof(self->client) == sizeof(cef_client_t));
    assert(self != NULL);
    assert(my_life_span != NULL);
    assert(my_keyboard != NULL);
    assert(my_load != NULL);
    assert(my_request != NULL);

    self->my_life_span = my_life_span;
    self->my_keyboard = my_keyboard;
    self->my_load = my_load;
    self->my_request = my_request;

    self->client.base.size = sizeof(self->client);
    self->client.get_life_span_handler = get_life_span_handler;
    self->client.get_keyboard_handler = get_keyboard_handler;
    self->client.get_load_handler = get_load_handler;
    self->client.get_request_handler = get_request_handler;
}
