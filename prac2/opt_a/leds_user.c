#include <linux/errno.h>
#include <sys/syscall.h>
#include <linux/unistd.h>
#include <stdio.h>
//#include <pthread.h> /* for pthread_create(), pthread_cancel() */

#ifdef __i386__
#define __NR_LEDCTL 353
#else
#define __NR_LEDCTL 316
#endif

#define BLINK_DONE 300

long ledctl( unsigned int mask ) {
	//printf("Ledctl call: %d", mask);
	return (long) syscall(__NR_LEDCTL, mask);
}

#define CALL(fun, mask) \
	if (fun(mask) != 0) { \
		perror("ERROR"); \
		return -1; \
	}

void print_usage( void ) {
	printf("Usage: ./ledctl <seconds-to-wait>\nExample: ./ledctl 5\n");
}

// This function only reads the value of loading
// and is a pure function,
// so there are no synchronization issues.
int display_loading() {
	unsigned int mask = 1;

	while (1) {
		CALL(ledctl, 1);
		sleep(300);
		mask = (mask == 7) ? 1 : (mask << 1);
	}
	return 0;
}

int display_ready() {
	int i = 0;
	for (i = 0; i < BLINK_DONE; i++) {
		CALL(ledctl, 1);
		sleep(0.5);
		CALL(ledctl, 7);
		sleep(0.5);
	}
	return 0;
}

int main(int argc, char** argv) { 
	unsigned int seconds_to_wait;

	if (argc < 2) {
		print_usage();
		return 0;
	}

	if (sscanf(argv[1], "%d", &seconds_to_wait) != 1) {
		printf("Bad argument: %s\n", argv[1]);
		print_usage();
		return -1;
	}

	//display_loading();

	/*
	while (1) {
		ledctl(1);
		sleep(300);
		//mask = (mask == 7) ? 1 : (mask << 1);
	}
	*/
	if (display_ready() != 0) {
		return -1;
	}
		
	return 0;
	
}