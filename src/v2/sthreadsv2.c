/* On Mac OS (aka OS X) the ucontext.h functions are deprecated and requires the
	 following define.
*/
#define _XOPEN_SOURCE 700

/* On Mac OS when compiling with gcc (clang) the -Wno-deprecated-declarations
	 flag must also be used to suppress compiler warnings.
*/

#include <signal.h>   /* SIGSTKSZ (default stack size), MINDIGSTKSZ (minimal
												 stack size) */
#include <stdio.h>    /* puts(), printf(), fprintf(), perror(), setvbuf(), _IOLBF,
												 stdout, stderr */
#include <stdlib.h>   /* exit(), EXIT_SUCCESS, EXIT_FAILURE, malloc(), free() */
#include <ucontext.h> /* ucontext_t, getcontext(), makecontext(),
												 setcontext(), swapcontext() */
#include <stdbool.h>  /* true, false */

#include <sys/time.h>
#include <signal.h>

#include "sthreads.h"

/* Stack size for each context. */
#define STACK_SIZE SIGSTKSZ*100

/******************************************************************************************************************************************
														Global data structures

											Add data structures to manage the threads here.
******************************************************************************************************************************************/
// Contador global que vai servir para gerar tid's
int cont_global = 1;

// tid da thread que chamou done (só é usado quando a fila de threads suspensas não está vazia)
int tid_done;

// Guarda contexto do escolanador
ucontext_t scheduler_ctx;

// Contexto da thread que foi suspensa por join().
ucontext_t suspende_thread_ctx;

// Fila global de threads prontas
head_queue *ready_queue = NULL; 

// Fila global de threads suspensas
head_queue *suspend_queue = NULL;

// Guarda contexto mais atual de uma thread, para que assim, quando esta voltar a ser executada, continue de onde parou.
ucontext_t atual_ctx;

/* Aponta para thread atual sendo executada. Serve, principalmente, para que o escalonador possa identificá-la pelo tid, pois tanto        
seu contexto, quanto seu estado estarão desatualizados no momento que esta estiver que ser enfileirada.
*/
thread_t *thread_atual;


/******************************************************************************************************************************************
														 Auxiliary functions

											Add internal helper functions here.
******************************************************************************************************************************************/
// As duas funções abaixo, implementam o timer, que basicamente força uma thread a parar sua execução para outra da fila de prontas executar.
// OBS: O QUANTUM ESTÁ CONFIGURADO PARA 200ms.
void timer_stop(int sig)
{
	printf("Thread Preemptada, devido ao tempo limite excedido.\n");
 	yield();
}
 
void inicializa_timer()
{
	struct itimerval it;
	struct sigaction act, oact;
	act.sa_handler = timer_stop;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	sigaction(SIGPROF, &act, &oact); 
	// Start itimer
	it.it_interval.tv_sec = 0;
	it.it_interval.tv_usec = 200000;
	it.it_value.tv_sec = 0;
	it.it_value.tv_usec = 200000;
	setitimer(ITIMER_PROF, &it, NULL);
}

// Usado em conjunto com o join().
void suspende_thread()
{
	// Mudando estado da thread atual para waiting, para scheduler não enfileirá-la na fila de prontos
	(*thread_atual).state = waiting;

	// Mudando contexto para contexto que está lá em join, pois quando esta thread for executada novamente, ela irá retornar o tid 
	// de quem chamou done
	(*thread_atual).ctx = atual_ctx; 
	thread_t thread_aux = *thread_atual;

	// Emfileira thread suspensa na fila de suspensos.
	enqueue(suspend_queue, thread_aux);

	// Muda contexto de execução para scheduler
	setcontext(&scheduler_ctx);
}

/* FCFS
1 - Enfileira thread que estava sendo executada que sofreu preempsão ou deu yield, muda seu estado para ready e atualiza seu contexto
(PARA O PONTO ONDE ELA PAROU)

2 - Desenfileira a thread a ser executada, muda seu estado para running, faz thread_atual apontar para ela e muda contexto de execução
 para esta thread
 
3 - Além disso, se a thread trocou o contexto para scheduler por que ela finalizou a execução, esta não será enfileirada novamente.
*/
void scheduler()
{	
	// Copia conteúdo de thread_atual para uma thread auxiliar 
	thread_t thread_aux = *thread_atual; 

	// Atualiza contexo da thread que estava sendo executada e muda seu estado para ready
	if(thread_aux.state != terminated && thread_aux.state != waiting)
	{
		thread_aux.ctx = atual_ctx;
		thread_aux.state = ready;

		// Enfileira thread
		enqueue(ready_queue, thread_aux);	
	}

	if( is_empty(ready_queue) == 0 ){ // Se a fila não estiver vazia, faça
		// Desenfileira thread a ser executada
		thread_aux = dequeue(ready_queue);
		thread_aux.state = running;
	
		thread_atual = &thread_aux;

		// Aciona o timer
		inicializa_timer();

		// Muda contexto de execução para thread
		setcontext(&(thread_atual->ctx));
	}
	else{
		printf("Não há threads na fila de prontos.\nEncerrando o programa.");
		exit(1);
	}
}

/* Única diferença entre createContext e createMainContext é que
	createContext é para contextos genéricos e createMainContext é
	usado para o contexto da main.
*/
void createContext(void (*func)(), ucontext_t *new_ctx)
{
	void *stack = malloc(STACK_SIZE);

  	if (stack == NULL) {
    	perror("Allocating stack");
    	exit(EXIT_FAILURE);
  	}

  	if (getcontext(new_ctx) < 0) { // Declarando estrutura para depois definir quem é generic_ctx
    	perror("getcontext");
    	exit(EXIT_FAILURE);
  	}

  	// Inicializando estrutura do contexto
  	new_ctx->uc_link           = &scheduler_ctx; // Para fazer com que a thread vá para o escalonador quando a função a ser executada
  														// por ela acabar naturalmente
  	new_ctx->uc_stack.ss_sp    = stack;
  	new_ctx->uc_stack.ss_size  = STACK_SIZE;
  	new_ctx->uc_stack.ss_flags = 0;
	
	// Agora sim generic_ctx recebe contexto de func
	makecontext(new_ctx, func, 0);
}

void createMainContext(void (*func)(), ucontext_t *main_ctx)
{
	void *stack = malloc(STACK_SIZE);

  	if (stack == NULL) {
    	perror("Allocating stack");
    	exit(EXIT_FAILURE);
  	}

  	if (getcontext(main_ctx) < 0) {
    	perror("getcontext");
    	exit(EXIT_FAILURE);
  	}

  	main_ctx->uc_link           = NULL; // Quando a thread principal acabar, ela encerra sua execução e de suas filhas.
  	main_ctx->uc_stack.ss_sp    = stack;
  	main_ctx->uc_stack.ss_size  = STACK_SIZE;
  	main_ctx->uc_stack.ss_flags = 0;
	
	makecontext(main_ctx, func, 0); // Define contexto
}

/******************************************************************************************************************************************
										Implementation of the Simple Threads API
******************************************************************************************************************************************/

int init(void (*main)())
{
	// Salvando contexto atual da main, para inicializar thread_atual 
	ucontext_t main_ctx;
	createMainContext(main, &main_ctx);

	// Definindo thread main atual (apenas para passar tid = 0 para a variável global thread_atual)
	thread_t main_thread;
	main_thread.tid = 0;
	main_thread.ctx = main_ctx;
	main_thread.state = running;

	// Definindo thread atual
	thread_atual = &main_thread;

	// Inicializando queue vazia
	ready_queue = init_queue(ready_queue); 
	suspend_queue = init_queue(suspend_queue);

	// Salvando contexto do scheduler e do empilhador de threads suspensas
	createContext(scheduler, &scheduler_ctx);
	createContext(suspende_thread, &suspende_thread_ctx);

	// Após acabar init , vai voltar para a main la em sthread_test.c automaticamente (claro né)
	
	return 1;
}


void spawn(void (*start)(), int *tid)
{
	// Salvando contexto da thread a ser criada
	ucontext_t new_ctx;
	createContext(start, &new_ctx);

	// Declarando nova thread
	thread_t generic_thread;

	generic_thread.tid = cont_global;
	cont_global++;

	generic_thread.ctx = new_ctx;
	generic_thread.state = ready;

	// Enfileirando nova thread:
	enqueue(ready_queue, generic_thread);
	
	// Passando tid da thread criada para main
	*tid = generic_thread.tid;

	// Salvando contexto atual e trocando para escalonador
	swapcontext(&atual_ctx, &scheduler_ctx);
}

void yield()
{
	printf("Thread Atual chamou yield\n");
	
	// Salvando contexto atual e trocando para escalonador
	swapcontext(&atual_ctx, &scheduler_ctx);
}

void done()
{
	printf("Thread Atual chamou done\n");

	// Verifica se há threads suspensas, se houver, então desempilha as threads, muda o estado para ready e a enfileira na fila de prontos
	// Por fim, salva tid
	while(is_empty(suspend_queue) == 0){
		thread_t thread_aux = dequeue(suspend_queue);
		thread_aux.state = ready;
		enqueue(ready_queue, thread_aux);

		tid_done = (*thread_atual).tid; 
	}
	
	// Mudando contexto da thread atual para terminated
	(*thread_atual).state = terminated;

	// Trocando contexto para escalonador
	setcontext(&scheduler_ctx);
}

tid_t join()
{
	printf("Thread Atual chamou join\n");
	
	// Vai para a função de suspender threads
	swapcontext(&atual_ctx, &suspende_thread_ctx);

	return tid_done;
}

/******************************************************************************************************************************************
															Funções da queue
******************************************************************************************************************************************/

head_queue* init_queue(head_queue *queue){
	queue = (head_queue*)malloc(sizeof(head_queue));

	queue->first = NULL;
	queue->last  = NULL;

	return queue;
}

int is_empty(head_queue *queue){
	if(queue->first == NULL && queue->last == NULL) return 1;

	else return 0;
}

void enqueue(head_queue *queue, thread_t t){
	cel_t *new;
	new = (cel_t*)malloc(sizeof(cel_t));

	if(new == NULL) exit(1);
	else{
		if(queue->first == NULL)
			queue->first = new;
		else
			queue->last->next = new;
	
		queue->last = new;

		new->t 		= t;
		new->next	= NULL;
	}
}

thread_t dequeue(head_queue *queue){
	cel_t *removed;

	removed = queue->first;
	queue->first = removed->next;

	if(removed == queue->last) queue->last = NULL;	

	thread_t removed_t = removed->t;

	free(removed);

	return removed_t;
}

void delete_queue(head_queue *queue){
	while(!is_empty(queue))
		dequeue(queue);
}

void show_queue(head_queue *queue){
	if(queue != NULL && queue->first != NULL){
		cel_t *show = queue->first;
		while(show != NULL){
			printf("| %3d ", show->t.tid);
			show = show->next;
		}
		printf("|\n");
	}
	else
		printf("ERR0 - cel_t de eventos vazia.\n");
}
