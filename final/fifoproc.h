#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <asm-generic/uaccess.h>
#include <asm-generic/errno.h>
#include <linux/semaphore.h>
#include "cbuffer.h"

MODULE_LICENSE("GPL");

#define BUFFER_LENGTH 50
#define MAX_CHARS_KBUF 40

typedef struct {
	struct proc_dir_entry *proc_entry;
	struct semaphore sem_mutex; //mutex mtx
	struct semaphore sem_prod;
	struct semaphore sem_cons;
	int prod_count;
	int cons_count;
	int nr_prod_waiting;
	int nr_cons_waiting;
	cbuffer_t* cbuffer;
} fifo_data_t;