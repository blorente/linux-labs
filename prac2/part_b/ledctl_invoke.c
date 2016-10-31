#include <linux/errno.h>
#include <sys/syscall.h>
#include <linux/unistd.h>
#include <stdio.h>
#ifdef __i386__
#define __NR_LEDCTL 353
#else
#define __NR_LEDCTL 316
#endif

long ledctl( unsigned int mask) {
	return (long) syscall(__NR_LEDCTL, mask);
}

void print_usage( void ) {
	printf("Usage: ./ledctl <mask>, where mask is between 0x0-0x7\nExample: ./ledctl 0x5\n");
}

int main(int argc, char** argv) { 
	unsigned int mask;

	if (argc == 0) {
		print_usage();
		return 0;
	}

	if (sscanf(argv[1], "%x", &mask) != 1) {
		printf("Bad argument: %s\n", argv[1]);
		print_usage();
		return -1;
	}


	if (ledctl(mask) != 0) {
		perror("ERROR");
		return -1;
	}

	return 0;
}