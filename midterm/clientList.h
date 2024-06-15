#ifndef LINKEDLIST_H
#define LINKEDLIST_H

// Define a structure for a node in the linked list
typedef struct Node {
    int pid;
    struct Node* next;
} Node;

// Function to add a new PID to the linked list
void addPid(Node** head, int pid);

// Function to remove a PID from the linked list
void removePid(Node** head, int pid);

// Function to print the linked list
void printList(Node* head);

// Function to calculate the size of the linked list
int sizeOfList(Node* head);

// Function to free the memory allocated to the linked list
void freeList(Node* head);

#endif /* LINKEDLIST_H */
