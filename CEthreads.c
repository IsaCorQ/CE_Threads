#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <stdatomic.h>

#define STACK_SIZE (1024 * 1024)

// CETHREAD definition
typedef struct {
    pid_t pid;
    void* (*start_routine)(void*);
    void* arg;
    void* stack;
    void* return_value;
    atomic_int status;  // Thread status: 0 = running, 1 = finished
    int detached;       // Detach flag: 0 = joinable, 1 = detached
} cethread_t;

// Mutex implementation using atomic_flag
typedef struct {
    atomic_flag lock;
    atomic_int value;
} cemutex_t;

// Condition variable implementation using atomic_int
typedef struct {
    atomic_int waiting_threads;
} cecond_t;

// CETHREAD Functions
int cethread_create(cethread_t *thread, const void *attr,
                    void *(*start_routine) (void *), void *arg) {
    (void)attr;  // Ignore attr for this implementation

    thread->start_routine = start_routine;
    thread->arg = arg;
    thread->status = 0;
    thread->detached = 0;  // Initialize as joinable by default

    // Allocate aligned stack for the thread
    thread->stack = aligned_alloc(16, STACK_SIZE);
    if (!thread->stack) {
        perror("Failed to allocate stack");
        return -1;
    }

    // Create the thread using clone
    thread->pid = clone((int (*)(void *))start_routine, (char*)thread->stack + STACK_SIZE,
                        CLONE_VM | CLONE_FS | CLONE_FILES | SIGCHLD, arg);
    if (thread->pid == -1) {
        perror("clone failed");
        free(thread->stack);
        return -1;
    }
    return 0;
}

int cethread_join(cethread_t *thread, void **retval) {
    if (thread->detached) {
        return -1;  // Error: cannot join a detached thread
    }

    int status;
    waitpid(thread->pid, &status, 0);

    if (retval) {
        *retval = thread->return_value;
    }

    free(thread->stack);
    return 0;
}

int cethread_detach(cethread_t *thread) {
    if (thread->detached) {
        return -1;  // Thread already detached
    }

    thread->detached = 1;  // Mark the thread as detached

    // Optionally, check if the thread has already terminated and free resources
    if (atomic_load(&thread->status) == 1) {
        // If the thread has already finished, free resources now
        free(thread->stack);
    }

    return 0;
}

void cethread_exit(void* retval) {

    _exit(0);  // Terminate the thread
}

// Mutex Functions
void cemutex_init(cemutex_t *mutex) {
    atomic_flag_clear(&mutex->lock);
    atomic_init(&mutex->value, 0);
}

void cemutex_lock(cemutex_t *mutex) {
    while (atomic_flag_test_and_set(&mutex->lock)) {
        // Spin until we acquire the lock
    }
    atomic_store(&mutex->value, 1);
}

void cemutex_unlock(cemutex_t *mutex) {
    atomic_store(&mutex->value, 0);
    atomic_flag_clear(&mutex->lock);
}

void cemutex_destroy(cemutex_t *mutex) {
    // Nothing special to destroy in this simple implementation
}

// Condition Variable Functions
void cecond_init(cecond_t *cond) {
    atomic_init(&cond->waiting_threads, 0);
}

void cecond_wait(cecond_t *cond, cemutex_t *mutex) {
    cemutex_unlock(mutex);  // Release the mutex while waiting for condition
    while (atomic_load(&cond->waiting_threads) == 0) {
        // Spin-wait (no busy waiting, mutex is unlocked here)
        usleep(100);  // Small sleep to reduce CPU usage
    }
    cemutex_lock(mutex);  // Reacquire the mutex when done
}

void cecond_broadcast(cecond_t *cond) {
    atomic_store(&cond->waiting_threads, 1);  // Notify all waiting threads
}

void cecond_destroy(cecond_t *cond) {
    // Nothing special to destroy in this simple implementation
}
