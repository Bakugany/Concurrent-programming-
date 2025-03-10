#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <pthread.h>
#include <stdbool.h>
#include "../common/io.h"
#include "../common/sumset.h"
#include <assert.h>

#define STACK_MEMORY_SIZE 4096

typedef struct {
    int a;
    int b;
    bool processed;
    int8_t destroy;
} Frame;

typedef struct {
    Frame data[STACK_MEMORY_SIZE];
    int capacity;
    int top;
    int last_to_share;
} Stack;

void stack_init(Stack* stack) {
    stack->capacity = STACK_MEMORY_SIZE;
    stack->top = -1;
    stack->last_to_share = -1;
}

void push_frame(Stack* stack, int x, int y, bool processed, int destroy) {
    Frame frame = {
        .a = x,
        .b = y,
        .processed = processed,
        .destroy = destroy,
    };
    stack->data[++stack->top] = frame;
}

Frame* pop_frame(Stack* stack) {
    return &stack->data[stack->top--];
}

typedef struct {
    Sumset data[STACK_MEMORY_SIZE];
    bool manager[STACK_MEMORY_SIZE];
    int last;
} Memory;

void memory_init(Memory* memory) {
    memory->last = 0;
    for (size_t i = 0; i < STACK_MEMORY_SIZE; i++) {
        memory->manager[i] = 0;
    }
}

static int get_memory(Memory* memory) {
    int i = memory->last;
    while (memory->manager[i] > 0){
        i++;
    assert(i < STACK_MEMORY_SIZE);
    }
    memory->manager[i] = 1;
    memory->last = i + 1;
    return i;
}

static void free_memory(Memory* memory, int addr) {
    memory->manager[addr] = 0;
    if (addr < memory->last)
        memory->last = addr;
}

struct SharedThreadData {
    int num_threads;
    pthread_mutex_t mutex;
    pthread_cond_t wait_task;
    Solution best_solution;
    Sumset* A;
    Sumset* B;
    atomic_int threads_waiting;
    bool is_available_task;
};

static void initializeSharedThreadData(struct SharedThreadData* sharedThreadData, int num_threads, Sumset* A, Sumset* B) {
    sharedThreadData->num_threads = num_threads;
    pthread_mutex_init(&sharedThreadData->mutex, NULL);
    pthread_cond_init(&sharedThreadData->wait_task, NULL);
    sharedThreadData->A = A;
    sharedThreadData->B = B;
    atomic_init(&sharedThreadData->threads_waiting, 0);
    sharedThreadData->is_available_task = true;
}

static void destroySharedThreadData(struct SharedThreadData* sharedThreadData) {
    pthread_mutex_destroy(&sharedThreadData->mutex);
    pthread_cond_destroy(&sharedThreadData->wait_task);
}

static Solution best_solution;
static InputData input_data;

void* worker_thread(void* arg) {
    struct SharedThreadData* sharedThreadData = arg;
    Solution local_best_solution;
    solution_init(&local_best_solution);

    Memory local_memory;
    memory_init(&local_memory);
    Stack local_stack;
    stack_init(&local_stack);

    while(true) {
        pthread_mutex_lock(&sharedThreadData->mutex);
        atomic_fetch_add(&sharedThreadData->threads_waiting, 1);

        while (!sharedThreadData->is_available_task && atomic_load(&sharedThreadData->threads_waiting) != sharedThreadData->num_threads) {
            pthread_cond_wait(&sharedThreadData->wait_task, &sharedThreadData->mutex);
        }
        atomic_fetch_sub(&sharedThreadData->threads_waiting, 1);

        if (!sharedThreadData->is_available_task) {
            break;
        }

        sharedThreadData->is_available_task = false;
        int a_addr = get_memory(&local_memory);
        local_memory.data[a_addr] = *sharedThreadData->A;
        int b_addr = get_memory(&local_memory);
        local_memory.data[b_addr] = *sharedThreadData->B;
        pthread_mutex_unlock(&sharedThreadData->mutex);
        push_frame(&local_stack, a_addr, b_addr, 1, -1);

        while (local_stack.top != local_stack.last_to_share) {
            Frame* popped_frame = pop_frame(&local_stack);
            int x = popped_frame->a;
            int y = popped_frame->b;
            bool processed = popped_frame->processed;
            int destroy = popped_frame->destroy;

            if (processed == 0) {
                if (destroy == 0)
                    free_memory(&local_memory, x);
                else if (destroy == 1)
                    free_memory(&local_memory, y);
                continue;
            }

			if (local_stack.top - local_stack.last_to_share > input_data.d / 2 && atomic_load(&sharedThreadData->threads_waiting) > 0) {
                   pthread_mutex_lock(&sharedThreadData->mutex);
                     if (!sharedThreadData->is_available_task) {
                         do {
                             local_stack.last_to_share++;
                            } while (local_stack.data[local_stack.last_to_share].processed == 0);

                          if (local_stack.last_to_share == local_stack.top) {
                              local_stack.last_to_share --;
                          }
                          else {
                          sharedThreadData->A = &local_memory.data[local_stack.data[local_stack.last_to_share].a];
                          sharedThreadData->B = &local_memory.data[local_stack.data[local_stack.last_to_share].b];;
                          sharedThreadData->is_available_task = true;
                          pthread_cond_signal(&sharedThreadData->wait_task);
                          }
                     }
                   pthread_mutex_unlock(&sharedThreadData->mutex);

            }

            if (local_memory.data[x].sum > local_memory.data[y].sum)  {
                int tmp = x;
                x = y;
                y = tmp;
                destroy = 1 - destroy;
            }

            if (is_sumset_intersection_trivial(&local_memory.data[x], &local_memory.data[y])) {
                push_frame(&local_stack, x, y, 0, destroy);
                for (size_t i = local_memory.data[x].last; i <= input_data.d; ++i) {
                    if (!does_sumset_contain(&local_memory.data[y], i)) {
                        int x_with_i = get_memory(&local_memory);
                        sumset_add(&local_memory.data[x_with_i], &local_memory.data[x], i);
                        push_frame(&local_stack, x_with_i, y, 1, 0);
                    }
                }
            } else {
                if ((local_memory.data[x].sum == local_memory.data[y].sum)
                    && local_memory.data[y].sum > local_best_solution.sum
                    && get_sumset_intersection_size(&local_memory.data[x], &local_memory.data[y]) == 2) {
                        solution_build(&local_best_solution, &input_data, &local_memory.data[x], &local_memory.data[y]);
                }

                if (destroy == 0)
                    free_memory(&local_memory, x);
                else if (destroy == 1)
                    free_memory(&local_memory, y);
            }
        }
    }
        if (local_best_solution.sum > best_solution.sum) {
            best_solution = local_best_solution;
        }
        sharedThreadData->num_threads--;
        pthread_cond_signal(&sharedThreadData->wait_task);
        pthread_mutex_unlock(&sharedThreadData->mutex);
    return NULL;
}

static void solve(Sumset* const a, Sumset* const b) {
    int num_threads = input_data.t;
   	struct SharedThreadData sharedThreadData;
    initializeSharedThreadData(&sharedThreadData, num_threads, a, b);

    pthread_t threads[num_threads];
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, worker_thread, &sharedThreadData);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    destroySharedThreadData(&sharedThreadData);
}

int main() {
    input_data_read(&input_data);
    solution_init(&best_solution);

    solve(&input_data.a_start, &input_data.b_start);
    solution_print(&best_solution);

    return 0;
}