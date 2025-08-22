#ifndef _MEMPOOL_H_
#define _MEMPOOL_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef union __mempool_chunk __mempool_chunk_t;

typedef struct mempool {
    arena_t *arena;
    __mempool_chunk_t *freelist;
    size_t  chunk_size;
} mempool_t;

#ifdef __cplusplus
}
#endif

#endif // _MEMPOOL_H_
