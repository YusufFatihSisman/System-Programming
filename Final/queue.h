#ifndef QUEUE_H_INCLUDED
#define QUEUE_H_INCLUDED

typedef struct Node{
	int data;
	struct Node *next;
}Node;

typedef struct Queue{
	struct Node *front, *rear;
}Queue;

Node* newNode(int c);
Queue* createQueue();
void enQueue(Queue *q, int c);
int deQueue(Queue *q);
int isEmpty(Queue *q);
void empty(Queue *q);

#endif