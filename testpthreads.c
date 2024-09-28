#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

// Function that will be executed by each thread
void* thread_function(void* arg) {
    int thread_id = *(int*)arg;
    printf("Hello from thread %d\n", thread_id);
    return NULL;
}

int main() {
    pthread_t threads[5];
    int thread_ids[5];

    // Create 5 threads
    for (int i = 0; i < 5; i++) {
        thread_ids[i] = i + 1;
        if (pthread_create(&threads[i], NULL, thread_function, &thread_ids[i]) != 0) {
            fprintf(stderr, "Error creating thread %d\n", i + 1);
            return 1;
        }
    }

    // Wait for each thread to finish
    for (int i = 0; i < 5; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("All threads have finished execution\n");
    return 0;
}