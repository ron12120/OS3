#include "proactor.h"
#include <stdio.h>
#include <stdlib.h>

// Proactor main loop
void *proactor_run(void *arg)
{
    Proactor *proactor = (Proactor *)arg;
    while (proactor->running)
    {
        pthread_mutex_lock(&proactor->mutex);
       
        if (!proactor->running)
        {
            pthread_mutex_unlock(&proactor->mutex);
            break;
        }
        Task *task = proactor->task;
        pthread_mutex_unlock(&proactor->mutex);
        task->function(task->arg);
    }
    return NULL;
}
// Initialize the proactor
Proactor *proactor_create()
{
    Proactor *proactor = (Proactor *)malloc(sizeof(Proactor));
    if (!proactor)
    {
        perror("Error creating proactor");
        exit(EXIT_FAILURE);
    }

    proactor->task = (Task *)malloc(sizeof(Task ));
    if (!proactor->task)
    {
        perror("Error creating proactor tasks");
        free(proactor);
        exit(EXIT_FAILURE);
    }

    pthread_mutex_init(&proactor->mutex, NULL);
    pthread_cond_init(&proactor->cond, NULL);
    proactor->running = true;
    return proactor;
}

// Destroy the proactor
void proactor_destroy(Proactor *proactor)
{
    proactor_stop(proactor);
    pthread_join(proactor->thread, NULL);
    pthread_mutex_destroy(&proactor->mutex);
    pthread_cond_destroy(&proactor->cond);
    free(proactor->task);
    free(proactor);
}

// Submit a task to the proactor
void proactor_submit_task(Proactor *proactor, void (*task)(void *arg), void *arg)
{
    pthread_mutex_lock(&proactor->mutex);
    Task *new_task = (Task *)malloc(sizeof(Task));
    if (!new_task)
    {
        perror("Error creating task");
        exit(EXIT_FAILURE);
    }

    new_task->function = task;
    new_task->arg = arg;

    proactor->task = new_task;

    pthread_create(&proactor->thread, NULL, proactor_run, proactor);

    pthread_cond_signal(&proactor->cond);
    pthread_mutex_unlock(&proactor->mutex);
}

// Stop the proactor
void proactor_stop(Proactor *proactor)
{
    pthread_mutex_lock(&proactor->mutex);
    proactor->running = false;
    pthread_cond_broadcast(&proactor->cond);
    pthread_mutex_unlock(&proactor->mutex);
}
