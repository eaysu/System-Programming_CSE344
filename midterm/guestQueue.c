// queue.c

#include "guestQueue.h"
#include <stdlib.h>

// Initialize the queue
Queue* createQueue() {
    Queue* queue = (Queue*)malloc(sizeof(Queue));
    queue->front = 0;
    queue->rear = -1;
    queue->size = 0;
    return queue;
}

// Check if the queue is empty
int isEmpty(Queue* queue) {
    return queue->size == 0;
}

// Check if the queue is full
int isFull(Queue* queue) {
    return queue->size == MAX_SIZE;
}

// Add an element to the rear of the queue
void enqueue(Queue* queue, int data) {
    if (isFull(queue)) {
        // Handle full queue condition
        return;
    }
    queue->rear = (queue->rear + 1) % MAX_SIZE;
    queue->items[queue->rear].data = data;
    queue->size++;
}

// Remove and return an element from the front of the queue
int dequeue(Queue* queue) {
    if (isEmpty(queue)) {
        // Handle empty queue condition
        return -1; // Assuming -1 denotes an error condition
    }
    int data = queue->items[queue->front].data;
    queue->front = (queue->front + 1) % MAX_SIZE;
    queue->size--;
    return data;
}

// Get the element at the front of the queue without removing it
int peek(Queue* queue) {
    if (isEmpty(queue)) {
        // Handle empty queue condition
        return -1; // Assuming -1 denotes an error condition
    }
    return queue->items[queue->front].data;
}

// Get the current size of the queue
int size(Queue* queue) {
    return queue->size;
}
