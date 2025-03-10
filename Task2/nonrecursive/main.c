#include <stddef.h>
#include <stdbool.h>
#include "common/io.h"
#include "common/sumset.h"

#define STACK_SIZE 512

static InputData input_data;
static Solution best_solution;

typedef struct {
    Sumset current;
    Sumset* A;
    Sumset* B;
    bool processed;
} Frame;

typedef struct {
    Frame data[STACK_SIZE];
    Frame *top;
} Stack;

void stack_init(Stack* stack) {
    stack->top = stack->data - 1;
}

bool stack_is_empty(const Stack* stack) {
    return stack->top < stack->data;
}

Frame* stack_next(Stack* stack) {
    return stack->top + 1;
}

void stack_push(Stack* stack, Sumset* A, Sumset* B) {
    stack->top++;
    if ( A->sum > B->sum ) {
        stack->top->A = B;
        stack->top->B = A;
    } else {
        stack->top->A = A;
        stack->top->B = B;
    }
    stack->top->processed = false;
}

void stack_pop(Stack* stack) {
    do {
        stack->top--;
    } while (stack->top->processed);
}

static void solve(Sumset* const a, Sumset* const b) {
    Stack stack;
    stack_init(&stack);
    stack_push(&stack, a, b);

    while (!stack_is_empty(&stack)) {
        Frame* frame = stack.top;

		if (frame->processed) {
            stack_pop(&stack);
            continue;
        }

     	if (is_sumset_intersection_trivial(frame->A, frame->B)) {
            for (size_t i = frame->A->last; i <= input_data.d; ++i) {
                if (!does_sumset_contain(frame->B, i)) {
                    sumset_add(&stack_next(&stack)->current, frame->A, i);
                    stack_push(&stack, &stack_next(&stack)->current, frame->B);
                }
            }
            frame->processed = true;
        } else {
            if ((frame->A->sum == frame->B->sum)
            	&& (frame->B->sum > best_solution.sum)
           		&& (get_sumset_intersection_size(frame->A, frame->B) == 2) ) {
                	solution_build(&best_solution, &input_data, frame->A, frame->B);
        	}
            stack_pop(&stack);
        }
    }
}

int main() {
    input_data_read(&input_data);
    solution_init(&best_solution);

    solve(&input_data.a_start, &input_data.b_start);
    solution_print(&best_solution);

    return 0;
}