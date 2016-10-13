#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <asm-generic/uaccess.h>

MODULE_LICENSE("GPL");

#define LIST_LENGTH 10
static int data[LIST_LENGTH];
static int lpos;

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
  int nr_bytes;
  
  printk(KERN_INFO "Modlist: Reading\n");
  if ((*off) > 0) /* Tell the application that there is nothing left to read */
      return 0;

  nr_bytes = lpos * sizeof(int);
  char *result = (char *)vmalloc(nr_bytes + lpos + 1); // Dest string length = space for the data, lpos linebreaks and \0
  if (!result) {
    printk(KERN_INFO "Modlist: Panic! (failed vmalloc)\n");
    return 0;
  }
  
  int i = 0;
  for (; i < lpos; i++) {
    char elem[6];
    sprintf(&elem[0], "%d\n", data[i]);
    strcat(result, &elem[0]);
    printk(KERN_INFO "Modlist: Reading (partial: %s)", result);
  }

  printk(KERN_INFO "Modlist: Reading (finished: %s)", result);
    
  if (len < nr_bytes)
    return -ENOSPC;
  
  /* Transfer data from the kernel to userspace */  
  if (copy_to_user(buf, &result[0], nr_bytes))
    return -EINVAL;
    
  (*off) += len;  /* Update the file pointer */

  if (result)
    vfree(result);

  return nr_bytes; 
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

  // Eliminar esto una vez implementemos write
  lpos = 0;
  int i = 0;
  for(; i < 5; i++) {
    data[i] = i;
    lpos++;
  }

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
      printk(KERN_INFO "Modlist: Module loaded 1\n");
    }    
  }

  return ret;
}


void exit_modlist_module( void ) {
  vfree(kbuf);
  remove_proc_entry("modlist", NULL);
  printk(KERN_INFO "Modlist: Module unloaded.\n");
}


module_init( init_modlist_module );
module_exit( exit_modlist_module );
