
#ifndef CETHREADS_H
#define CETHREADS_H
#include <sched.h>
#include <stdatomic.h>

#define STACK_SIZE (1024 * 1024)
#define NUM_THREADS 5
#define INCREMENTS_PER_THREAD 100000

// Thread and mutex structures
typedef struct {
    atomic_flag lock;
} cemutex;

#define CETHREAD_MUTEX_INITIALIZER {ATOMIC_FLAG_INIT}

typedef struct {
    pid_t pid;
    void* (*start_routine)(void*);
    void* arg;
    void* stack;
    void* return_value;
    atomic_int status;
} cethread;

// Thread functions
int cethread_create(cethread *thread, const void *attr, void *(*start_routine) (void *), void *arg);
int cethread_join(cethread *thread, void **retval);

// Mutex functions
void cemutex_init(cemutex *mutex);
void cemutex_lock(cemutex *mutex);
void cemutex_unlock(cemutex *mutex);



#endif 
