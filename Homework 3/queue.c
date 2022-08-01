#include<stdio.h>
#include<stdlib.h>
#include "queue.h"

Node* newNode(char c){
    Node* temp = (Node*)malloc(sizeof(Node));
    temp->data = c;
    temp->next = NULL;
    return temp;
}

Queue* createQueue(){
    Queue* q = (Queue*)malloc(sizeof(Queue));
    q->front = q->rear = NULL;
    return q;
}

void enQueue(Queue* q, char c){
    Node* temp = newNode(c);
    if (q->rear == NULL) {
        q->front = q->rear = temp;
        return;
    }
    q->rear->next = temp;
    q->rear = temp;
}

void deQueue(Queue* q){
    if (q->front == NULL)
        return;

    Node* temp = q->front;
  
    q->front = q->front->next;

    if (q->front == NULL)
        q->rear = NULL;
  
    free(temp);
}

int isEmpty(Queue* q){
	return q->front == NULL;
}

void empty(Queue *q){
    while(q->front != NULL){
        deQueue(q);
    }
}

