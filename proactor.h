#ifndef PROACTOR_H
#define PROACTOR_H

#include <pthread.h>
#include <stdbool.h>

// Task structure to hold task information
typedef struct Task {
    void (*function)(void*);
    void* arg;
} Task;

// Proactor structure
typedef struct Proactor {
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    Task** tasks;
    int size;
    int capacity;
    bool running;
} Proactor;

// Function prototypes
Proactor* proactor_create(int capacity);
void proactor_destroy(Proactor* proactor);
void proactor_submit_task(Proactor* proactor, void (*task)(void* arg), void* arg);
void proactor_stop(Proactor* proactor);

#endif