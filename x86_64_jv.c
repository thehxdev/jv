#define JV_STACK_SIZE 4096
#define JV_ARGS_LIMIT 6

#define jv_concat_(A, B) A##B
#define jv_concat(A, B) jv_concat_(A, B)
#define jv_static_assert(cond, id) \
	extern char jv_concat(id, __LINE__)[ ((cond)) ? 1 : -1 ]

// static assert for sizeof(void*)
jv_static_assert(sizeof(void*) == 8, assert_sizeof_pointer);

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

typedef struct jv_context {
	// this field MUST be the first in this struct
	uintptr_t regs[JV_R_COUNT];
	uintptr_t first_run;
	void *args[JV_ARGS_LIMIT];
	struct jv_context *next, *prev;
	unsigned char stack_end[JV_STACK_SIZE];
} jv_context_t;

static size_t actives     = 0;
static jv_context_t *curr = NULL;
static jv_context_t *tail = NULL;

extern void jv_yield(void);
extern void jv_context_store(jv_context_t *ctx, void *rsp, void *rip);
extern void jv_context_restore(jv_context_t *ctx);

void jv_context_switch(void *rsp, void *rip) {
	jv_context_store(curr, rsp, rip);
	curr = curr->next;
	jv_context_restore(curr);
}

void jv_context_end(void) {
	actives--;
	jv_context_t *ended = curr;
	ended->prev->next = ended->next;
	ended->next->prev = ended->prev;
	curr = ended->next;
	free(ended);
	jv_context_restore(curr);
}

int jv_has_active(void) {
	return (actives > 0);
}

int jv_init(void) {
	curr = calloc(1, sizeof(*curr));
	if (!curr)
		return 0;
	tail = curr;
	tail->prev = tail->next = curr;
	return 1;
}

void jv_run(void *fn, void *args[JV_ARGS_LIMIT]) {
	jv_context_t *new = calloc(1, sizeof(*new));
	new->regs[JV_R_RIP] = (uintptr_t) fn;
	if (args) {
		new->first_run = 1;
		memcpy(new->args, args, sizeof(void*) * JV_ARGS_LIMIT);
	}

	uintptr_t *rsp = (uintptr_t*) &new->stack_end[JV_STACK_SIZE];
	// store the return address for coroutine's `ret` instruction
	*(--rsp) = (uintptr_t) jv_context_end;
	new->regs[JV_R_RSP] = (uintptr_t) rsp;

	new->prev = tail;
	new->next = tail->next;

	tail->next = new;
	tail = new;

	actives++;
}

void jv_end(void) {
	free(curr);
}
