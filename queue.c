#include <stdio.h>
#include <stdlib.h>

#include "queue.h"

/*
 * functions for manipulating the queue
 * push
 * pop
 * show
 * delete
*/

void q_init(Queue *q)
{
	q->head = NULL;
	
	return;
}

void q_push(Queue *q, int value)
{
	if( q->head == NULL )
	{
		List *temp = malloc(sizeof(List));
		
		temp->data = value;
		temp->next = q->head;
		q->head = temp;
	}
	else
	{
		// while not at end of list
		// continue going
		List *cur = q->head;
		while( cur->next != NULL )
		{
			cur = cur->next;
		}
		
		// at end of queue
		List *temp = malloc(sizeof(List));
		
		temp->data = value;
		temp->next = NULL;
		cur->next = temp;
	}
	q->length++;
	
	return;
}

int q_pop(Queue *q)
{
	List *temp = q->head;
	int ret;
	
	if( temp )
	{
		ret = temp->data;
		q->head = temp->next;
		
		free(temp);
		temp = NULL;
	}
	else
	{
		fprintf(stderr, "Error: queue is empty\n");
		exit(-1);
	}
	q->length--;
	
	return ret;
}

void q_show(Queue *q)
{
	List *cur = q->head;
	
	while( cur != NULL )
	{
		printf(" %d -> ", cur->data);
		cur = cur->next;
	}
	printf("\n");
	
	return;
}

void q_delete(Queue *q)
{
	List *cur = q->head;
	
	while( cur )
	{
		// debug line
		//printf("deleting %d\n", cur->data);
		cur = cur->next;
		q_pop(q);
	}
}

