#include<stdio.h>
#include<stdlib.h>
#include "queue.h"

Node* newNode(int c){
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

void enQueue(Queue* q, int c){
    Node* temp = newNode(c);
    if (q->rear == NULL) {
        q->front = q->rear = temp;
        return;
    }
    q->rear->next = temp;
    q->rear = temp;
}

int deQueue(Queue* q){
    if (q->front == NULL)
        return -1;

    Node* temp = q->front;
    int returnVal = temp->data;
    q->front = q->front->next;

    if (q->front == NULL)
        q->rear = NULL;
  
    free(temp);
    return returnVal;
}

int isEmpty(Queue* q){
	return q->front == NULL;
}

void empty(Queue *q){
    while(q->front != NULL){
        deQueue(q);
    }
}

