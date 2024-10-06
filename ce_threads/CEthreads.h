#ifndef CETHREADS_H
#define CETHREADS_H

#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdatomic.h>

// Define the cethread_t structure
typedef struct {
    pid_t pid;
    void* (*start_routine)(void*);
    void* arg;
    void* stack;
    void* return_value;
    atomic_int status; 
} cethread_t;

// Define the cemutex_t structure
typedef struct {
    atomic_flag lock;
    atomic_int value;
} cemutex_t;

// Define the cecond_t structure for condition variables
typedef struct {
    cemutex_t mutex;
    atomic_int wait_count;
} cecond_t;

// Function declarations
int cethread_create(cethread_t *thread, const void *attr, 
                    void *(*start_routine)(void *), void *arg);
int cethread_join(cethread_t *thread, void **retval);
void cethread_exit(void *retval);

int cemutex_init(cemutex_t *mutex, const void *attr);
int cemutex_lock(cemutex_t *mutex);
int cemutex_unlock(cemutex_t *mutex);
int cemutex_destroy(cemutex_t *mutex);

int cecond_init(cecond_t *cond, const void *attr);
int cecond_wait(cecond_t *cond, cemutex_t *mutex);
int cecond_signal(cecond_t *cond);
int cecond_broadcast(cecond_t *cond);
int cecond_destroy(cecond_t *cond);

#endif // CETHREADS_H