#ifndef _QUEUE_H
#define _QUEUE_H

/*
 * queue to hold the page entries
 * queue is FIFO
 * pop head, add at the end
 * used in replacing pages with FIFO
*/

typedef struct LIST
{
	int data;
	struct LIST *next;
}List;

typedef struct QUEUE
{
	List *head;
	int length;
}Queue;

void q_init(Queue *q);
void q_push(Queue *q, int value);
int q_pop(Queue *q);
void q_show(Queue *q);
void q_delete(Queue *q);

#endif

