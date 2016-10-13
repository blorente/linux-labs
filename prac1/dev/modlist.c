#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>

MODULE_LICENSE("GPL");

static struct proc_dir_entry *proc_entry;
struct list_head storage; /* Lista enlazada */

/* Nodos de la lista */
typedef struct {
  int data;
  struct list_head storage_links;
} list_item_t;


static ssize_t modlist_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {
  printk(KERN_INFO "Modlist: Reading\n");
  return 0;
}

static ssize_t modlist_write(struct file *filp, const char __user *buf, size_t len, loff_t *off) {
  printk(KERN_INFO "Modlist: Module unloaded.\n");
  return len;
}

static const struct file_operations proc_entry_fops = {
    .read = modlist_read,
    .write = modlist_write,    
};



int init_modlist_module( void ) {
  int ret = 0;

  proc_entry = proc_create( "modlist", 0666, NULL, &proc_entry_fops);
  if (proc_entry == NULL) {
    ret = -ENOMEM;
    printk(KERN_INFO "Modlist: Can't create /proc entry\n");
  } else {
    printk(KERN_INFO "Modlist: Module loaded\n");
  }  

  return ret;
}


void exit_modlist_module( void ) {
  remove_proc_entry("modlist", NULL);
  printk(KERN_INFO "Modlist: Module unloaded.\n");
}


module_init( init_modlist_module );
module_exit( exit_modlist_module );
