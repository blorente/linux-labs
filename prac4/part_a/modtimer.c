#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/random.h>

MODULE_LICENSE("GPL");
/* Based on code from Juan Carlos Saez */

#define MAX_INT_TO_INSERT 100

struct timer_list my_timer; /* Structure that describes the kernel timer */

/* Nodes of the list */
struct list_item_t {
  int data;
  struct list_head storage_links;
} list_item_t;
struct list_head storage; /* Linked list to store numbers */

/* Functions to manipulate the list */
static int append_to_list(int elem);
static void remove_from_list(int elem);
static void clear_list( void );

/* Function invoked when timer expires (fires) */
static void fire_timer(unsigned long data) {
    /* Insert number into linked list */
    unsigned int to_insert = get_random_int() % MAX_INT_TO_INSERT;
    printk(KERN_INFO "Modtimer: int to insert [%i]", to_insert);
    append_to_list(to_insert);

    /* Re-activate the timer one second from now */
    mod_timer( &(my_timer), jiffies + HZ); 
}

int append_to_list(int elem) {
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
      printk(KERN_INFO "Modlist: Deleting node (%i)\n", item->data);
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
    printk(KERN_INFO "Modlist: Deleting node\n");
    item = list_entry(cur_node, struct list_item_t, storage_links);
    list_del(cur_node);
    vfree(item);
  }
}

int init_timer_module( void ) {
    /* Initialize linked list */
    INIT_LIST_HEAD(&storage); 

    /* Create timer */
    init_timer(&my_timer);
    /* Initialize field */
    my_timer.data=0;
    my_timer.function=fire_timer;
    my_timer.expires=jiffies + HZ;  /* Activate it one second from now */
    /* Activate the timer for the first time */
    add_timer(&my_timer); 
    return 0;
}

void cleanup_timer_module( void ) {
    /* Clear linked list */
    clear_list();    

    /* Wait until completion of the timer function (if it's currently running) and delete timer */
    del_timer_sync(&my_timer);
}

module_init( init_timer_module );
module_exit( cleanup_timer_module );
