// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DAINF UTFPR
// Versão 1.0 -- Março de 2015
//
// Estruturas de dados internas do sistema operacional

#ifndef __DATATYPES__
#define __DATATYPES__

#include <ucontext.h>

// Estrutura que define uma tarefa
typedef struct task_t
{
	struct task_t *prev, *next;				//para usar com a biblioteca de filas (cast)
	int tid;								//task id
	ucontext_t context;						//task context
	char state;								//task state; 1 means ready and 0 means not ready
	int static_prio;						//task static priority; ranges from -20 to 20
	int dynamic_prio;						//task dynamic priority; ranges from -20 to 20
	int quantum;
	unsigned int start_time;
	unsigned int end_time;
	unsigned int execution_time;
	unsigned int processor_time;
	int activations;
	int exit_code;
	char joined;
} task_t ;

// estrutura que define um semáforo
typedef struct
{
  // preencher quando necessário
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct
{
  // preencher quando necessário
} mqueue_t ;

#endif
