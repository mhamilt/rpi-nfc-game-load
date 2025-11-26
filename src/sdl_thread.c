#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>  // for sleep

int shared_value = 0;       // variable shared between threads
pthread_mutex_t lock;       // mutex to protect access

void* thread_func(void* arg) {
    for (int i = 0; i < 5; i++) {
        pthread_mutex_lock(&lock);      // lock before modifying
        shared_value += 10;
        printf("[Thread] shared_value = %d\n", shared_value);
        pthread_mutex_unlock(&lock);    // unlock after modifying
        sleep(1); // simulate work
    }
    return NULL;
}

int main() {
    pthread_t thread;

    // Initialize mutex
    if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("Mutex init failed\n");
        return 1;
    }

    // Create thread
    if (pthread_create(&thread, NULL, thread_func, NULL) != 0) {
        printf("Thread creation failed\n");
        return 1;
    }

    // Main thread work
    for (int i = 0; i < 5; i++) {
        pthread_mutex_lock(&lock);      // lock before accessing
        printf("[Main] shared_value = %d\n", shared_value);
        shared_value += 1;              // modify shared variable
        pthread_mutex_unlock(&lock);    // unlock after accessing
        sleep(1);
    }

    // Wait for the thread to finish
    pthread_join(thread, NULL);

    pthread_mutex_destroy(&lock);

    printf("Final shared_value = %d\n", shared_value);
    return 0;
}
