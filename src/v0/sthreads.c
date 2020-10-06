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

/*******************************************************************************
														 Global data structures

								Add data structures to manage the threads here.
********************************************************************************/
// Contador global que vai servir para gerar tid's
int cont_global = 1;
ucontext_t new_ctx, scheduler_ctx, main_ctx, atual_ctx;
//ucontext_t ucp_blank;
// Fila de threads vai ser global
head_queue *queue = NULL; 

// Declarando main thread
thread_t main_thread;


// IDEIA: criar um ponteiro que aponta para thread sendo executada (para usar em scheduler)
// Assim, poderemos atualizar seu estado e contexto!
 thread_t *thread_atual;



/*******************************************************************************
														 Auxiliary functions

											Add internal helper functions here.
********************************************************************************/





 PARAMOS AQUUUUUEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE
/////////////////////////////////////////////////////////
int count = 0;
void timer_stop(int sig)
{

 	yield();
}
 
int main(void)
{
 struct itimerval it;
 struct sigaction act, oact;
 act.sa_handler = timer_stop;
 sigemptyset(&act.sa_mask);
 act.sa_flags = 0;

 sigaction(SIGPROF, &act, &oact); 
 // Start itimer
 it.it_interval.tv_sec = 4;
 it.it_interval.tv_usec = 50000;
 it.it_value.tv_sec = 1;
 it.it_value.tv_usec = 100000;
 setitimer(ITIMER_PROF, &it, NULL);
 for ( ; ; ) ;
}
//////////////////////////////////






// Vai olhar para a queue global e escolher próxima thread (setcontext)
void scheduler()
{

	// ESQUEMA FIFO
	// Enfileira thread que estava sendo executada que sofreu preempsão ou deu yield, muda seu estado para ready  e atualiza seu contexto

	// Desenfileira a thread a ser executada, muda seu estado para running, faz thread_atual apontar para ela
	// e aplica setcontext

	// Além disso, se a thread trocou o contexto para scheduler por que ela finalizou a execução, então deve-se mudar seu estado para
	// terminated e nao enfileirá-la

	// Copia conteúdo de thread_atual para uma thread auxiliar
	thread_t thread_aux = *thread_atual; 

	// Como contexto da thread que estava sendo executada antes do escalonador entrar em ação, está salvo em atual_ctx
	 // Atualiza contexo da thread que estava sendo executada e muda seu estado para ready
	if(thread_aux.state != terminated)
	{
		thread_aux.ctx = atual_ctx;
		thread_aux.state = ready;

		// Enfileira thread
		enqueue(queue, thread_aux);	
	}

	// Desenfileira thread a ser executada
	thread_aux = dequeue(queue);
	thread_aux.state = running;
	thread_atual = &thread_aux;
	// Muda contexto de execução para thread
	setcontext(&(thread_atual-> ctx));

	
}

// Única diferença entre createContext e createMainContext é o uc_link de cada contexto
ucontext_t createContext(void (*func)())
{
	// generic_ctx vai receber o contexto de func
	ucontext_t generic_ctx;

	void *stack = malloc(STACK_SIZE);

  	if (stack == NULL) {
    	perror("Allocating stack");
    	exit(EXIT_FAILURE);
  	}

  	if (getcontext(&generic_ctx) < 0) { // Declarando estrutura para depois definir quem é generic_ctx
    	perror("getcontext");
    	exit(EXIT_FAILURE);
  	}

  	// Inicializando estrutura do contexto
  	(&generic_ctx)->uc_link           = &scheduler_ctx; // Para fazer com que a thread vá para o escalonador quando a função a ser executada
  													// por ela acabar naturalmente
  	(&generic_ctx)->uc_stack.ss_sp    = stack;
  	(&generic_ctx)->uc_stack.ss_size  = STACK_SIZE;
  	(&generic_ctx)->uc_stack.ss_flags = 0;
	
	// Agora sim generic_ctx recebe contexto de func
	makecontext(&generic_ctx, func, 0);

	return generic_ctx;
}

void createMainContext(void (*func)())
{
	void *stack = malloc(STACK_SIZE);

  	if (stack == NULL) {
    	perror("Allocating stack");
    	exit(EXIT_FAILURE);
  	}

  	if (getcontext(&main_ctx) < 0) {
    	perror("getcontext");
    	exit(EXIT_FAILURE);
  	}

  	(&main_ctx)->uc_link           = NULL;
  	(&main_ctx)->uc_stack.ss_sp    = stack;
  	(&main_ctx)->uc_stack.ss_size  = STACK_SIZE;
  	(&main_ctx)->uc_stack.ss_flags = 0;
	
	makecontext(&main_ctx, func, 0);

}


/*******************************************************************************
										Implementation of the Simple Threads API
********************************************************************************/

// FALTA INICIALIZAR O TIMER
int init(void (*main)()){
	// Salvando contexto de scheduler
	scheduler_ctx = createContext(scheduler);

	// Salvando contexto atual da main
	createMainContext(main);

	main_thread.tid = 0;
	main_thread.ctx = main_ctx;
	main_thread.state = running;

	// Inicializando queue
	queue = init_queue(queue); 
	
	thread_atual = &main_thread;


	// Nao sei nao
	// Temos que enfileirar a thread da main:
	//enqueue(queue, main_thread);

	// Após acabar init , vai voltar para a main la em sthread_test.c automaticamente (claro né)
	
	return 1; // Fazer um if antes de retornar 1 (que diz q deu tudo certo)
}


void spawn(void (*start)(), int *tid){
	// Salvando contexto da thread a ser criada
	new_ctx = createContext(start);

	// Declarando nova thread
	thread_t generic_thread;

	generic_thread.tid = cont_global;
	cont_global++;

	generic_thread.ctx = new_ctx;
	generic_thread.state = ready;

	// Temos que enfileirar a nova thread:
	enqueue(queue, generic_thread);
	
	*tid = generic_thread.tid;
	// Após criar a thread e enfileirá-la, devemos mudar o contexto para o escalonador, para ser decidido quem vai
	// ser a próxima thread a ser executada
	getcontext(&atual_ctx); // só para inicializar a estrutura

	swapcontext(&atual_ctx, &scheduler_ctx);
}

void yield(){
	printf("Entrando no yield\n");
	// Passível de ser removido
	getcontext(&atual_ctx); // só para inicializar a estrutura
	// Armazena contexto atual e vai para escalonador
	swapcontext(&atual_ctx, &scheduler_ctx);
}

void done(){

	printf("Entrando no done\n");
	// Passível de ser removido

	getcontext(&atual_ctx); // só para inicializar a estrutura
	(*thread_atual).state = terminated;
	// Armazena contexto atual e vai para escalonador
	swapcontext(&atual_ctx, &scheduler_ctx);

}

tid_t join() {

	printf("Entrando no join\n");
	return -1;
}



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
//void *thread_manager(){
//	getcontext(&ucp_blank);
	
//}