#define JV_STACK_SIZE 4096
#define JV_ARGS_LIMIT 6

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
	struct jv_context *next, *prev;
	unsigned char stack_end[JV_STACK_SIZE];
} jv_context_t;

static jv_context_t *curr = NULL;
static jv_context_t *tail = NULL;
static size_t actives     = 0;

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
	// for now ignore argument. can't find a solution for handling them
	(void)args;

	jv_context_t *new = calloc(1, sizeof(*new));
	new->regs[JV_R_RIP] = (uintptr_t) fn;

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
