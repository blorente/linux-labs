#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_CHARS_MSG 128

typedef enum {
	NORMAL_MSG,
	USERNAME_MSG,
	END_MSG
} message_type_t;

struct chat_message_t {
	char text[MAX_CHARS_MSG];
	message_type_t type;
};

static const int chat_message_size = sizeof(struct chat_message_t);

void print_usage( void ) {
	printf("Usage: ./chat <username> <send fifo> <recieve fifo>\n");
}

int recieve(char * recieve_fifo) {
	int fifo_fd;
	int bytes_read = 1;
	struct chat_message_t message;

	printf("Opening fifo %s...\n", recieve_fifo);
	fifo_fd = open(recieve_fifo, O_RDONLY);
	if (fifo_fd == -1) {
		perror("open()");
		return 1;
	}

	printf("Recieving data...\n");

	while((bytes_read = read(fifo_fd, &message, chat_message_size)) != 0) {
		if (bytes_read == -1) {
			perror("read()");
			return 1;
		}

		printf("%s\n", message.text);
	}

	printf("Connection ended...\n");
	close(fifo_fd);
	return 0;
}

int send(char * send_fifo) {
	int fifo_fd;
	int bytes_written;
	struct chat_message_t message;

	printf("Opening fifo %s...\n", send_fifo);
	fifo_fd = open(send_fifo, O_WRONLY);
	if (fifo_fd == -1) {
		perror("open()");
		return 1;
	}

	printf("Sending data...\n");

	while(1) {
		// Read from stdin
		read(0, message.text, MAX_CHARS_MSG);
		printf("Sending %s\n", message.text);
		bytes_written = write(fifo_fd, &message, chat_message_size);
		if (bytes_written == -1) {
			perror("write()");
			return 1;
		}		
	}

	close(fifo_fd);
	return 0;
}

int main(int argc, char **argv) {

	if (argc != 4) {
		print_usage();
		return 1;
	}

	/* For debugging, remove from here once pthread has gone in */

	if (strcmp(argv[1], "-r") == 0) {
		return recieve(argv[3]);
	} else if (strcmp(argv[1], "-s") == 0) {
		return send(argv[2]);
	}

	return 0;
}