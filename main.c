#include <stdio.h>

// helper macro to pass a non-pointer argument
#define _(v) ((void*)(v))

#define BUFFER_SIZE 1024
#define async __attribute__((noinline))

async void echo_routine(int clientfd) {
    int status;
    ssize_t nread, nwritten;
    unsigned char buf[BUFFER_SIZE];

    while (1) {
        status = jv_wait_io(clientfd, JV_READ);
        if (!status) {
            // handle IO error
            break;
        }
        nread = read(clientfd, buf, BUFFER_SIZE);
        if (nread <= 0) {
            perror("read");
            break;
        }

        status = jv_wait_io(clientfd, JV_WRITE);
        if (!status) {
            // hanlde IO error
            break;
        }
        nwritten = write(clientfd, buf, nread);
        if (nwritten == -1 || nwritten != nread) {
            // handle write error
            break;
        }

        jv_yield();
    }
}

async void __main(void) {
    int serverfd, clientfd, status;

    // socket, bind, listen, etc.

    while (running) {
        status = jv_wait_io(serverfd, JV_READ);
        if (!status) {
            // handle io error
            break;
        }
        clientfd = accept(serverfd, NULL, NULL);
        if (clientfd == -1) {
            perror("accept");
            break;
        }
        jv_run(echo_routine, jv_args( _(clientfd) ));
        jv_yield();
    }

    close(serverfd);
}

int main(void) {
    jv_init();

    // only return here if `jv_end` is called
    // or `__main` has returned
    jv_run_blocking(__main, NULL);

    jv_end();
    return 0;
}
