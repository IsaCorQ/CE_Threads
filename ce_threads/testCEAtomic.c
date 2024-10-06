#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <stdatomic.h>

#define STACK_SIZE (1024 * 1024)
#define NUM_THREADS 5
#define INCREMENTS_PER_THREAD 100000

typedef struct {
    atomic_flag lock;
} cemutex_t;

#define CETHREAD_MUTEX_INITIALIZER {ATOMIC_FLAG_INIT}

typedef struct {
    pid_t pid;
    void* (*start_routine)(void*);
    void* arg;
    void* stack;
    void* return_value;
    atomic_int status;
} cethread_t;

int cethread_create(cethread_t *thread, const void *attr,
                    void *(*start_routine) (void *), void *arg) {
    (void)attr;  

    thread->start_routine = start_routine;
    thread->arg = arg;
    thread->status = 0;

    thread->stack = aligned_alloc(16, STACK_SIZE);
    if (!thread->stack) {
        perror("Failed to allocate stack");
        return -1;
    }

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
    int status;
    waitpid(thread->pid, &status, 0);
    
    if (retval) {
        *retval = thread->return_value;
    }
    
    free(thread->stack);
    return 0;
}


void cemutex_init(cemutex_t *mutex) {
    atomic_flag_clear(&mutex->lock);
}

void cemutex_lock(cemutex_t *mutex) {
    while (atomic_flag_test_and_set(&mutex->lock)) {
        
    }
}

void cemutex_unlock(cemutex_t *mutex) {
    atomic_flag_clear(&mutex->lock);
}

void cemutex_destroy(cemutex_t *mutex) {
   
}

int shared_counter = 0;
cemutex_t counter_mutex;

void *increment_counter(void *arg) {
    for (int i = 0; i < INCREMENTS_PER_THREAD; i++) {
        cemutex_lock(&counter_mutex);
        shared_counter++;
        cemutex_unlock(&counter_mutex);
    }
    return NULL;
}

int main() {
    cethread_t threads[NUM_THREADS];
    cemutex_init(&counter_mutex);  

    for (int i = 0; i < NUM_THREADS; i++) {
        if (cethread_create(&threads[i], NULL, increment_counter, NULL) != 0) {
            fprintf(stderr, "Error creating thread %d\n", i + 1);
            return 1;
        }
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        cethread_join(&threads[i], NULL);
    }

    cemutex_destroy(&counter_mutex);  

    printf("Final counter value: %d\n", shared_counter);
    return 0;
}
