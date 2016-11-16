#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_PATH_LEN 100
#define MAX_MESSAGE_LEN 80
#define DRIVER_PATH "/dev/usb/blinkstick0"
#define RED 0x050000
#define GREEN 0x000500
#define ORANGE 0xAA5D31

void print_usage( void ) {
	printf("Usage: ./ledctl <seconds-to-wait>\nExample: ./ledctl 5\n");
}

void compose_message(char *target, unsigned char led, unsigned int color) {
	sprintf(message, "%i:0x%x", 0, RED);
}

int write_to_stick(char *message, int len) {
	int driver_file;

	driver_file = open(DRIVER_PATH, O_WRONLY);
	if (driver_file <= 0) {
		printf("Open operation failed\n");
		return -1;
	}
	
	if (write(driver_file, message, len) == 0) {
		printf("Write operation failed\n");
		return -1;
	}
	return 0;
}

int main(int argc, char** argv) { 
	char repo_to_watch[MAX_PATH_LEN];
	char message[MAX_MESSAGE_LEN];

	if (argc < 2) {
		print_usage();
		return 0;
	}

	if (sscanf(argv[1], "%s", repo_to_watch) != 1) {
		printf("Bad argument: %s\n", argv[1]);
		print_usage();
		return -1;
	}

	compose_message(message, 0, RED);

	if (write_to_stick(message, 10) != 0) {
		printf("Could not write to stick\n");
		return -1;
	}
		
	return 0;	
}