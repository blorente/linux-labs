#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/random.h>
#include <linux/vmalloc.h>
#include <linux/list.h>
#include <linux/smp.h>
#include <asm-generic/uaccess.h>
#include "cbuffer.h"

MODULE_LICENSE("GPL");
/* Based on code from Juan Carlos Saez */

#define BUFFER_LENGTH 24

/* Cbuffer for the Top Half */
static cbuffer_t* cbuffer;

/* Functions to manipulate the list */
struct list_item_t {
  unsigned int data;
  struct list_head storage_links;
} list_item_t;
struct list_head storage; /* Linked list to store numbers */
static int append_to_list(unsigned int elem);
static void remove_from_list(int elem);
static void clear_list( void );

/* Functions to schedule work */
struct timer_list my_timer;
struct work_struct flush_work_data;
static int select_cpu( void );
static void schedule_flush(int cpu_to_use);
static void flush_wq_function(struct work_struct *work);
static void fire_timer(unsigned long data);

/* Functions for modconfig proc entry */
#define MAX_KBUF_MODCONFIG 100
static struct proc_dir_entry *modconfig_proc_entry;
volatile int timer_period_ms = 1000;
volatile int emergency_threshold = 12;
volatile int max_random = 100;
static ssize_t modconfig_read(struct file *filp, char __user *buf, size_t len, loff_t *off);
static ssize_t modconfig_write(struct file *filp, const char __user *buf, size_t len, loff_t *off);
static const struct file_operations modconfig_entry_fops = {
    .read = modconfig_read,
    .write = modconfig_write,
};

int append_to_list(unsigned int elem) {
  struct list_item_t *new_item = (struct list_item_t *)vmalloc(sizeof(struct list_item_t));
  new_item->data = elem;
  list_add_tail(&new_item->storage_links, &storage);
  return 0;
}

void remove_from_list(int elem) {
  struct list_item_t *item = NULL;
  struct list_head *cur_node = NULL;
  struct list_head *aux_node = NULL;

  list_for_each_safe(cur_node, aux_node, &storage) {    
    item = list_entry(cur_node, struct list_item_t, storage_links);
    if (item->data == elem) {
      printk(KERN_INFO "Modtimer: Deleting node (%i)\n", item->data);
      list_del(cur_node);
      vfree(item);      
    }
  }
}

void clear_list() {
  struct list_item_t *item = NULL;
  struct list_head *cur_node = NULL;
  struct list_head *aux_node = NULL;

  list_for_each_safe(cur_node, aux_node, &storage) {
    printk(KERN_INFO "Modtimer: Deleting node\n");
    item = list_entry(cur_node, struct list_item_t, storage_links);
    list_del(cur_node);
    vfree(item);
  }
}

int select_cpu() {
    int current_cpu = smp_processor_id();
    return (current_cpu + 1) % 2;
}

void schedule_flush(int cpu_to_use) {
    printk(KERN_INFO "Modtimer: Schedule work on cpu %i\n", cpu_to_use);
    schedule_work_on(cpu_to_use, &flush_work_data);
}

void flush_wq_function(struct work_struct *work) {
    unsigned int num_to_insert = 0;
    /* Flush the buffer into the linked list */
    printk(KERN_INFO "Modtimer: Work started\n");
    while(size_cbuffer_t(cbuffer) >= sizeof(int)) {
        remove_items_cbuffer_t(cbuffer, (char *)&num_to_insert, sizeof(int));
        printk(KERN_INFO "Modtimer: Inserting elem %i into list\n", num_to_insert);
        append_to_list(num_to_insert);
    }
}

void fire_timer(unsigned long data) {
    /* Create random number to insert */
    unsigned int to_insert = get_random_int() % max_random;
    printk(KERN_INFO "Modtimer: int to insert [%i]\n", to_insert);

    /* Insert into buffer to be flushed into list */
    insert_items_cbuffer_t(cbuffer, (char *)&to_insert, 4);

    if (size_cbuffer_t(cbuffer) >= emergency_threshold) {
        /* Create deferred work in another CPU */        
        int cpu_to_use = select_cpu();
        /* Enqueue work */     
        schedule_flush(cpu_to_use);
    }

    /* Re-activate the timer timer_period_ms from now */
    mod_timer( &(my_timer), jiffies + timer_period_ms); 
}

ssize_t modconfig_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {
    char kbuf[MAX_KBUF_MODCONFIG];
    char *result;
    int nr_bytes = 0;

    if ((*off) > 0)
      return 0;

    /* Use result as a writing head */
    result = &kbuf[0];
    result += sprintf(result, "timer_period_ms=%i\n", timer_period_ms);
    result += sprintf(result, "emergency_threshold=%i\n", emergency_threshold);
    result += sprintf(result, "max_random=%i\n", max_random);

    printk(KERN_INFO "Modtimer: Read modconfig: %s\n", kbuf);

    nr_bytes = result - kbuf;

    if (len < nr_bytes)
        return -ENOSPC;
  
    if (copy_to_user(buf, &kbuf[0], nr_bytes))
        return -EINVAL;

    (*off) += len;

    return nr_bytes; 
}

ssize_t modconfig_write(struct file *filp, const char __user *buf, size_t len, loff_t *off) {
    char kbuf[MAX_KBUF_MODCONFIG];
    int value;

    if ((*off) > 0)
      return 0;

    if (len + 1 > MAX_KBUF_MODCONFIG) {
        printk(KERN_INFO "Modtimer: Modconfig Panic! (not enough space)\n");
        return -ENOSPC;
    }

    if (copy_from_user(&kbuf[0], buf, len)) {
        printk(KERN_INFO "Modtimer: Modconfig Panic! (failed copy_from_user)\n");
        return -EFAULT;
    }

    kbuf[len] = '\0'; // Add string terminator

    if (sscanf(kbuf, "timer_period_ms %i", &value) == 1) {
        timer_period_ms = value;
    } else if (sscanf(kbuf, "emergency_threshold %i", &value) == 1) {
        emergency_threshold = value;
    } else if (sscanf(kbuf, "max_random %i", &value) == 1) {
        max_random = value;
    } else {
        printk(KERN_INFO "Modtimer: Modconfig Panic! (unrecognized option: %s)", kbuf);
    }

    return len;
}

int init_timer_module( void ) {
    /* Create cbuffer for the top half */
    cbuffer = create_cbuffer_t(BUFFER_LENGTH);
    if (cbuffer == NULL) {
        printk(KERN_INFO "Modtimer: Cannot allocate cbuffer\n");
        return-ENOMEM;
    }
    
    /* Create modconfig /proc entry */
    modconfig_proc_entry = proc_create( "modconfig", 0666, NULL, &modconfig_entry_fops);
    if (modconfig_proc_entry == NULL) {
        printk(KERN_INFO "Modlist: Can't create /proc/modconfig entry\n");

        /* Cleanup */
        destroy_cbuffer_t(cbuffer);

        return -ENOMEM;
    }

    /* Initialize linked list */
    INIT_LIST_HEAD(&storage);

    /* Initialize work struct */
    INIT_WORK(&flush_work_data, flush_wq_function);

    /* Create timer */
    init_timer(&my_timer);
    /* Initialize field */
    my_timer.data=0;
    my_timer.function=fire_timer;
    my_timer.expires=jiffies + timer_period_ms;  /* Activate it one second from now */
    /* Activate the timer for the first time */
    add_timer(&my_timer);
    
    return 0;
}

void cleanup_timer_module( void ) {
    /* Clear linked list */
    clear_list();

    /* Destroy top half buffer */
    destroy_cbuffer_t(cbuffer);

    /* Remove /proc entries */
    remove_proc_entry("modconfig", NULL);
    
    /* Wait until completion of the timer function (if it's currently running) and delete timer */
    del_timer_sync(&my_timer);

    /* Wait until all jobs scheduled so far have finished */
    flush_scheduled_work();
}

module_init( init_timer_module );
module_exit( cleanup_timer_module );
