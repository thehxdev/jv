#include <stdio.h>

#ifndef __linux__
    #error "At this point, only Linux is supported"
#endif

// helper macro to pass a non-pointer argument
#define _(v) ((void*)(v))

void counter(int n) {
	int i;
	for (i = n; i < (n + 5); ++i) {
		printf("%d\n", i);
		jv_yield();
	}
	printf("counter %d end\n", n);
}

int main(void) {
	jv_init();

	// register coroutines
	jv_run(counter, jv_args( _(0)  ));
	jv_run(counter, jv_args( _(5)  ));
	jv_run(counter, jv_args( _(10) ));

	while (jv_has_active())
		jv_yield();

	jv_end();
	return 0;
}
