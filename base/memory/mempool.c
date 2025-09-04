#include "arena.h"
#include "mempool.h"

#ifdef __cplusplus
extern "C" {
#endif

union __mempool_chunk {
    __mempool_chunk_t *next;
};

void mempool_init(mempool_t *self, arena_t *arena, size_t chunk_size) {
    chunk_size = (chunk_size >= sizeof(__mempool_chunk_t)) ? chunk_size : sizeof(__mempool_chunk_t);
    *self = (mempool_t){
        .arena = arena,
        .freelist = NULL,
        .chunk_size = chunk_size
    };
}

void *mempool_get(mempool_t *self) {
    __mempool_chunk_t *c;
    if (!self->freelist)
        return arena_alloc(self->arena, self->chunk_size);
    c = self->freelist;
    self->freelist = c->next;
    return c;
}

void mempool_put(mempool_t *self, void *v) {
    __mempool_chunk_t *c = (__mempool_chunk_t*) v;
    c->next = self->freelist;
    self->freelist = c;
}

#ifdef __cplusplus
}
#endif
