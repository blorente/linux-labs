#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/random.h>
#include <linux/vmalloc.h>
#include <linux/list.h>
#include <linux/smp.h>
#include <linux/semaphore.h>
#include <asm-generic/uaccess.h>
#include "cbuffer.h"

MODULE_LICENSE("GPL");
/* Based on code from Juan Carlos Saez */

#define BUFFER_LENGTH 24

volatile int cleanup_done = 0;
static void cleanup( void );

/* Cbuffer for the Top Half */
static cbuffer_t* cbuffer;
DEFINE_SPINLOCK(cbuffer_sl);

/* Functions to manipulate the list */
#define MAX_ITEMS_LIST sizeof(unsigned int)
struct list_item_t {
  unsigned int data;
  struct list_head storage_links;
} list_item_t;
struct list_head storage; /* Linked list to store numbers */
struct semaphore list_sem; //Linked list semaphore "mutex"
volatile unsigned int list_items; // Only modified at startup or inside a lock
static int append_to_list(unsigned int elem);
static int remove_from_list(int elem);
static int clear_list( void );
static void free_nodes(struct list_item_t** elems, unsigned int nelems);

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
volatile int timer_period_ms = 3000;
volatile int emergency_threshold = 12;
volatile int max_random = 100;
static int modconfig_open(struct inode *inode, struct file *file);
static int modconfig_release(struct inode *inode, struct file *file);
static ssize_t modconfig_read(struct file *filp, char __user *buf, size_t len, loff_t *off);
static ssize_t modconfig_write(struct file *filp, const char __user *buf, size_t len, loff_t *off);
static const struct file_operations modconfig_entry_fops = {
    .open = modconfig_open,
    .release = modconfig_release,
    .read = modconfig_read,
    .write = modconfig_write,
};

/* Functions for modtimer proc entry */
#define MAX_KBUF_MODTIMER 100
volatile int opened_for_read = 0;
volatile int reader_waiting = 0;
struct semaphore read_sem;
static struct proc_dir_entry *modtimer_proc_entry;
static int modtimer_open(struct inode *inode, struct file *file);
static int modtimer_release(struct inode *inode, struct file *file);
static ssize_t modtimer_read(struct file *filp, char __user *buf, size_t len, loff_t *off);
static const struct file_operations modtimer_entry_fops = {
    .open = modtimer_open,
    .release = modtimer_release,
    .read = modtimer_read,
};

int append_to_list(unsigned int elem) {
    struct list_item_t *new_item = (struct list_item_t *)vmalloc(sizeof(struct list_item_t));
    new_item->data = elem;
    if(down_interruptible(&list_sem)) {
        return -EINTR;
    }
    list_add_tail(&new_item->storage_links, &storage);
    list_items++;
    up(&list_sem);
    return 0;
}

int remove_from_list(int elem) {
    struct list_item_t *item = NULL;
    struct list_head *cur_node = NULL;
    struct list_head *aux_node = NULL;
    struct list_item_t *to_be_removed[1];

    if(down_interruptible(&list_sem)) {
        return -EINTR;
    }
    list_for_each_safe(cur_node, aux_node, &storage) {    
    item = list_entry(cur_node, struct list_item_t, storage_links);
        if (item->data == elem) {
            printk(KERN_INFO "Modtimer: Deleting node (%i)\n", item->data);
            list_del(cur_node);
            to_be_removed[0] = item;
            list_items--;
        }
    }
    up(&list_sem);

    // Blocking -> Must be called outsde the lock
    free_nodes(to_be_removed, 1);
    return 0;
}

int clear_list() {
    struct list_head *cur_node = NULL;
    struct list_head *aux_node = NULL;
    unsigned int num_items = 0;
    struct list_item_t *to_be_removed[MAX_ITEMS_LIST];

    if(down_interruptible(&list_sem)) {
        return -EINTR;
    }
    list_for_each_safe(cur_node, aux_node, &storage) {
        printk(KERN_INFO "Modlist: Deleting node\n");
        to_be_removed[num_items] = list_entry(cur_node, struct list_item_t, storage_links);
        num_items++;
        list_del(cur_node);
    }
    list_items = 0;
    up(&list_sem);

    // Blocking -> Must be called outsde the lock
    free_nodes(to_be_removed, num_items);
    return 0;
}

void free_nodes(struct list_item_t **elems, unsigned int nelems) {
    int i;
    for(i = 0; i < nelems; i++) {
        vfree(elems[i]);
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
    unsigned long lock_flags = 0;
    unsigned int *nums_to_insert = NULL;
    unsigned int item_to_insert = 0;
    unsigned int num_items = 0;
    unsigned int i = 0;
    /* Flush the buffer into the linked list */
    printk(KERN_INFO "Modtimer: Work started\n");

    /* Blocking: assign items to a data structure to copy from buffer */
    nums_to_insert = (unsigned int *)vmalloc(BUFFER_LENGTH);
    if (nums_to_insert == NULL) {
        printk(KERN_INFO "Modtimer: Failed to allocate insertion buffer\n");
        return;
    }

    /* Retrieve items from buffer */
    spin_lock_irqsave(&cbuffer_sl, lock_flags);
    num_items = size_cbuffer_t(cbuffer) / 4; // No truncating problems
    remove_items_cbuffer_t(cbuffer, (char *)nums_to_insert, num_items * 4);
    spin_unlock_irqrestore(&cbuffer_sl, lock_flags);
    
    /* Blocking: Insert elements into linked list */
    for (i = 0; i < num_items; i++) {
        item_to_insert = nums_to_insert[i];
        printk(KERN_INFO "Modtimer: Inserting elem %i into list\n", item_to_insert);
        if (append_to_list(item_to_insert)) {
            printk(KERN_INFO "Modtimer: Inserting into list Failed!\n");
            vfree(nums_to_insert);
            return;
        }
    }
    vfree(nums_to_insert);

    /* Awake possible user space reader */
    if (down_interruptible(&list_sem)) {
        printk(KERN_INFO "Modtimer: Notifying user program failed!\n");
        return;
    }
    if (reader_waiting > 0) {
        up(&read_sem);
        reader_waiting = 0;
    }
    up(&list_sem);
}

void fire_timer(unsigned long data) {
    /* Create random number to insert */
    unsigned int to_insert = get_random_int() % max_random;
    unsigned int current_size = 0;
    unsigned long lock_flags = 0;
    printk(KERN_INFO "Modtimer: int to insert [%i]\n", to_insert);

    /* Insert into buffer to be flushed into list */
    spin_lock_irqsave(&cbuffer_sl, lock_flags);
    insert_items_cbuffer_t(cbuffer, (char *)&to_insert, 4);
    current_size = size_cbuffer_t(cbuffer);
    spin_unlock_irqrestore(&cbuffer_sl, lock_flags);

    /* Enqueue flush if necessary */
    if (current_size >= emergency_threshold) {
        /* Create deferred work in another CPU */        
        int cpu_to_use = select_cpu();
        /* Enqueue work */     
        schedule_flush(cpu_to_use);
    }

    /* Re-activate the timer timer_period_ms from now */
    mod_timer( &(my_timer), jiffies + timer_period_ms); 
}

int modconfig_open(struct inode *inode, struct file *file) {
    try_module_get(THIS_MODULE);
    return 0;
}

int modconfig_release(struct inode *inode, struct file *file) {
    module_put(THIS_MODULE);
    return 0;
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

int modtimer_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "Modtimer: /proc/modtimer open\n");

    if(down_interruptible(&list_sem)) {
        return -EINTR;
    }

    opened_for_read = 1;    
    
    /* Wait while the list is empty */
    printk(KERN_INFO "Modtimer: Waiting for list to fill...\n");
    while(list_items == 0) {            
        reader_waiting = 1;
        up(&list_sem);
        printk(KERN_INFO "Modtimer: Registering reader in queue...\n");
        if (down_interruptible(&read_sem)) {
            down(&list_sem);
            reader_waiting = 0;
            up(&list_sem);
            return -EINTR;
        }
        printk(KERN_INFO "Modtimer: Checking list emptiness...\n");
        if (down_interruptible(&list_sem))
            return -EINTR;
    }
    up(&list_sem);

    /* Increment module refcount */
    try_module_get(THIS_MODULE);
    return 0;
}

int modtimer_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "Modtimer: /proc/modtimer release\n");
    cleanup();

    /* Decrement module reference count */
    module_put(THIS_MODULE);
    return 0;
}

ssize_t modtimer_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {    
    printk(KERN_INFO "Modtimer: /proc/modtimer read\n");
    if ((*off) > 0)
      return 0;

    (*off) += len;

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

    /* Create modtimer /proc entry */
    modtimer_proc_entry = proc_create( "modtimer", 0666, NULL, &modtimer_entry_fops);
    if (modtimer_proc_entry == NULL) {
        printk(KERN_INFO "Modlist: Can't create /proc/modtimer entry\n");

        /* Cleanup */
        destroy_cbuffer_t(cbuffer);
        remove_proc_entry("modconfig", NULL);

        return -ENOMEM;
    }

    /* Initialize linked list */
    INIT_LIST_HEAD(&storage);
    sema_init(&list_sem, 1);
    list_items = 0;

    /* Initialize reader semaphore */
    sema_init(&read_sem, 0);

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
    if (!cleanup_done) {
        cleanup();
    }

    /* Remove /proc entries */
    remove_proc_entry("modconfig", NULL);
    remove_proc_entry("modtimer", NULL);
    printk(KERN_INFO "Modtimer: Proc entries removed.\n");
}

void cleanup( void ) {
    /* Wait until completion of the timer function (if it's currently running) and delete timer */
    del_timer_sync(&my_timer);
    printk(KERN_INFO "Modtimer: Timer completed.\n");

    /* Wait until all jobs scheduled so far have finished */
    flush_scheduled_work();
    printk(KERN_INFO "Modtimer: Scheduled jobs flushed.\n");
    
    /* Clear linked list */
    if(clear_list()) {
        printk(KERN_INFO "Modtimer: Failed to clear list.\n");
    }
    printk(KERN_INFO "Modtimer: List cleared.\n");

    /* Destroy top half buffer */
    destroy_cbuffer_t(cbuffer);
    printk(KERN_INFO "Modtimer: Cbuffer destroyed.\n");

    cleanup_done = 1;
}

module_init( init_timer_module );
module_exit( cleanup_timer_module );
