#include "executor.h"
#include "future.h"
#include "mio.h"
#include "waker.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

struct Executor {
    Mio *mio;
    Future **queue;
    size_t max_queue_size;
    size_t current_queue_size;
    size_t pending_futures_count;
};

Executor* executor_create(size_t max_queue_size) {
    Executor *executor = (Executor *)malloc(sizeof(Executor));
    if (!executor) {
        perror("Failed to create executor");
        exit(EXIT_FAILURE);
    }
    executor->mio = mio_create(executor);
    executor->queue = (Future **)malloc(max_queue_size * sizeof(Future *));
    if (!executor->queue) {
        perror("Failed to create task queue");
        free(executor);
        exit(EXIT_FAILURE);
    }
    executor->max_queue_size = max_queue_size;
    executor->current_queue_size = 0;
    executor->pending_futures_count = 0;
    return executor;
}

void executor_spawn(Executor *executor, Future *fut) {
    if (executor->current_queue_size >= executor->max_queue_size) {
        fprintf(stderr, "Task queue is full\n");
        return;
    }
    executor->queue[executor->current_queue_size++] = fut;
    ++executor->pending_futures_count;
}

void executor_run(Executor *executor) {
    while (executor->pending_futures_count > 0) {
        for (size_t i = executor->current_queue_size; i > 0; --i) {
            Future *fut = executor->queue[i - 1];
            Waker waker = {executor, fut};
            FutureState state = fut->progress(fut, executor->mio, waker);
            executor->queue[i - 1] = executor->queue[--executor->current_queue_size];
            if (state != FUTURE_PENDING) {
                --executor->pending_futures_count;
            }
        }

        if (executor->current_queue_size == 0 && executor->pending_futures_count > 0) {
            mio_poll(executor->mio);
        }
    }
}

void executor_destroy(Executor *executor) {
    free(executor->queue);
    mio_destroy(executor->mio);
    free(executor);
}

void waker_wake(Waker *waker) {
    Executor *executor = (Executor *)waker->executor;
    if (executor->current_queue_size < executor->max_queue_size) {
        executor->queue[executor->current_queue_size++] = waker->future;
    } else {
        fprintf(stderr, "Task queue is full\n");
    }
}
