#ifndef STHREADS_H
#define STHREADS_H

/* On Mac OS (aka OS X) the ucontext.h functions are deprecated and requires the
	 following define.
*/
#define _XOPEN_SOURCE 700

/* On Mac OS when compiling with gcc (clang) the -Wno-deprecated-declarations
	 flag must also be used to suppress compiler warnings.
*/

#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>

/* A thread can be in one of the following states. */
typedef enum {running, ready, waiting, terminated, noWaiting, indoEmpilhar} state_t;

/* Thread ID. */
typedef int tid_t;

typedef struct cel cel_t;
typedef struct head head_queue;

typedef struct thread {
	tid_t tid;
	state_t state;
	ucontext_t ctx;
} thread_t;


struct cel{
	thread_t t;
	cel_t *next;
};

struct head{
	cel_t *first, *last;
};




/* Data to manage a single thread should be kept in this structure. Here are a few
	 suggestions of data you may want in this structure but you may change this to
	 your own liking.
*/




/*******************************************************************************
															 Simple Threads API

You may add or change arguments to the functions in the API. You may also add
new functions to the API.
********************************************************************************/


/* Initialization

	 Initialization of global thread management data structures. A user program
	 must call this function exactly once before calling any other functions in
	 the Simple Threads API.

	 Returns 1 on success and a negative value on failure.
*/
int init(void (*main)());

/* Creates a new thread executing the start function.

	 start - a function with zero arguments returning void.

	 On success the positive thread ID of the new thread is returned. On failure a
	 negative value is returned. 
*/
void spawn(void (*start)(), int* tid);

/* Cooperative scheduling

	 If there are other threads in the ready state, a thread calling yield() will
	 trigger the thread scheduler to dispatch one of the threads in the ready
	 state and change the state of the calling thread from running to ready.
*/
void  yield();

/* Thread termination

	 A thread calling done() will change state from running to terminated. If
	 there are other threads waiting to join, these threads will change state from
	 waiting to ready. Next, the thread scheduler will dispatch one of the ready
	 threads.
*/
void  done();

/* Join with a terminated thread

	 A thread calling join() will be suspended and change state from running to
	 waiting and trigger the thread scheduler to dispatch one of the ready
	 threads. The suspended thread will change state from waiting to ready once another
	 thread calls done() and the join() should then return the thread id of the
	 terminated thread.
*/
tid_t join();

head_queue* init_queue(head_queue *queue);

int is_empty(head_queue *queue);

void enqueue(head_queue *queue, thread_t t);

thread_t dequeue(head_queue *queue);

void delete_queue(head_queue *queue);

void show_queue(head_queue *queue);

#endif
