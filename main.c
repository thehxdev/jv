#include <stdio.h>

#ifndef __linux__
    #error "At this point, only Linux is supported"
#endif

void counter3(void) {
	int i;
	for (i = 20; i < 30; ++i) {
		printf("%d\n", i);
		jv_yield();
	}
	printf("counter3 end\n");
}

void counter2(void) {
	int i;
	for (i = 10; i < 20; ++i) {
		printf("%d\n", i);
		jv_yield();
	}
	printf("counter2 end\n");
}

void counter1(void) {
	int i;
	for (i = 0; i < 10; ++i) {
		printf("%d\n", i);
		jv_yield();
	}
	printf("counter1 end\n");
}

int main(void) {
	jv_init();

	// register coroutines
	jv_run(counter1, NULL);
	jv_run(counter2, NULL);
	jv_run(counter3, NULL);

	while (jv_has_active())
		jv_yield();

	jv_end();
	return 0;
}
