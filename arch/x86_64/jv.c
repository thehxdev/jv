#define JV_ARGS_LIMIT 6
#define JV_TASK_STACK_SIZE 4096
#define JV_STACK_SIZE JV_TASK_STACK_SIZE

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
    // just return to event-loop and continue other tasks
    JV_NONE,
    // wait for a file descriptor to become readable (not implemented)
    JV_READ,
    // wait for a file descriptor to become writable (not implemented)
    JV_WRITE,
    // wait for a task to terminate
    JV_TASK_TERMINATE
};

// task states
enum {
    // _NEW is used for the tasks that have arguments
    JV_STATE_NEW        = 1 << 0,
    JV_STATE_READY      = 1 << 1,
    JV_STATE_WAITING    = 1 << 2,
    JV_STATE_RUNNING    = 1 << 3
};
#define JV_TLIST_COUNT (JV_STATE_WAITING + 1)

// callee saved registers thet we store in
// each task's context
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
    // ABI specific fields. Order of these fields matter. Don't change them!
    uintptr_t regs[JV_R_COUNT];
    uintptr_t state;
    void *args[JV_ARGS_LIMIT];

    // non-ABI fields
    long wait_on, event;
    unsigned char *stack;
    struct jv_task *prev, *next, *parent;
} jv_task_t;

// a task list is just a double linked list
typedef struct jv_tlist {
    jv_task_t *head;
    jv_task_t *tail;
} jv_tlist_t;

// The task id is a unique identifier of a task. You MUST NEVER
// use/reuse the same task id after it's termination as JV itself
// may reuse the same id for new tasks.
typedef uintptr_t jv_tid_t;

// pool allocators for frequently used structures
static mempool_t stack_pool;
static mempool_t task_pool;

// global memory. managed by an arena provided by the user
static arena_t *gmem   = NULL;

// init is the task that called `jv_init`. could be the
// main function.
static jv_task_t *init = NULL;

// current task that is executing
static jv_task_t *curr = NULL;

// library's stack. used for function calls during task termination and
// task switch.
static unsigned char *stack = NULL;

// task lists. access each list by corresponding JV_STATE_* selector.
// only JV_STATE_READ and JV_STATE_WAITING are used.
static jv_tlist_t ls[JV_TLIST_COUNT];

// store and restore a task's context
extern void jv_task_store(jv_task_t *task, void *rsp, void *rip);
extern void jv_task_restore(jv_task_t *task);

// remove a task from a task list but from it's head
static jv_task_t *jv_tlist_pop_head(jv_tlist_t *q)
{
    jv_task_t *t = q->head;
    if (t) {
        q->head = t->next;
        if (t->next)
            t->next->prev = NULL;
        t->next = NULL;
    }
    return t;
}

// remove a task from a task list
static void jv_tlist_remove(jv_tlist_t *q, jv_task_t *t) {
    if (t->prev)
        t->prev->next = t->next;
    else
        q->head = t->next;

    if (t->next)
        t->next->prev = t->prev;
    else
        q->tail = t->prev;

    t->prev = t->next = NULL;
}

// push a task to a task list
static void jv_tlist_push(jv_tlist_t *q, jv_task_t *t)
{
    if (!q->head) {
        q->head = q->tail = t;
        t->next = t->prev = NULL;
        return;
    }
    t->next = NULL;
    t->prev = q->tail;
    q->tail->next = t;
    q->tail = t;
}

// change a task's state
static inline void jv_task_change_state(jv_task_t *t, int state)
{
    t->prev = t->next = NULL;
    t->state = state;
    jv_tlist_push(&ls[state], t);
}

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

    // prepare and do task switch
    __asm__ __volatile__ (
        "movq %0,    %%rsp\n"
        "movq %%r10, %%rdi\n"
        "movq %%r11, %%rsi\n"
        "jmp jv_task_switch"
        :
        : "m" (stack)
        : "rdi", "rsi");
}

// This is where the magic happens :)
void jv_task_switch(long awaitable, long event)
{
    long i;
    jv_task_t *tmp;

    if (event > 0) {
        // if there was an event that caller want's to subscribe to,
        // add the calling task to waiting list
        curr->wait_on = awaitable;
        curr->event = event;
        jv_task_change_state(curr, JV_STATE_WAITING);
    } else if (curr) {
        // if there was no events (task called jv_yield), just
        // add the task to ready list
        jv_task_change_state(curr, JV_STATE_READY);
    }

    // here we can do more complex stuff like poll/epoll/io_uring for
    // file descriptors that are registered by routines. then add the
    // ready ones to ready list. the OS specific way of doing async
    // IO can be abstracted.

    // get next ready to run task from ready list.
    curr = jv_tlist_pop_head(&ls[JV_STATE_READY]);
    if (curr == NULL) {
        // if there was no task to continue, fallback to initial
        // routine (the routine that called `jv_init`)
        curr = init;
    }

    // save library's stack pointer
    __asm__ __volatile__ ( "movq %%rsp, %0\n" : "=m" (stack) );

    jv_task_restore(curr);
}

// when a task returns (via `ret` instruction), it will return here
static void __attribute__((naked)) jv_task_end(void)
{
    // this is a wrapper around `jv_task_end_` function to
    // set rsp to library's stack
    __asm__ __volatile__ (
        "movq %0, %%rsp\n"
        "jmp  jv_task_end_\n"
        :
        : "m" (stack));
}

void jv_task_end_(void)
{
    jv_task_t *tmp;

    // do a naive linear search in waiting routines to see if one of them
    // was waiting for completion of this terminated task
    tmp = ls[JV_STATE_WAITING].head;
    while (tmp) {
        if (tmp->wait_on == (jv_tid_t)curr) {
            // if there was a task that was waiting for termination of current
            // task, remove it from waiting list and add it to ready list
            jv_tlist_remove(&ls[JV_STATE_WAITING], tmp);
            jv_task_change_state(tmp, JV_STATE_READY);
            tmp->wait_on = 0;
            tmp->event = 0;
            break;
        }
        tmp = tmp->next;
    }

    // release the resources used by the terminated task
    memset(curr->stack, 0, JV_TASK_STACK_SIZE);
    mempool_put(&stack_pool, curr->stack);
    mempool_put(&task_pool, curr);

    // do task switch
    curr = NULL;
    jv_task_switch(0, JV_NONE);
}

int jv_init(arena_t *arena)
{
    gmem = arena;
    stack = (unsigned char*) arena_alloc(gmem, JV_STACK_SIZE) + JV_STACK_SIZE - 1;

    memset(ls, 0, sizeof(ls));
    mempool_init(&task_pool, gmem, sizeof(jv_task_t));
    mempool_init(&stack_pool, gmem, JV_TASK_STACK_SIZE);

    init = curr = mempool_get(&task_pool);
    memset(curr, 0, sizeof(*curr));

    curr->state = JV_STATE_RUNNING;
    return (jv_tid_t)curr;
}

jv_tid_t jv_spawn(void *fn, void *args[JV_ARGS_LIMIT])
{
    jv_task_t *new = mempool_get(&task_pool);
    memset(new, 0, sizeof(*new));
    new->stack = mempool_get(&stack_pool);

    new->regs[JV_R_RIP] = (uintptr_t) fn;
    new->state = JV_STATE_READY;
    if (args) {
        new->state |= JV_STATE_NEW;
        memmove(new->args, args, sizeof(void*) * JV_ARGS_LIMIT);
    }
    jv_tlist_push(&ls[JV_STATE_READY], new);

    uintptr_t *rsp = (uintptr_t*) &new->stack[JV_TASK_STACK_SIZE];
    // store the return address for this routine's `ret` instruction
    *(--rsp) = (uintptr_t) jv_task_end;
    new->regs[JV_R_RSP] = (uintptr_t) rsp;

    new->parent = curr;
    return (jv_tid_t)new;
}

void jv_end(void)
{
}

jv_tid_t jv_self(void) {
    return (jv_tid_t)curr;
}
