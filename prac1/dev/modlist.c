#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <asm-generic/uaccess.h>

MODULE_LICENSE("GPL");

#define LIST_LENGTH 10
static int data[LIST_LENGTH];

#define MAX_CHAR 100
static char *kbuf;

struct list_head storage; /* Lista enlazada */

/* Nodos de la lista */
typedef struct {
  int data;
  struct list_head storage_links;
} list_item_t;

static struct proc_dir_entry *proc_entry;

static ssize_t modlist_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {
  printk(KERN_INFO "Modlist: Reading\n");
  return 0;
}

static ssize_t modlist_write(struct file *filp, const char __user *buf, size_t len, loff_t *off) {

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

  printk(KERN_INFO "Modlist: Writing %s", kbuf);

  return len;
}

static const struct file_operations proc_entry_fops = {
    .read = modlist_read,
    .write = modlist_write,
};

int init_modlist_module( void ) {
  int ret = 0;

  kbuf = (char *)vmalloc( MAX_CHAR );

  if (!kbuf) {
    printk(KERN_INFO "Modlist: Panic! (failed vmalloc)\n");
    ret = -ENOMEM;
  } else {

    memset( kbuf, 0, MAX_CHAR );

    proc_entry = proc_create( "modlist", 0666, NULL, &proc_entry_fops);
    if (proc_entry == NULL) {
      ret = -ENOMEM;
      printk(KERN_INFO "Modlist: Can't create /proc entry\n");
    } else {
      printk(KERN_INFO "Modlist: Module loaded\n");
    }    
  }

  return ret;
}


void exit_modlist_module( void )
{
  vfree(kbuf);
  remove_proc_entry("modlist", NULL);
  printk(KERN_INFO "Modlist: Module unloaded.\n");
}


module_init( init_modlist_module );
module_exit( exit_modlist_module );
