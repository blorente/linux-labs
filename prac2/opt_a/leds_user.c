#include <linux/errno.h>
#include <sys/syscall.h>
#include <linux/unistd.h>
#include <stdio.h>

#ifdef __i386__
#define __NR_LEDCTL 353
#else
#define __NR_LEDCTL 316
#endif

#define BLINK_DONE 5

#define LOADING_STEP 200000
unsigned int seconds_to_wait;

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

int display_loading() {
	unsigned int mask = 1;
	unsigned int elapsed = 0;

	while (elapsed < seconds_to_wait) {
		CALL(ledctl, mask);
		mask = (mask == 4) ? 1 : (mask << 1);
		usleep(LOADING_STEP);
		elapsed += LOADING_STEP;
	}
	return 0;
}

int display_ready() {
	int i = 0;
	for (i = 0; i < BLINK_DONE; i++) {
		CALL(ledctl, 0);
		usleep(50000);
		CALL(ledctl, 7);
		usleep(50000);
	}
	return 0;
}

int main(int argc, char** argv) { 	

	if (argc < 2) {
		print_usage();
		return 0;
	}

	if (sscanf(argv[1], "%d", &seconds_to_wait) != 1) {
		printf("Bad argument: %s\n", argv[1]);
		print_usage();
		return -1;
	}

	seconds_to_wait *= 1000000;

	if(display_loading() != 0) {
		return -1;
	}
	
	if (display_ready() != 0) {
		return -1;
	}
		
	return 0;
	
}