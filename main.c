#include <stdio.h>

#ifndef __linux__
    #error "At this point, only Linux is supported"
#endif

// helper macro to pass a non-pointer argument
#define _(v) ((void*)(v))

static int data = 0;

void server(void) {
	volatile int i;
	for (i = 0; i < 10; ++i)
		jv_yield();
	data = i;
}

void reader(void) {
poll:
	jv_yield();
	if (data == 0) {
		puts("no data provided by server, yielding...");
		goto poll;
	}
	printf("got data: %d\n", data);
}

void counter(int n) {
	int i;
	for (i = 0; i < n; ++i) {
		printf("%d\n", i);
		jv_yield();
	}
	printf("counter %d end\n", n);
}

int main(void) {
	jv_init();

	// register coroutines
	jv_run(server, NULL);
	jv_run(reader, NULL);
	jv_run(counter, jv_args( _(10) ));

	while (jv_has_active())
		jv_yield();

	jv_end();
	return 0;
}
