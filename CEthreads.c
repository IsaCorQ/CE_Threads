#define _GNU_SOURCE
#include "CEthreads.h"
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <stdatomic.h>

// Thread creation function
int cethread_create(cethread *thread, const void *attr, void *(*start_routine) (void *), void *arg) {
    (void)attr;

    thread->start_routine = start_routine;
    thread->arg = arg;
    thread->status = 0;

    thread->stack = aligned_alloc(16, STACK_SIZE);
    if (!thread->stack) {
        perror("Failed to allocate stack");
        return -1;
    }

    thread->pid = clone((int (*)(void *))start_routine, (char*)thread->stack + STACK_SIZE, CLONE_VM | CLONE_FS | CLONE_FILES | SIGCHLD, arg);
    if (thread->pid == -1) {
        perror("clone failed");
        free(thread->stack);
        return -1;
    }
    return 0;
}

// Thread join function
int cethread_join(cethread *thread, void **retval) {
    int status;
    waitpid(thread->pid, &status, 0);

    if (retval) {
        *retval = thread->return_value;
    }

    free(thread->stack);
    return 0;
}

// Mutex initialization function
void cemutex_init(cemutex *mutex) {
    atomic_flag_clear(&mutex->lock);
}

// Mutex locking function
void cemutex_lock(cemutex *mutex) {
    while (atomic_flag_test_and_set(&mutex->lock)) {
    }
}

// Mutex unlocking function
void cemutex_unlock(cemutex *mutex) {
    atomic_flag_clear(&mutex->lock);
}

// Shared counter and mutex
int shared_counter = 0;
cemutex counter_mutex;

// Function to increment the counter with mutex locking
void *increment_counter(void *arg) {
    for (int i = 0; i < INCREMENTS_PER_THREAD; i++) {
        cemutex_lock(&counter_mutex);
        shared_counter++;
        cemutex_unlock(&counter_mutex);
    }
    return NULL;
}

// Sample function for thread execution (from second file)
int ce_thread_function(void* arg) {
    int thread_num = *(int*)arg;
    printf("Hello from my thread %d\n", thread_num);
    free(arg);  // Free memory for thread-specific data
    fflush(stdout);
    return 0;
}

// Main function to demonstrate both thread types
int main() {
    cethread threads[NUM_THREADS];

    // Initialize the mutex
    cemutex_init(&counter_mutex);

    // Create and run threads for printing messages
    printf("Creating threads for message printing...\n");
    for (int i = 0; i < NUM_THREADS; i++) {
        int* thread_id = malloc(sizeof(int));
        if (!thread_id) {
            fprintf(stderr, "Error allocating memory for thread ID\n");
            return 1;
        }
        *thread_id = i + 1;

        if (cethread_create(&threads[i], NULL, (void* (*)(void*))ce_thread_function, thread_id) != 0) {
            fprintf(stderr, "Error creating thread %d\n", i + 1);
            free(thread_id);
            return 1;
        }
    }

    // Join the threads
    for (int i = 0; i < NUM_THREADS; i++) {
        cethread_join(&threads[i], NULL);
    }

    printf("All message threads finished execution\n");

    // Create and run threads for incrementing the counter
    printf("Creating threads for incrementing counter...\n");
    for (int i = 0; i < NUM_THREADS; i++) {
        if (cethread_create(&threads[i], NULL, increment_counter, NULL) != 0) {
            fprintf(stderr, "Error creating counter thread %d\n", i + 1);
            return 1;
        }
    }

    // Join the threads
    for (int i = 0; i < NUM_THREADS; i++) {
        cethread_join(&threads[i], NULL);
    }

    // Print the final value of the shared counter
    printf("Final value of shared_counter: %d\n", shared_counter);

    // Clean up the mutex
    return 0;
}
