#ifndef my_cef_h
#define my_cef_h

#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>

#include <X11/Xlib.h>

#include "../cef/include/capi/cef_app_capi.h"

// Must be declared after struct's `handler`
// Uses `handler.base.size` as an offset to itself
typedef struct _RefCount {
    atomic_int count;
    void (*free)(void *self);
} RefCount;

typedef struct _my_cef_life_span_handler_t {
    cef_life_span_handler_t handler;
} my_cef_life_span_handler_t;

typedef struct _my_cef_keyboard_handler_t {
    cef_keyboard_handler_t handler;
    Display *display;
    Window window;
} my_cef_keyboard_handler_t;

static_assert(offsetof(my_cef_keyboard_handler_t, display) == sizeof(cef_keyboard_handler_t));

typedef struct _my_cef_load_handler_t {
    cef_load_handler_t handler;
} my_cef_load_handler_t;

typedef struct _ResourceArena {
    char *name;
    char *data;
} ResourceArena;

typedef struct _Resource {
    ResourceArena arena;
    size_t name_len;
    int64_t data_len;
    cef_string_userfree_utf16_t mime;
    int64_t offset;
} Resource;

typedef struct _my_cef_resource_handler_t {
    cef_resource_handler_t handler;
    RefCount ref;
    Resource *resource;
} my_cef_resource_handler_t;

static_assert(offsetof(my_cef_resource_handler_t, ref) == sizeof(cef_resource_handler_t));

typedef struct _my_cef_resource_request_handler_t {
    cef_resource_request_handler_t handler;
    Resource *resources;
    size_t resources_count;
} my_cef_resource_request_handler_t;

static_assert(offsetof(my_cef_resource_request_handler_t, resources) == sizeof(cef_resource_request_handler_t));

typedef struct _my_cef_request_handler_t {
    cef_request_handler_t handler;
    my_cef_resource_request_handler_t *my_resource_request;
} my_cef_request_handler_t;

static_assert(offsetof(my_cef_request_handler_t, my_resource_request) == sizeof(cef_request_handler_t));

typedef struct _my_cef_client_t {
    cef_client_t client;
    my_cef_life_span_handler_t *my_life_span;
    my_cef_keyboard_handler_t *my_keyboard;
    my_cef_load_handler_t *my_load;
    my_cef_request_handler_t *my_request;
} my_cef_client_t;

static_assert(offsetof(my_cef_client_t, my_life_span) == sizeof(cef_client_t));

extern void my_cef_base_ref_counted_init(cef_base_ref_counted_t *self, RefCount *ref, void (*free)(void *self));

extern void my_cef_client_init(
    my_cef_client_t *self,
    my_cef_life_span_handler_t *my_life_span,
    my_cef_keyboard_handler_t *my_keyboard,
    my_cef_load_handler_t *my_load,
    my_cef_request_handler_t *my_request
);

extern void my_cef_keyboard_handler_init(my_cef_keyboard_handler_t *self, Display *display, Window window);

extern void my_cef_life_span_handler_init(my_cef_life_span_handler_t *self);

extern void my_cef_load_handler_init(my_cef_load_handler_t *self);

extern my_cef_resource_handler_t *my_cef_resource_handler_create(void);

extern bool my_cef_resource_request_handler_init(my_cef_resource_request_handler_t *self);

extern void my_cef_resource_request_handler_deinit(my_cef_resource_request_handler_t *self);

extern void my_cef_request_handler_init(
    my_cef_request_handler_t *self,
    my_cef_resource_request_handler_t *my_resource_request
);

#endif // my_cef_h
