#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

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

struct send_info_t {
	char *username;
	char *send_fifo;
};
