/*
	Queue library for pingpong-os
	Author: Diogo G. Garcia de Freitas
*/

#include <stdio.h>

#include "queue.h"

void queue_append(queue_t **queue, queue_t *elem)
{
	queue_t *first;												//these two local variables could be ommited
	queue_t *last;												//but they make the code easier to understand

	if (queue == NULL)											//trying to append to a queue that doesn't exist
	{
		printf("ERROR | queue_append() | Queue doesn't exist.\n");
	}
	else if (elem == NULL)										//trying to append an inexistent node
	{
		printf("ERROR | queue_append() | Node to be appended doesn't exist.\n");
	}
	else if (elem->next != NULL || elem->prev != NULL)			//trying to append a node from another queue
	{
		printf("ERROR | queue_append() | Node to be appended already belongs to a queue.\n");
	}
	else if (*queue == NULL)									//appending on empty queue
	{
		*queue = elem;
		(*queue)->next = *queue;
		(*queue)->prev = *queue;
	}
	else														//appending on non-empty queue
	{
		first = *queue;
		last = (*queue)->prev;
		
		last->next = elem;
		elem->prev = last;
		
		elem->next = first;
		first->prev = elem;
	}
}

queue_t *queue_remove(queue_t **queue, queue_t *elem)
{
	if (queue == NULL)									//trying to remove from a queue that doesn't exist
	{
		printf("ERROR | queue_remove() | Queue doesn't exist.\n");
		return NULL;
	}
	else if (*queue == NULL)							//trying to remove from an empty queue
	{
		printf("ERROR | queue_remove() | Queue is empty.\n");
		return NULL;
	}
	else if (queue_contains(queue, elem) == 0)			//trying to remove a node from another queue
	{
		printf("ERROR | queue_remove() | Node doesn't belong to this queue.\n");
		return NULL;
	}
	else 												//removing from queue
	{
		elem->prev->next = elem->next;					//manipulating next and prev pointers accordingly
		elem->next->prev = elem->prev;

		if ((*queue) == elem)							//if node to be removed is the first one
			*queue = elem->next;						//then update the first node
		
		elem->next = NULL;								//update next and prev from the removed node
		elem->prev = NULL;

		if ((*queue) == elem)							//if node to be removed is the last node in the queue
			*queue = NULL;								//then make it a empty queue

		return elem;
	}
}

int queue_size(queue_t *queue)
{
	int counter = 0;
	queue_t *first = queue;
	queue_t *current = queue;
	
	if (current == NULL)			//empty queue
		return 0;
	
	counter += 1;					//queue is not empty
	current = current->next;
	
	while (current != first)
	{
		current = current->next;
		counter++;
	}
	return counter;
}

void queue_print(char *name, queue_t *queue, void print_elem (void*) )
{
	queue_t *current = queue;
	queue_t *first = queue;

	printf("%s: [", name);

	if (current != NULL)
	{
		do									//iterates over queue nodes
		{
			print_elem(current);
			current = current->next;		//goes to next node
			printf(" ");
		}
		while (current != first);
	}
	
	printf("]\n");
}

int queue_contains(queue_t **queue, queue_t *elem)
{
	queue_t *current = *queue;
	queue_t *first = *queue;

	do									//iterates over queue nodes
	{
		if (current == elem)			//if 'current' node is 'elem'
			return 1;					//then 'elem' belongs to 'queue'
		else
			current = current->next;	//goes to next node
	}
	while (current != first);
	
	return 0;							//otherwise, 'elem' doesn't belong to 'queue'
}
