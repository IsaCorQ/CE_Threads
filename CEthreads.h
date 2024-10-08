#ifndef CETHREADS_H
#define CETHREADS_H

#include <stdatomic.h>
#include <sys/types.h>

// Stack size for thread creation
#define STACK_SIZE (1024 * 1024)

// CETHREAD structure definition
typedef struct {
    pid_t pid;
    void* (*start_routine)(void*);
    void* arg;
    void* stack;
    void* return_value;
    atomic_int status;  // Thread status: 0 = running, 1 = finished
    int detached;       // Detach flag: 0 = joinable, 1 = detached
} cethread_t;

// Mutex structure definition using atomic_flag
typedef struct {
    atomic_flag lock;
    atomic_int value;
} cemutex_t;

// Condition variable structure using atomic_int
typedef struct {
    atomic_int waiting_threads;
} cecond_t;

// CETHREAD Functions
int cethread_create(cethread_t *thread, const void *attr,
                    void *(*start_routine) (void *), void *arg);
int cethread_join(cethread_t *thread, void **retval);
int cethread_detach(cethread_t *thread);
void cethread_exit(void* retval);

// Mutex Functions
void cemutex_init(cemutex_t *mutex);
void cemutex_lock(cemutex_t *mutex);
void cemutex_unlock(cemutex_t *mutex);
void cemutex_destroy(cemutex_t *mutex);

// Condition Variable Functions
void cecond_init(cecond_t *cond);
void cecond_wait(cecond_t *cond, cemutex_t *mutex);
void cecond_broadcast(cecond_t *cond);
void cecond_destroy(cecond_t *cond);

#endif // CETHREADS_H
