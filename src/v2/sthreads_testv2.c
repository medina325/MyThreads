#include <stdlib.h>   // exit(), EXIT_FAILURE, EXIT_SUCCESS
#include <stdio.h>    // printf(), fprintf(), stdout, stderr, perror(), _IOLBF
#include <stdbool.h>  // true, false
#include <limits.h>   // INT_MAX

#include "sthreads.h" // init(), spawn(), yield(), done()



/*******************************************************************************
									 Functions to be used together with spawn()

		You may add your own functions or change these functions to your liking.
********************************************************************************/

/* Prints the sequence 0, 1, 2, .... INT_MAX over and over again.
 */
void numbers() 
{
	int n = 0;
	while (true) 
	{
		printf(" n = %d\n", n);
		
		n = (n + 1) % (INT_MAX);
		
		if (n > 3) 
			done();
		
		yield();
	}
}

/* Prints the sequence a, b, c, ..., z over and over again.
 */
void letters() {
	char c = 'a';

	while (true) {
			printf(" c = %c\n", c);
			if (c == 'a') done();
			yield();
			c = (c == 'z') ? 'a' : c + 1;
		}
}

/* Calculates the nth Fibonacci number using recursion.
 */
int fib(int n) {
	switch (n) {
	case 0:
		return 0;
	case 1:
		return 1;
	default:
		return fib(n-1) + fib(n-2);
	}
}

/* Print the Fibonacci number sequence over and over again.

	 https://en.wikipedia.org/wiki/Fibonacci_number

	 This is deliberately an unnecessary slow and CPU intensive
	 implementation where each number in the sequence is calculated recursively
	 from scratch.
*/

void fibonacci_slow() {
	int n = 0;
	int f;
	while (true) {
		f = fib(n);
		if (f < 0) {
			// Restart on overflow.
			n = 0;
		}
		printf(" fib_slow(%02d) = %d\n", n, fib(n));
		//join();
		n = (n + 1) % INT_MAX;
	}
}

/* Print the Fibonacci number sequence over and over again.

	 https://en.wikipedia.org/wiki/Fibonacci_number

	 This implementation is much faster than fibonacci().
*/
void fibonacci_fast() {

	int a = 0;
	int b = 1;
	int n = 0;
	int next = a + b;

	while(true) {
		printf(" fib_fast(%02d) = %d\n", n, a);
		//join();
		next = a + b;
		a = b;
		b = next;
		n++;
		if (a < 0) {
			// Restart on overflow.
			a = 0;
			b = 1;
			n = 0;
		}
	}
}

/* Prints the sequence of magic constants over and over again.

	 https://en.wikipedia.org/wiki/Magic_square
*/
void magic_numbers() {
	int n = 3;
	int m;
	while (true) {
		m = (n*(n*n+1)/2);
		if (m > 0) {
			printf(" magic(%d) = %d\n", n, m);
			n = (n+1) % INT_MAX;
		} else {
			// Start over when m overflows.
			n = 3;
		}
		yield();
	}
}

/*******************************************************************************
																		 main()

						Here you should add code to test the Simple Threads API.
********************************************************************************/


void main(){
	puts("\n==== Test program for the Simple Threads API ====\n");
	int tid[5];

	// Inicialização
	init(main);

	// cria as threads que vão executar uma das funções acima:
	
	//spawn(&magic_numbers, &(tid[0]));

	//spawn(&fibonacci_fast, &(tid[1]));

	spawn(&fibonacci_slow, &(tid[2]));

	spawn(&letters, &(tid[3]));

	spawn(&numbers, &(tid[4]));


}

/* casos de teste

	1 (para o quantum)
	spawn(&fibonacci_slow, &(tid[2]));

	spawn(&letters, &(tid[3]));

	spawn(&numbers, &(tid[4]));

	2

*/
