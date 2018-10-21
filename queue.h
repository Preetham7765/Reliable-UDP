// A C program to demonstrate linked list based implementation of queue 
// reference : geeksforgeeks
#include <stdlib.h> 
#include <stdio.h> 
#include <string.h>
#define SIZE 11 // size of packet payload

// A linked list (LL) node to store a queue entry 
struct QNode 
{ 
    char buffer[SIZE]; 
    struct QNode *next; 
}; 
  
// The queue, front stores the front node of LL and rear stores ths 
// last node of LL 
struct Queue 
{ 
    struct QNode *front, *rear; 
};

struct QNode* newNode(char *p);
struct Queue *createQueue();
void enQueue(struct Queue *q, char *p);
struct QNode *deQueue(struct Queue *q) ;