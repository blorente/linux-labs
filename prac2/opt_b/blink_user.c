#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#define MAX_PATH_LEN 80
#define MAX_MESSAGE_LEN 80
#define MAX_COMMAND_LEN 100
#define MAX_LINE_LEN 256

#define DRIVER_PATH "/dev/usb/blinkstick0"
#define RED 0x050000
#define GREEN 0x000500
#define BLUE 0x000005

#define REPO_CLEAN 0
#define REPO_STAGING 1
#define REPO_UNSAVED 2

char cwd[MAX_PATH_LEN];

void print_usage( void ) {
	printf("Usage: ./blink <led> <repo>\nExample: ./ledctl 0 /path/to/my_repo\n");
}

void compose_message(char *target, unsigned char led, unsigned int color) {
	sprintf(target, "%i:0x%x", led, color);
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
	int status = REPO_UNSAVED;
	FILE * statusfile;
	char line[MAX_LINE_LEN];
	
	fetch_repo_status(repo_to_watch);

	// Open .statusfile and read contents
	if (!(statusfile = fopen(".statusfile", "r"))) {
		perror("Cannot read statusfiles");
	}

	while (fgets(line, MAX_LINE_LEN, statusfile) != NULL) {
		if (strstr(line, "nothing to commit")) {
			status = REPO_CLEAN;
		} else if (strstr(line, "Changes ")) {
			status = REPO_STAGING;
		}
	}

	fclose(statusfile);
	return status;
}

int display_repo_status(unsigned char led, int status) {	
	char message[MAX_MESSAGE_LEN];
	unsigned int color;
	if (status == REPO_CLEAN) {
		color = GREEN;
	} else if (status == REPO_STAGING) {
		color = BLUE;
	} else {
		color = RED;
	}

	compose_message(message, led, color);

	if (write_to_stick(message, 10) != 0) {
		perror("write_to_stick() error");
		return -1;
	}

	return 0;
}

int main(int argc, char** argv) { 
	char repo_to_watch[MAX_PATH_LEN];
	int led_to_display;
	int repo_status;

	if (argc < 3) {
		print_usage();
		return 0;
	}

	if (sscanf(argv[1], "%i", &led_to_display) != 1) {
		printf("Bad argument: %s\n", argv[1]);
		print_usage();
		return -1;
	}

	if (sscanf(argv[2], "%s", repo_to_watch) != 1) {
		printf("Bad argument: %s\n", argv[2]);
		print_usage();
		return -1;
	}


	if (getcwd(cwd, sizeof(cwd)) == NULL) {    	
    	perror("getcwd() error");
    }

    while(1) {    	
		repo_status = get_repo_status(repo_to_watch);
		
		if (display_repo_status(led_to_display, repo_status) != 0) {
			return -1;
		}

		sleep(2);	
    }	

		
	return 0;
}