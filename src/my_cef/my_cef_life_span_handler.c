#include <assert.h>

#include "../../cef/include/capi/cef_life_span_handler_capi.h"

#include "../../include/my_cef.h"

static void on_before_close(struct _cef_life_span_handler_t *self, struct _cef_browser_t *browser) {
    (void)self;
    (void)browser;

    cef_quit_message_loop();
}

extern void my_cef_life_span_handler_init(my_cef_life_span_handler_t *self) {
    static_assert(sizeof(self->handler) == sizeof(cef_life_span_handler_t));
    assert(self != NULL);

    self->handler.base.size = sizeof(self->handler);
    self->handler.on_before_close = on_before_close;
}
