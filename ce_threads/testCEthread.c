#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

#define STACK_SIZE 1024*1024  // 1 MB stack size for each thread

typedef struct {
    int thread_id;
    pid_t pid;
    int (*start_routine)(void*);
    void* arg;
} cethread;

int thread_wrapper(void* arg) {
    cethread* thread = (cethread*)arg;
    return thread->start_routine(thread->arg);
}

int cethread_create(cethread* thread, int (*function)(void*), void* arg) {
    thread->start_routine = function;
    thread->arg = arg;

    void* stack = aligned_alloc(16, STACK_SIZE);  // Ensuring 16-byte alignment for stack memory
    if (!stack) {
        perror("Failed to allocate stack");
        return -1;
    }

    thread->pid = clone(thread_wrapper, stack + STACK_SIZE, CLONE_VM | CLONE_FS | CLONE_FILES | SIGCHLD, thread);
    if (thread->pid == -1) {
        perror("clone failed");
        free(stack);
        return -1;
    }

    return 0;
}

void cethread_join(cethread* thread) {
    waitpid(thread->pid, NULL, 0);
}

int ce_thread_function(void* arg) {
    int thread_num = *(int*)arg;
    printf("Hello from my thread %d\n", thread_num);
    free(arg);  // Free memory for thread-specific data
    fflush(stdout);  // Ensure the output is printed in order
    return 0;
}

int main() {
    cethread threads[5];

    for (int i = 0; i < 5; i++) {
        int* thread_id = malloc(sizeof(int));
        if (thread_id == NULL) {
            fprintf(stderr, "Error allocating memory for thread ID\n");
            return 1;
        }
        *thread_id = i + 1;

        if (cethread_create(&threads[i], ce_thread_function, thread_id) != 0) {
            fprintf(stderr, "Error creating thread %d\n", i + 1);
            free(thread_id);
            return 1;
        }
    }

    for (int i = 0; i < 5; i++) {
        cethread_join(&threads[i]);
    }

    printf("Thread finished execution\n");

    return 0;
}
