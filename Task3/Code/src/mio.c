#include "mio.h"
#include "waker.h"
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>
#include "debug.h"
#include "executor.h"

#define MAX_EVENTS 64

struct Mio {
    int epoll_fd;
    Executor *executor;
};

Mio* mio_create(Executor *executor) {
    Mio *mio = (Mio *)malloc(sizeof(Mio));
    if (!mio) {
        perror("Failed to create Mio");
        exit(EXIT_FAILURE);
    }
    mio->epoll_fd = epoll_create1(0);
    if (mio->epoll_fd == -1) {
        perror("Failed to create epoll instance");
        free(mio);
        exit(EXIT_FAILURE);
    }
    mio->executor = executor;
    return mio;
}

void mio_destroy(Mio *mio) {
    close(mio->epoll_fd);
    free(mio);
}

int mio_register(Mio *mio, int fd, uint32_t events, Waker waker) {
    debug("Registering (in Mio = %p) fd = %d with", mio, fd);
    struct epoll_event event;
    event.events = events;
    event.data.ptr = waker.future;
    if (epoll_ctl(mio->epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1) {
        perror("Failed to register fd with epoll");
        return -1;
    }
    return 0;
}

int mio_unregister(Mio *mio, int fd) {
    debug("Unregistering (from Mio = %p) fd = %d\n", mio, fd);
    if (epoll_ctl(mio->epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
        perror("Failed to unregister fd with epoll");
        return -1;
    }
    return 0;
}

void mio_poll(Mio *mio) {
    debug("Mio (%p) polling\n", mio);
    struct epoll_event events[MAX_EVENTS];
    int n = epoll_wait(mio->epoll_fd, events, MAX_EVENTS, -1);
    if (n == -1) {
        perror("Failed to poll epoll");
        return;
    }
    for (int i = 0; i < n; i++) {
        Waker *waker = (Waker *)events[i].data.ptr;
        waker_wake(waker);
    }
}
