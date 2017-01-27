#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/semaphore.h>
#include <linux/stat.h>
#include <asm-generic/uaccess.h>
#include <asm-generic/errno.h>

#include "cbuffer.h"

MODULE_LICENSE("GPL");
/* Based on code from Juan Carlos Saez */
MODULE_AUTHOR("Borja Lorente Escobar");

#define BUFFER_LENGTH 50
#define MAX_CHARS_KBUF 40
// Since we control the name length, it's unlikely to create a bigger name
#define FIFO_NAME_MAX 50
#define DEFAULT_FIFOS 1

typedef struct {
	struct proc_dir_entry *proc_entry;
	char name[FIFO_NAME_MAX];
	struct semaphore sem_mutex; //mutex mtx
	struct semaphore sem_prod;
	struct semaphore sem_cons;
	int prod_count;
	int cons_count;
	int nr_prod_waiting;
	int nr_cons_waiting;
	cbuffer_t* cbuffer;
} fifo_data_t;

static fifo_data_t *fifo_list;
static int fifo_num = DEFAULT_FIFOS;

module_param(fifo_num, int, 0000);

static int init_fifo(fifo_data_t *fifo, char *name);
static void cleanup_fifos_until(fifo_data_t *fifos, int num);