/*
	pingpong-os
	Author: Diogo G. Garcia de Freitas
*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>

#include "pingpong.h"
#include "datatypes.h"
#include "queue.h"

#define STACKSIZE 32768						//threads stack size
#define SUSPENDED 0
#define READY 1
#define FINISHED 2

//#define DEBUG

//global variables
int tid_counter;
unsigned int clock;
task_t MainTask;
task_t DispatcherTask;
task_t *CurrentTask;
task_t *PreviousTask;
task_t *ready_queue;
task_t *suspended_queue;
struct sigaction action;
struct itimerval timer;

//=========================GENERAL FUNCTIONS=========================

void pingpong_init()
{
	setvbuf(stdout, 0, _IONBF, 0);			//disable stdout buffer, used by printf

	tid_counter = 0;						//creates MainTask
	MainTask.tid = tid_counter;
	MainTask.state = READY;
	MainTask.static_prio = 0;
	MainTask.dynamic_prio = MainTask.static_prio;
	MainTask.quantum = 20;
	CurrentTask = &MainTask;				//sets MainTask as CurrentTask

	//append MainTask to ready_queue
	queue_append((queue_t **) &ready_queue, (queue_t *) &MainTask);

	task_create(&DispatcherTask, dispatcher_body, NULL);

	//timer setup
	action.sa_handler = tick_handler;
	sigemptyset (&action.sa_mask);
	action.sa_flags = 0;
	if (sigaction (SIGALRM, &action, 0) < 0)
	{
		perror("Error in sigaction: ");
		exit(1);
	}

	//adjust timer values
	timer.it_value.tv_usec = 1000;			//first trigger in usecs
	timer.it_value.tv_sec  = 0;				//first trigger in secs
	timer.it_interval.tv_usec = 1000;		//periodic triggers in usec
	timer.it_interval.tv_sec  = 0;			//periodic triggers in secs

	//sets ITIMER_REAL timer
	if (setitimer (ITIMER_REAL, &timer, 0) < 0)
	{
		perror("Error in setitimer: ");
		exit(1);
	}
}

//=========================TASK MANAGEMENT=========================

int task_create(task_t *task, void (*start_routine)(void *), void *arg)
{
	char *stack;

	getcontext(&(task->context));

	stack = malloc(STACKSIZE);
	if (stack)
	{
		task->context.uc_stack.ss_sp = stack;
		task->context.uc_stack.ss_size = STACKSIZE;
		task->context.uc_stack.ss_flags = 0;
		task->context.uc_link = 0;
	}
	else if (stack == NULL)
	{
		printf("ERROR | task_create() | memory could not allocated for stack");
		return -1;
	}
	else
	{
		perror("ERROR | task_create() | stack not created");
		exit(1);
	}
	
	makecontext(&(task->context), (void*)(*start_routine), 1, arg);		//potential crash when no args are passed (hardcoded)

	tid_counter++;
	task->tid = tid_counter;
	task->state = READY;
	task->static_prio = 0;
	task->dynamic_prio = task->static_prio;
	task->execution_time = systime();
	task->processor_time = 0;

	if (task != &DispatcherTask)
		queue_append((queue_t **) &ready_queue, (queue_t *) task);

	#ifdef DEBUG
	printf ("DEBUG | task_create() | created task %d\n", task->tid);
	#endif

	return task->tid;
}

void task_exit(int exitCode)
{
	#ifdef DEBUG
	printf ("DEBUG | task_exit() | exiting task %d\n", CurrentTask->tid);
	#endif

	CurrentTask->state = FINISHED;
	CurrentTask->exit_code = exitCode;

	CurrentTask->execution_time = systime() - CurrentTask->execution_time;		//total execution time = current time - time when execution started

	printf("Task %d exit: ", CurrentTask->tid);
	printf("execution time %d ms, ", CurrentTask->execution_time);
	printf("processor time %d ms, ", CurrentTask->processor_time);
	printf("%d activations\n", CurrentTask->activations);

	if (queue_size((queue_t*) suspended_queue) > 0)
		task_resume(suspended_queue);

	task_yield();
}

int task_switch(task_t *task)
{
	CurrentTask->end_time = systime();
	CurrentTask->processor_time = CurrentTask->processor_time + CurrentTask->end_time - CurrentTask->start_time;

	if (task == NULL)
	{
		printf("ERROR | task_switch() | task doesn't exist");
		return -1;
	}

	PreviousTask = CurrentTask;
	CurrentTask = task;

	swapcontext(&(PreviousTask->context), &(task->context));

	CurrentTask->start_time = systime();
	CurrentTask->activations++;
	#ifdef DEBUG
	printf ("DEBUG | task_switch() | switched context from task %d to task %d\n", task->tid, CurrentTask->tid);
	#endif

	return 0;
}

int task_id()
{
	#ifdef DEBUG
	printf ("DEBUG | task_id() | tid is %d\n", CurrentTask->tid);
	#endif
	return CurrentTask->tid;
}

void task_suspend (task_t *task, task_t **queue)
{
	task_t *task_to_suspend;

	if (task == NULL)
		task_to_suspend = CurrentTask;
	else
		task_to_suspend = task;

	if (queue != NULL)
	{
		if (task_to_suspend->state == READY && task_to_suspend != CurrentTask)
		{
			queue_remove((queue_t**) &ready_queue, (queue_t*) task_to_suspend);
		}

		task_to_suspend->state = SUSPENDED;
		queue_append((queue_t **) queue, (queue_t *) task_to_suspend);
	}
}

void task_resume (task_t *task)
{
	if (task->state == SUSPENDED)
	{
		queue_remove((queue_t **) &suspended_queue, (queue_t*) task);
	}

	task->state = READY;
	queue_append((queue_t **) &ready_queue, (queue_t *) task);
}

void task_yield()
{
	#ifdef DEBUG
	printf ("DEBUG | task_yield() | switching context back to the dispatcher\n");
	#endif
	task_switch(&DispatcherTask);
}

void dispatcher_body()
{
	task_t *next;

	while (queue_size((queue_t*) ready_queue) > 0)
	{
		next = scheduler();													//gets next task from scheduler
		next->quantum = 20;													//a quantum time in ticks
		if (next)
		{
			if (next->state == READY)
				queue_remove((queue_t**) &ready_queue, (queue_t*) next);	//removes from front of queue

			task_switch(next); 												//switches to next task

			if (next->state == READY)
				queue_append((queue_t **) &ready_queue, (queue_t *) next);	//appends to the end of queue
		}
	}
	task_exit(0);															//finishes dispatcher
}

task_t* scheduler()
{
	task_t *current = ready_queue;
	task_t *first = ready_queue;
	task_t *biggest_priority = ready_queue;
	do																	//iterates over queue nodes
	{
		if (current->dynamic_prio < biggest_priority->dynamic_prio)		//if 'current' node is 'elem'
			biggest_priority = current;									//then 'elem' belongs to 'queue'
		current = current->next;										//goes to next node
	}
	while (current != first);

	biggest_priority->dynamic_prio = biggest_priority->static_prio;		//aging algorithm requirement
	
	if (queue_size ((queue_t*) ready_queue) > 0 && CurrentTask != &MainTask)
		task_aging(&biggest_priority);									//updates dynamic priority on other tasks

	return biggest_priority;
}

void task_setprio(task_t *task, int prio)
{
	if (prio < 20 && prio > -20)
	{
		if (task != NULL)
			task->static_prio = prio;
		else
			CurrentTask->static_prio = prio;
	}
	else
	{
		printf("ERROR | task_setprio() | invalid priority value");
	}
}

int task_getprio(task_t *task)
{
	if (task != NULL)
		return task->static_prio;
	else
		return CurrentTask->static_prio;
}

void task_aging(task_t *task)
{
	task_t *current = ready_queue;
	task_t *first = ready_queue;

	do									//iterates over queue nodes
	{
		if (current != task && current != &DispatcherTask)
			current->dynamic_prio--;	//updates priority on aged tasks

		current = current->next;		//goes to next node
	}
	while (current != first);
}

void tick_handler (int signum)
{
	clock++;							//Increments clock

	if (CurrentTask->tid != 1)			//if task isn't dispatcher (dispatcher tid = 1)
	{
		CurrentTask->quantum--;
	}

	if (CurrentTask->quantum == 0)		//if quantum is over, return to dispatcher
	{
		task_yield();
	}
}

unsigned int systime()
{
	return clock;
}

int task_join(task_t *task)
{
	if (task == NULL)
	{
		return -1;
	}

	if (task->state == FINISHED)
	{
		return task->exit_code;
	}
	else
	{
		task_suspend(NULL, &suspended_queue);
		task_yield();
		return task->exit_code;
	}
}

