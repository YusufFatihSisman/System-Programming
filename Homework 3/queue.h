#ifndef QUEUE_H_INCLUDED
#define QUEUE_H_INCLUDED

typedef struct Node{
	char data;
	struct Node *next;
}Node;

typedef struct Queue{
	struct Node *front, *rear;
}Queue;

Node* newNode(char c);
Queue* createQueue();
void enQueue(Queue *q, char c);
void deQueue(Queue *q);
int isEmpty(Queue *q);
void empty(Queue *q);

#endif