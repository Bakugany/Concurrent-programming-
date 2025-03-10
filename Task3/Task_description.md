# Asynchronous Executor in C

## Motivation

Modern computer systems often need to handle many tasks simultaneously: managing numerous network connections, processing large amounts of data, or communicating with external devices. The traditional approach based on multithreading can lead to significant overhead in thread management, as well as scalability issues in systems with limited resources.

Asynchronous executors provide a modern solution to this problem. Instead of launching each task in a separate thread, the executor manages tasks cooperatively – tasks "yield" the processor when they are waiting for resources, such as data from a file, signals from a device, or a response from a database. This allows thousands or even millions of simultaneous operations to be handled using a limited number of threads.

## Use Cases

- **HTTP Servers**  
  Servers such as Nginx or Tokio in Rust use asynchronous executors to handle thousands of network connections while minimizing CPU and memory usage.

- **Embedded Systems Programming**  
  In resource-constrained systems (e.g., microcontrollers), effectively managing multiple tasks, such as reading sensor data or controlling devices, is crucial. Asynchronicity helps avoid the costly overhead of multithreading.

- **Interactive Applications**  
  In graphical or mobile applications, asynchronicity ensures smooth operation – for example, when downloading data from the network, the user interface remains responsive.

- **Data Processing**  
  The asynchronous approach efficiently manages numerous concurrent I/O operations, such as disk operations which may otherwise become a bottleneck in data analysis systems.

## Task Objective

For this homework assignment, you will write a simplified version of a single-threaded asynchronous executor in the C language (inspired by the Tokio library in Rust). In this assignment, no threads are created: both the executor and the tasks it executes all run in the main thread (or yield to others while waiting, e.g., for the ability to read).

You will receive an extensive project skeleton and will be tasked with implementing the key components. The result will be a simple but functional asynchronous library. Examples of its usage along with descriptions can be found in the source files in the `tests/` directory (which also serve as the basic, sample tests).

The key mechanism in this assignment is the use of **epoll**, which allows waiting for a selected set of events. This primarily involves listening for the availability of file descriptors (e.g., connections or network sockets) for reading and writing. See:

- _epoll in 3 easy steps_
- The `epoll(7)` man page. Specifically, we will only be using:
  - `epoll_create1(0)` (i.e., `epoll_create` without a suggested size and without special flags);
  - `epoll_ctl(...)` only for connection (pipe) file descriptors, only for events of type `EPOLLIN`/`EPOLLOUT` (ready for read/write), without special flags (in particular, in the default mode which is level-triggered, i.e., without the `EPOLLET` flag, not edge-triggered);
  - `epoll_wait(...)`;
  - `close()` to close the created `epoll_fd`.

## Specification

### Structures

The most important structures required for the implementation are:

- **Executor**:  
  Responsible for executing tasks on the processor – implements the main event loop `executor_run()` and contains a task queue (in general, it could, for example, contain multiple queues and threads for parallel processing).

- **Mio**:  
  Responsible for communication with the operating system – implements waiting for resources (e.g., data available for reading), i.e., calls `epoll_*` functions.

- **Future**:  
  In this homework, this is not just a placeholder for a value that can be waited for. The Future will also contain a coroutine, i.e., information about:
  - which task is to be executed (in the form of a pointer to the function `progress`),
  - any state that needs to be preserved between successive steps of task execution (if any).

  In C, there is no inheritance, so instead of subclassing the `Future` class, we will have structures like `FutureFoo` that contain `Future base` as the first field, and we will cast `Future*` to `FutureFoo*` accordingly.

- **Waker**:  
  This is a callback (here: a pointer to a function along with its arguments) that defines how to notify the executor that a task can proceed further (e.g., a task was waiting for data to be read and that data has become available).

In this homework, a project skeleton (header files and part of the structures) is provided, along with sample futures. The remaining work is to implement the executor, Mio, and other futures (details below).

### Operation Scheme

A task (future) may be in one of four states:

- **COMPLETED**: The task has finished and has a result.
- **FAILURE**: The task has finished with an error or was interrupted.
- **PENDING (queued)**: The task can proceed, i.e., it is either currently being executed by the executor or is waiting for CPU allocation in the executor queue.
- **PENDING (waker waiting)**: The task could not proceed further in the executor and someone (e.g., Mio or a helper thread) holds the Waker, which will be called when the situation changes.

The state diagram is as follows:

```
executor_spawn                     COMPLETED / FAILURE
     │                                       ▲
     ▼               executor calls           │
   PENDING  ───► fut->progress(fut, waker) ──+
(queued)                                      │
     ▲                                       │
     │                                       ▼
     └─── someone (e.g., mio_poll) calls ◄───PENDING
              waker_wake(waker)         (waker waiting)
```

(In the code, we treat PENDING as a single state without distinguishing who holds the pointer to the Future.)

More specifically, the operation scheme is as follows:

1. The program creates an executor (which, in turn, creates an instance of Mio).
2. Tasks are submitted to the executor (`executor_spawn`).
3. `executor_run` is called, which will execute tasks that may themselves spawn subtasks, continuing until the program ends.
4. In this homework, the executor does not create threads: `executor_run()` runs in the thread that calls it.

The executor in its loop processes tasks as follows:

- If there are no pending (PENDING) tasks left, it terminates the loop.
- If there are no active tasks, it calls `mio_poll()` to put the executor thread to sleep until something changes.
- For each active future task, it calls `future.progress(future, waker)` (creating a Waker that will re-enqueue the task if needed).

In the `progress` function of a Future:

- It tries to make progress on the task (e.g., reading more bytes from a connection or performing a computation step).
- In case of an error, it returns the FAILURE state.
- If the task is completed, it returns the COMPLETED state.
- Otherwise, it returns the PENDING state, ensuring that someone (e.g., Mio, a helper thread, or itself) will eventually call the waker.

Mio provides:

- In the `mio_register` function: registers that the given waker should be called when a specific event occurs; these events are readiness for reading or writing on a given file descriptor (for the purpose of this homework).
- In the `mio_poll` function: puts the calling thread to sleep until at least one of the registered events occurs and then calls the corresponding wakers.

### What `future.progress()` Can Do

Low-level tasks in their `progress()` function may call, for example:

- `mio_register(..., waker)` to be able to return PENDING and be woken up later by the executor (only when something becomes ready, then proceed to the next computation step);
- `waker_wake(waker)` to immediately request the executor to re-enqueue the task – this makes sense if we want to return PENDING and allow other tasks to run before proceeding to the next stage of some longer computation (analogous to `Thread.yield()`);
- `other_future->progress(other_future, waker)` to attempt executing a subtask within its own CPU time slice;
- `executor_spawn` to submit a subtask to the executor that may execute independently.
- (Not in this homework) `pthread_create` to perform computations in the background, in parallel with the executor.

### Future Combinators to Implement

The final part of this homework is implementing three combinators, i.e., Futures that combine other Futures:

- **ThenFuture**:  
  When executing a `ThenFuture` created from `fut1` and `fut2`, it should first execute `fut1`, and then `fut2`, passing the result of `fut1` as the input argument to `fut2`.

- **JoinFuture**:  
  It should execute `fut1` and `fut2` concurrently, finishing only when both have completed execution.

- **SelectFuture**:  
  It should execute `fut1` and `fut2` concurrently, finishing as soon as one of them successfully completes; for example, if `fut1` completes successfully (state COMPLETED), the second, `fut2`, should be abandoned, meaning that `fut2.progress()` will not be called further, even if its last state returned was PENDING.

The detailed interface and behavior (including error handling) are specified in `future_combinators.h`.
