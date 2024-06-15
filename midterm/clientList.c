#include <stdio.h>
#include <stdlib.h>
#include "clientList.h"

// Function to create a new node
Node* createNode(int pid) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL) {
        printf("Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    newNode->pid = pid;
    newNode->next = NULL;
    return newNode;
}

// Function to add a new PID to the linked list
void addPid(Node** head, int pid) {
    Node* newNode = createNode(pid);
    if (*head == NULL) {
        *head = newNode;
    } else {
        Node* temp = *head;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = newNode;
    }
}

// Function to remove a PID from the linked list
void removePid(Node** head, int pid) {
    if (*head == NULL) {
        printf("List is empty. Nothing to remove.\n");
        return;
    }

    Node* temp = *head;
    Node* prev = NULL;

    // If the node to be removed is the head node
    if (temp != NULL && temp->pid == pid) {
        *head = temp->next;
        free(temp);
        printf("PID %d removed from the list.\n", pid);
        return;
    }

    // Search for the node to be removed
    while (temp != NULL && temp->pid != pid) {
        prev = temp;
        temp = temp->next;
    }

    // If the node with the given PID is not found
    if (temp == NULL) {
        printf("PID %d not found in the list.\n", pid);
        return;
    }

    // Unlink the node from the linked list
    prev->next = temp->next;
    free(temp);
    printf("PID %d removed from the list.\n", pid);
}

// Function to print the linked list
void printList(Node* head) {
    printf("Linked List of PIDs: ");
    while (head != NULL) {
        printf("%d ", head->pid);
        head = head->next;
    }
    printf("\n");
}

// Function to calculate the size of the linked list
int sizeOfList(Node* head) {
    int count = 0;
    while (head != NULL) {
        count++;
        head = head->next;
    }
    return count;
}

// Function to free the memory allocated to the linked list
void freeList(Node* head) {
    Node* temp;
    while (head != NULL) {
        temp = head;
        head = head->next;
        free(temp);
    }
}
