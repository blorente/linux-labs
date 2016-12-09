#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/list.h>
#include <asm-generic/uaccess.h>

MODULE_LICENSE("GPL");

#define MAX_CHAR 100
#define MAX_ITEMS_LIST 100
// Only modified on startup or inside a lock
int list_items;

/* Nodos de la lista */
struct list_item_t {
  int data;
  struct list_head storage_links;
} list_item_t;

struct list_head storage; /* Lista enlazada */
DEFINE_SPINLOCK(sp);

static struct proc_dir_entry *proc_entry;

static int append_to_list(int elem);
static void remove_from_list(int elem);
static void clear_list( void );
// Call the blocking function vfree in each of the nodes
static void free_nodes(struct list_item_t** elems, unsigned int nelems);

static ssize_t modlist_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {
  int nr_bytes;
  char *result;
  struct list_item_t* item = NULL;
  struct list_head* cur_node = NULL;
  char kbuf[MAX_CHAR];
  
  printk(KERN_INFO "Modlist: Reading\n");

  /* Tell the application that there is nothing left to read */
  if ((*off) > 0)
      return 0;

  /* Use result as a writing head */
  result = &kbuf[0];
  
  spin_lock(&sp);
  list_for_each(cur_node, &storage) {
    /* item points to the structure wherein the links are embedded */
    item = list_entry(cur_node, struct list_item_t, storage_links);
    printk(KERN_INFO "Modlist: Reading (element: %i)", item->data);
    result += sprintf(result, "%i\n", item->data);
  }
  spin_unlock(&sp);

  /* Use pointer arithmetic to determine how many bytes were written */
  nr_bytes = result - kbuf;

  printk(KERN_INFO "Modlist: Reading (finished: %s)", kbuf);
    
  if (len < nr_bytes)
    return -ENOSPC;
  
  /* Transfer data from the kernel to userspace */  
  if (copy_to_user(buf, &kbuf[0], nr_bytes))
    return -EINVAL;
    
  (*off) += len;  /* Update the file pointer */

  return nr_bytes; 
}

static ssize_t modlist_write(struct file *filp, const char __user *buf, size_t len, loff_t *off) {

  int elem;
  char kbuf[MAX_CHAR];
  
  if ((*off) > 0)
      return 0;

  if (len + 1 > MAX_CHAR) {
    printk(KERN_INFO "Modlist: Panic! (not enough space)\n");
    return -ENOSPC;
  }

  printk(KERN_INFO "Modlist: Writing.\n");
  if (copy_from_user( &kbuf[0], buf, len )) {
    printk(KERN_INFO "Modlist: Panic! (failed copy_from_user)\n");
    return -EFAULT;
  }

  kbuf[len] = '\0'; // Add string terminator

  printk(KERN_INFO "Modlist: Writing (raw: %s)", kbuf);

  if (sscanf(kbuf, "add %i", &elem) == 1) {
    if ( append_to_list(elem) ) 
      return -ENOMEM;    
  } else if (sscanf(kbuf, "remove %i", &elem) == 1) {
    remove_from_list(elem);
  } else if (strcmp(kbuf, "cleanup\n") == 0) {
    clear_list();
  } else {
    printk(KERN_INFO "Modlist: Writing (unrecognized option: %s)", kbuf);
  }

  return len;
}

int append_to_list(int elem) {
  // Blocking -> Must be called outsde the lock
  struct list_item_t *new_item = (struct list_item_t *)vmalloc(sizeof(struct list_item_t));
  spin_lock(&sp);
  new_item->data = elem;
  list_add_tail(&new_item->storage_links, &storage);
  list_items++;
  spin_unlock(&sp);
  return 0;
}

void remove_from_list(int elem) {
  struct list_item_t *item = NULL;
  struct list_head *cur_node = NULL;
  struct list_head *aux_node = NULL;
  struct list_item_t *to_be_removed[1];

  spin_lock(&sp);
  list_for_each_safe(cur_node, aux_node, &storage) {    
    item = list_entry(cur_node, struct list_item_t, storage_links);
    if (item->data == elem) {
      printk(KERN_INFO "Modlist: Deleting node (%i)\n", item->data);
      list_del(cur_node);
      to_be_removed[0] = item;
      list_items--;
    }
  }
  spin_unlock(&sp);

  // Blocking -> Must be called outsde the lock
  free_nodes(to_be_removed, 1);
}

void clear_list() {
  struct list_head *cur_node = NULL;
  struct list_head *aux_node = NULL;
  unsigned int num_items = 0;
  struct list_item_t *to_be_removed[MAX_ITEMS_LIST];

  spin_lock(&sp);
  list_for_each_safe(cur_node, aux_node, &storage) {
    printk(KERN_INFO "Modlist: Deleting node\n");
    to_be_removed[num_items] = list_entry(cur_node, struct list_item_t, storage_links);
    num_items++;
    list_del(cur_node);
  }
  list_items = 0;
  spin_unlock(&sp);

  // Blocking -> Must be called outsde the lock
  free_nodes(to_be_removed, num_items);
}

void free_nodes(struct list_item_t **elems, unsigned int nelems) {
  int i;
  for(i = 0; i < nelems; i++) {
    vfree(elems[i]);
  }
}

static const struct file_operations proc_entry_fops = {
    .read = modlist_read,
    .write = modlist_write,
};

int init_modlist_module( void ) {
  int ret = 0;

  INIT_LIST_HEAD(&storage);
  list_items = 0;
  proc_entry = proc_create( "modlist", 0666, NULL, &proc_entry_fops);
  if (proc_entry == NULL) {
    ret = -ENOMEM;
    printk(KERN_INFO "Modlist: Can't create /proc entry\n");
  } else {
    printk(KERN_INFO "Modlist: Module loaded.\n");
  }
  return ret;
}


void exit_modlist_module( void ) {
  clear_list();
  remove_proc_entry("modlist", NULL);
  printk(KERN_INFO "Modlist: Module unloaded.\n");
}

module_init( init_modlist_module );
module_exit( exit_modlist_module );
