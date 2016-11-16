#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_PATH_LEN 80
#define MAX_MESSAGE_LEN 80
#define MAX_COMMAND_LEN 100
#define DRIVER_PATH "/dev/usb/blinkstick0"
#define RED 0x050000
#define GREEN 0x000500
#define ORANGE 0xAA5D31

#define REPO_CLEAN 0
#define REPO_STAGING 1
#define REPO_UNSAVED 2

char cwd[MAX_PATH_LEN];

void print_usage( void ) {
	printf("Usage: ./ledctl <seconds-to-wait>\nExample: ./ledctl 5\n");
}

void compose_message(char *target, unsigned int color) {
	sprintf(target, "%i:0x%x", 0, color);
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

void fetch_repo_status(char *repo_to_watch) {
	char command[MAX_COMMAND_LEN];
	chdir(repo_to_watch);
	sprintf(command, "git status > %s/.statusfile", cwd);
	system(command);
	chdir(cwd);
}

int get_repo_status(char *repo_to_watch) {
	int status = REPO_CLEAN;
	fetch_repo_status(repo_to_watch);

	// Open .statusfile and read contents
	
	return status;
}

int display_repo_status(int status) {	
	char message[MAX_MESSAGE_LEN];
	unsigned int color;
	if (status == REPO_CLEAN) {
		color = GREEN;
	} else if (status == REPO_STAGING) {
		color = ORANGE;
	} else {
		color = RED;
	}

	compose_message(message, color);

	if (write_to_stick(message, 10) != 0) {
		perror("write_to_stick() error");
		return -1;
	}

	return 0;
}

int main(int argc, char** argv) { 
	char repo_to_watch[MAX_PATH_LEN];
	int repo_status;

	if (argc < 2) {
		print_usage();
		return 0;
	}

	if (sscanf(argv[1], "%s", repo_to_watch) != 1) {
		printf("Bad argument: %s\n", argv[1]);
		print_usage();
		return -1;
	}

	if (getcwd(cwd, sizeof(cwd)) == NULL) {    	
    	perror("getcwd() error");
    }	

	repo_status = get_repo_status(repo_to_watch);
	
	if (display_repo_status(repo_status) != 0) {
		return -1;
	}	
		
	return 0;
}