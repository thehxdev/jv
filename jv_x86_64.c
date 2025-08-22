#include <sys/mman.h>

#define JV_ARGS_LIMIT 6
#define JV_STACK_SIZE 4096

#define jv_args(...) \
    ((void*[JV_ARGS_LIMIT]){ __VA_ARGS__ })

#define jv_yield     jv_await(-1, JV_NONE)
#define jv_join(tid) jv_await((tid), JV_TASK_TERMINATE)

#define jv_concat_(A, B) A##B
#define jv_concat(A, B) jv_concat_(A, B)
#define jv_static_assert(cond, id) \
    extern char jv_concat(id, __LINE__)[ ((cond)) ? 1 : -1 ]

// static assert to make sure that sizeof(void*) == 8
jv_static_assert(sizeof(void*) == 8, assert_sizeof_pointer);

enum {
    // just return to event-loop and try to continue other tasks
    JV_NONE,
    // wait for a file descriptor to become readable
    JV_READ,
    // wait for a file descriptor to become writable
    JV_WRITE,
    // wait for a task to terminate
    JV_TASK_TERMINATE
};

enum {
    JV_STATE_NEW,
    JV_STATE_READY,
    JV_STATE_WAITING,
    JV_STATE_TERMINATED
};

enum {
    JV_R_RBX,
    JV_R_RBP,
    JV_R_R12,
    JV_R_R13,
    JV_R_R14,
    JV_R_R15,
    JV_R_RSP,
    JV_R_RIP,

    JV_R_COUNT
};

typedef struct jv_task {
    // Order of these fields matter. Don't change them!
    uintptr_t regs[JV_R_COUNT];
    uintptr_t first_run;
    void *args[JV_ARGS_LIMIT];

    long state, awaitable, event;
    struct jv_task *next, *prev;
    unsigned char *stack;
} jv_task_t;

// The task id is a unique identifier of a task. You MUST NEVER
// use/reuse the same task id after it's termination as JV itself
// may reuse the same id for new tasks.
typedef uintptr_t jv_tid_t;


static mempool_t stack_pool;
static mempool_t task_pool;

static size_t actives  = 0;
static arena_t *gmem   = NULL;
static jv_task_t *init = NULL; // like init (pid 1) process in Linux
static jv_task_t *curr = NULL;
static jv_task_t *tail = NULL;


extern void jv_task_store(jv_task_t *task, void *rsp, void *rip);
extern void jv_task_restore(jv_task_t *task);

int __attribute__((naked))
jv_await(long awaitable, long event)
{
    // call jv_task_store
    __asm__ __volatile__ (
        "movq  %%rdi, %%r10\n"
        "movq  %%rsi, %%r11\n"
        "movq  %0, %%rdi\n"
        "leaq 8(%%rsp), %%rsi\n"
        "movq  (%%rsp), %%rdx\n"
        "call  jv_task_store\n"
        :
        : "m" (curr)
        : "r10", "r11", "rdi", "rsi", "rdx" );

    __asm__ __volatile__ (
        "movq %%r10, %%rdi\n"
        "movq %%r11, %%rsi\n"
        "jmp jv_task_switch"
        :
        :
        : "rdi", "rsi");
}

void jv_task_switch(long awaitable, long event)
{
    long i;
    if (event == JV_TASK_TERMINATE) {
        curr->state = JV_STATE_WAITING;
        curr->awaitable = awaitable;
        curr->event = event;
        curr = curr->next;
    }
    for (i = 0; i < actives; ++i) {
        if (curr->state == JV_STATE_READY)
            break;
        curr = curr->next;
    }
    if (i == actives)
        curr = init;
    jv_task_restore(curr);
}

void jv_task_end(void)
{
    jv_task_t *ended = curr, *tmp;
    tmp = ended->next;
    // tmp = ended->prev->next = ended->next;
    // ended->next->prev = ended->prev;
    // actives--;

    long i;
    for (i = 0; i < actives; ++i) {
        if (tmp->awaitable == (jv_tid_t)ended) {
            assert((uintptr_t)tmp == (uintptr_t)init);
            tmp->state = JV_STATE_READY;
            tmp->awaitable = 0;
            tmp->event = 0;
            break;
        }
        tmp = tmp->next;
    }

    // memset(ended->stack, 0, JV_STACK_SIZE);
    // mempool_put(&stack_pool, ended->stack);
    // mempool_put(&task_pool, ended);
    ended->state = JV_STATE_TERMINATED;

    curr = tmp;
    jv_task_switch(0, JV_NONE);
}

int jv_has_active(void)
{
    return (actives > 0);
}

int jv_init(void)
{
    arena_config_t aconf = ARENA_DEFAULT_CONFIG;
    aconf.reserve = ARENA_MB(64ULL);
    aconf.commit  = ARENA_MB(4ULL);
    if ( !(gmem = arena_new(&aconf)))
         return 0;

    mempool_init(&task_pool, gmem, sizeof(jv_task_t));
    mempool_init(&stack_pool, gmem, JV_STACK_SIZE);

    actives = 1;
    init = curr = mempool_get(&task_pool);
    curr->state = JV_STATE_READY;
    tail = curr;
    tail->prev = tail->next = curr;
    return 1;
}

jv_tid_t jv_spawn(void *fn, void *args[JV_ARGS_LIMIT])
{
    jv_task_t *new = mempool_get(&task_pool);
    new->stack = mempool_get(&stack_pool);

    new->regs[JV_R_RIP] = (uintptr_t) fn;
    if (args) {
        new->first_run = 1;
        memmove(new->args, args, sizeof(void*) * JV_ARGS_LIMIT);
    }

    uintptr_t *rsp = (uintptr_t*) &new->stack[JV_STACK_SIZE];
    // store the return address for coroutine's `ret` instruction
    *(--rsp) = (uintptr_t) jv_task_end;
    new->regs[JV_R_RSP] = (uintptr_t) rsp;
    new->state = JV_STATE_READY;

    new->prev = tail;
    new->next = tail->next;

    tail->next = new;
    tail = new;

    actives++;
    return (jv_tid_t)new;
}

void jv_end(void)
{
    arena_destroy(gmem);
}

jv_tid_t jv_self(void) {
    return (jv_tid_t)curr;
}
