#include <stdio.h>

#ifndef __linux__
    #error "At this point, only Linux is supported"
#endif

// helper macro to pass a non-pointer argument
#define _(v) ((void*)(v))

#define BUFFER_SIZE 1024

void echo_routine(int clientfd) {
	int status;
	ssize_t nread, nwritten;
	unsigned char buf[BUFFER_SIZE];

	while (1) {
		status = jv_wait_io(clientfd, JV_READ);
		if (!status) {
			// handle IO error
		}
		nread = read(clientfd, buf, BUFFER_SIZE);
		if (nread <= 0)
			break;

		status = jv_wait_io(clientfd, JV_WRITE);
		if (!status) {
			// hanlde IO error
		}
		nwritten = write(clientfd, buf, nread);
		if (nwritten == -1 || nwritten != nread) {
			// handle write error
		}

		jv_yield();
	}
}

void __main(void) {
	int serverfd, clientfd, status;

	// socket, bind, listen, etc.

	while (running) {
		status = jv_wait_io(serverfd, JV_READ);
		if (!status) {
			// handle io error
		}
		clientfd = accept(serverfd, NULL, NULL);
		if (clientfd == -1) {
			perror("accept");
			return;
		}
		jv_run(echo_routine, jv_args( _(clientfd) ));
		jv_yield();
	}
}

int main(void) {
	jv_init();

	jv_run(__main, NULL);
	
	while (jv_has_active())
		jv_yield();

	jv_end();
	return 0;
}
