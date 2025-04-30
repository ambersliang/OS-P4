#include "lab.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

struct queue {
    void **buffer;              // Buffer to store data
    int capacity;               // Maximum capacity of the queue
    int size;                   // Current number of elements in the queue
    int front;                  // Index of the front element
    int rear;                   // Index of the rear element
    pthread_mutex_t mutex;      // Mutex for thread synchronization
    pthread_cond_t not_empty;   // Condition variable for signaling queue not empty
    pthread_cond_t not_full;    // Condition variable for signaling queue not full
    bool shutdown;              // Flag indicating queue shutdown
};

queue_t queue_init(int capacity) {
    queue_t q = (queue_t)malloc(sizeof(struct queue));

    // Fail handle 
    if (!q) {
        return NULL;
    }

    // Assign values to variables 
    q->capacity = capacity;
    q->size = 0;
    q->front = 0;
    q->rear = 0;
    q->buffer = (void **)malloc(capacity * sizeof(void *));   

    // Fail handle
    if (!q->buffer) {
        free(q); 
        return NULL;   
    }

    // Initialize synchronization 
    pthread_mutex_init(&q->mutex, NULL);           
    pthread_cond_init(&q->not_empty, NULL);        
    pthread_cond_init(&q->not_full, NULL);  

    q->shutdown = false;                          
    
    return q;   
}

void queue_destroy(queue_t q) {
    // Free buffer data 
    free(q->buffer);

    // Destroys the sychronization conditions 
    pthread_mutex_destroy(&q->mutex);   
    pthread_cond_destroy(&q->not_empty);    
    pthread_cond_destroy(&q->not_full);

    // Free the queue
    free(q);    
}

void enqueue(queue_t q, void *data) {
    pthread_mutex_lock(&q->mutex);      

    // If queue is shutdown, prevent enqueue
    if (q->shutdown) {
        pthread_mutex_unlock(&q->mutex);  
        return;
    }

    // Can only add data if queue is not full 
    while (q->size == q->capacity) {    
        pthread_cond_wait(&q->not_full, &q->mutex);
    }

    // Add data, and increment rear and size 
    q->buffer[q->rear] = data;          
    q->rear = (q->rear + 1) % q->capacity;   
    q->size++;                          

    // Signal for not empty queue 
    pthread_cond_signal(&q->not_empty); 
    pthread_mutex_unlock(&q->mutex);    
}

void* dequeue(queue_t q) {
    pthread_mutex_lock(&q->mutex);      

    // Wait until queue is not empty or shutting down to dequeue
    while (q->size == 0 && !q->shutdown) {   
        pthread_cond_wait(&q->not_empty, &q->mutex);
    }

    // If shutting down and is empty, return null
    if (q->shutdown && q->size == 0) {   
        pthread_mutex_unlock(&q->mutex);
        return NULL;
    }

    // Remove data, decrement rear and size
    void *data = q->buffer[q->front];   
    q->front = (q->front + 1) % q->capacity;   
    q->size--;                          

    // Signal for not full queue
    pthread_cond_signal(&q->not_full);  
    pthread_mutex_unlock(&q->mutex);    
    return data;                        
}

bool is_empty(queue_t q) {
    return (q->size == 0);              
}

bool is_full(queue_t q) {
    return (q->size == q->capacity);    
}

bool is_shutdown(queue_t q) {
    return q->shutdown;                 
}

void queue_shutdown(queue_t q) {
    pthread_mutex_lock(&q->mutex);

    // Signal shutting down   
    q->shutdown = true;

    // Let all threads waiting for not empty and not full know that queue is shutting down     
    pthread_cond_broadcast(&q->not_empty);  
    pthread_cond_broadcast(&q->not_full);

    pthread_mutex_unlock(&q->mutex);    
}
