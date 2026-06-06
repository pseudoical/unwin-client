#include <assert.h>
#include <stdatomic.h>

#include "../../cef/include/capi/cef_base_capi.h"

#include "../../include/my_cef.h"

/**
 * References:
 * cef/include/base/cef_atomic_ref_count.h
 * cef/include/cef_base.h
 */

static void add_ref(struct _cef_base_ref_counted_t *self) {
    RefCount *ref = (void *)((char *)self + self->size);

    atomic_fetch_add_explicit(&ref->count, 1, memory_order_relaxed);
}

static int release(struct _cef_base_ref_counted_t *self) {
    RefCount *ref = (void *)((char *)self + self->size);

    assert(ref->free != NULL);

    if (!(atomic_fetch_sub_explicit(&ref->count, 1, memory_order_acq_rel) != 1)) {
        ref->free(self);
        return 1;
    }

    return 0;
}

static int has_one_ref(struct _cef_base_ref_counted_t *self) {
    RefCount *ref = (void *)((char *)self + self->size);

    return atomic_load_explicit(&ref->count, memory_order_acquire) == 1 ? 1 : 0;
}

static int has_at_least_one_ref(struct _cef_base_ref_counted_t *self) {
    RefCount *ref = (void *)((char *)self + self->size);

    return !(atomic_load_explicit(&ref->count, memory_order_acquire) == 0) ? 1 : 0;
}

extern void my_cef_base_ref_counted_init(cef_base_ref_counted_t *self, RefCount *ref, void (*free)(void *self)) {
    assert(self != NULL);
    assert(ref != NULL);
    assert(free != NULL);

    self->add_ref = add_ref;
    self->release = release;
    self->has_one_ref = has_one_ref;
    self->has_at_least_one_ref = has_at_least_one_ref;

    atomic_init(&ref->count, 1);
    ref->free = free;
}
