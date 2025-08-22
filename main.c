#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>

// helper macro to pass a non-pointer argument
#define _(v) ((void*)(v))

#define LISTEN_BACKLOG 128
#define BUFFER_SIZE 1024
#define async __attribute__((noinline))

#if 0
// This section is unfinished
static sig_atomic_t running = 1;

async void echo_routine(int clientfd)
{
    int status;
    ssize_t nread, nwritten;
    unsigned char buf[BUFFER_SIZE];

    while (1) {
        // Wait until `clientfd` becomes readable
        status = jv_await(clientfd, JV_READ);
        if (!status) {
            // handle IO error
            break;
        }
        nread = read(clientfd, buf, BUFFER_SIZE);
        if (nread <= 0) {
            perror("read");
            break;
        }

        // Wait until `clientfd` becomes writable
        status = jv_await(clientfd, JV_WRITE);
        if (!status) {
            // hanlde IO error
            break;
        }
        nwritten = write(clientfd, buf, nread);
        if (nwritten == -1 || nwritten != nread) {
            // handle write error
            break;
        }
    }
}

async void __main(void)
{
    long serverfd, clientfd, status;
    struct sockaddr_in serveraddr;

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port   = htons(8080);
    serveraddr.sin_addr.s_addr = INADDR_ANY;

    serverfd = socket(serveraddr.sin_family, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
    if (serverfd == -1) {
        perror("socket");
        return;
    }
    if (bind(serverfd, (struct sockaddr*) &serveraddr, sizeof(serveraddr)) == -1) {
        perror("bind");
        return;
    }
    if (listen(serverfd, LISTEN_BACKLOG) == -1) {
        perror("listen");
        return;
    }

    while (running) {
        // Wait until `serverfd` becomes readable (new connection arrived)
        status = jv_await(serverfd, JV_READ);
        if (!status) {
            // handle io error
            break;
        }
        clientfd = accept(serverfd, NULL, NULL);
        if (clientfd == -1) {
            perror("accept");
            break;
        }
        // spawn a new task to handle the new client
        jv_spawn(echo_routine, jv_args( _(clientfd) ));
    }

    close(serverfd);
}
#endif // end of unfinished section

#define ntask 2

async void dummy(void)
{
    int i = 3;
    while (i--) {
        puts("an async dummy function");
        jv_yield;
    }
}

async void counter(long id, long n)
{
    // call a new coroutine inside this coroutine
    jv_tid_t task = jv_spawn(dummy, NULL);
    while (n--) {
        printf("[Routine %d] %d\n", id, n);
        jv_yield;
    }
    jv_join(task);
}

int main(void)
{
    arena_t *arena;
    long i, count;

    // an array of task ids
    jv_tid_t tids[ntask];

    // create an arena allocator
    arena_config_t aconf = ARENA_DEFAULT_CONFIG;
    aconf.reserve = ARENA_MB(64ULL);
    aconf.commit  = ARENA_MB(16ULL);
    if ( !(arena = arena_new(&aconf)))
         return 1;

    jv_init(arena);

    // create tasks
    for (i = 0; i < ntask; ++i) {
        count = (i+1) * 5;
        tids[i] = jv_spawn(counter, jv_args( _(i), _(count) ));
    }

    // wait for them to finish
    for (i = 0; i < ntask; ++i)
        jv_join(tids[i]);

    // cleanup
    jv_end();
    arena_destroy(arena);
    return 0;
}

