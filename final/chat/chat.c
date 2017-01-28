#include "chat.h"

void print_usage( void ) {
	printf("Usage: ./chat <username> <send fifo> <recieve fifo>\n");
}

int recieve(char * recieve_fifo) {
	int fifo_fd;
	int bytes_read = 1;
	char username[MAX_CHARS_MSG];
	struct chat_message_t message;

	printf("Opening fifo %s...\n", recieve_fifo);
	fifo_fd = open(recieve_fifo, O_RDONLY);
	if (fifo_fd == -1) {
		perror("open()");
		return 1;
	}

	printf("Recieving data...\n");

	// First message recieved is the username
	bytes_read = read(fifo_fd, &message, chat_message_size);
	if (bytes_read == -1) {
		perror("read()");
		return 1;
	}

	if (message.type == USERNAME_MSG) {
		strcpy(username, message.text);
		printf("Connecting with user %s\n", username);
	} else {
		printf("First message was not a username\n");
		printf("Ending connection...\n");
		close(fifo_fd);
		return 1;
	}

	while(message.type != END_MSG) {
		bytes_read = read(fifo_fd, &message, chat_message_size);
		if (bytes_read == -1) {
			perror("read()");
			return 1;
		}

		if (message.type == NORMAL_MSG) {
			printf("%s says: %s", username, message.text);
		}
	}

	printf("Connection ended...\n");
	close(fifo_fd);
	return 0;
}

void *recieve_entry( void * data) {
	long int retval = recieve((char *) data);
	pthread_exit((void *)retval);
}


int send(char * username, char * send_fifo) {
	int fifo_fd;
	int bytes_written;
	int bytes_read;
	struct chat_message_t message;

	printf("Opening fifo %s...\n", send_fifo);
	fifo_fd = open(send_fifo, O_WRONLY);
	if (fifo_fd == -1) {
		perror("open()");
		return 1;
	}

	printf("Sending data...\n");

	// First message is the username
	message.type = USERNAME_MSG;
	strcpy(message.text, username);
	bytes_written = write(fifo_fd, &message, chat_message_size);
	if (bytes_written == -1) {
		perror("write()");
		return 1;
	}

	message.type = NORMAL_MSG;
	while((bytes_read = read(0, message.text, MAX_CHARS_MSG - 1)) != 0) {
		// Read from stdin		
		message.text[bytes_read] = '\0';
		bytes_written = write(fifo_fd, &message, chat_message_size);
		if (bytes_written == -1) {
			perror("write()");
			return 1;
		}		
	}

	// Send termination message
	printf("EOF recieved - Ending connection\n");

	message.type = END_MSG;
	memset(message.text, 0, MAX_CHARS_MSG);
	bytes_written = write(fifo_fd, &message, chat_message_size);
	if (bytes_written == -1) {
		perror("write()");
		return 1;
	}	

	close(fifo_fd);

	printf("Connection ended succesfully\n");
	return 0;
}

void *send_entry( void * send_info ) {
	long int retval;
	struct send_info_t *info = (struct send_info_t *) send_info;
	retval = send(info->username, info->send_fifo);
	pthread_exit((void *)retval);
}

int main(int argc, char **argv) {
	pthread_t recieve_thread, send_thread;
	struct send_info_t send_info;
	int retval;
	void *status;

	if (argc != 4) {
		print_usage();
		return 1;
	}

	/* For debugging, remove from here once pthread has gone in */
	if ((retval = pthread_create(&recieve_thread, NULL, recieve_entry, (void *)argv[3])) != 0) {
		printf("pthread_create() failed with code %d", retval);
		return 1;
	}

	send_info.username = argv[1];
	send_info.send_fifo = argv[2];
	// Only the main thread and the sender thread know about send_info,
	// so passing it's address is not dangerous
	if ((retval = pthread_create(&send_thread, NULL, send_entry, (void *)&send_info)) != 0) {
		printf("pthread_create() failed with code %d", retval);
		return 1;
	}

	if ((retval = pthread_join(recieve_thread, &status)) != 0) {
		printf("pthread_join() failed with code %d", retval);
		return 1;
	}
	printf("Receive thread completed, with code %ld\n", (long)status);

	if((retval = pthread_join(send_thread, &status)) != 0) {
		printf("pthread_join() failed with code %d", retval);
		return 1;
	}
	printf("Send thread completed, with code %ld\n", (long)status);

	pthread_exit(NULL);
}