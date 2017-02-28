#include <pthread.h>
#include <stdio.h>

pthread_t thread;

int i = 1;

void *fn(void *arg) {
//		int *i = (int *)arg;
//		*(int *)arg = 2;
//		printf("%d\n", *(int *)arg);
//		return ((void *)0);
		printf("%d\n", i);
		i = 2;
}

int main () {

//		int *i = 2;

//		printf("%d\n", *i);
		int err = pthread_create(&thread, NULL, &fn, NULL);

		pthread_join(thread, NULL);

		printf("i: %d\n", i);
		return 0;
}
