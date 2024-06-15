#ifndef GUEST_QUEUE_H
#define GUEST_QUEUE_H

#define MAX_SIZE 100 // Define the maximum size of the queue

// Define the structure for the queue
typedef struct {
    int data;
    // Add any additional fields if needed
} QueueItem;

typedef struct {
    QueueItem items[MAX_SIZE];
    int front, rear, size;
} Queue;

// Function declarations
Queue* createQueue();
int isEmpty(Queue* queue);
int isFull(Queue* queue);
void enqueue(Queue* queue, int data);
int dequeue(Queue* queue);
int peek(Queue* queue);
int size(Queue* queue);

#endif /* GUEST_QUEUE_H */