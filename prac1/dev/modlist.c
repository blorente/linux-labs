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

#define MAX_OPTION_LENGTH 10

/* Nodos de la lista */
typedef struct {
  int data;
  struct list_head storage_links;
} list_item_t;

struct list_head storage; /* Lista enlazada */

static struct proc_dir_entry *proc_entry;

static int allocate_kbuf(int num_bytes);
static int append_to_list(int elem);
static void remove_from_list(int elem);
static void clear_list( void );

int allocate_kbuf(int num_bytes) {
  if (kbuf) 
    vfree(kbuf);

  kbuf = (char *)vmalloc(num_bytes);
  if (!kbuf) {
    printk(KERN_INFO "Modlist: Panic! (failed vmalloc)\n");
    return 0;    
  }

  memset( kbuf, 0, num_bytes );
  return num_bytes;
}

static ssize_t modlist_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {
  int nr_bytes;
  char *result;
  int i;

  /* Allocate space to compose the result, then write to user space*/
  if ( allocate_kbuf(len) == 0) {
    return -ENOMEM;
  }
  
  printk(KERN_INFO "Modlist: Reading\n");

  /* Tell the application that there is nothing left to read */
  if ((*off) > 0)
      return 0;

  result = kbuf;
  /* Write the reult in kbuf, using the result pointer */
  for (i = 0; i < lpos; i++) {    
    result += sprintf(result, "%i\n", data[i]);
  }

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
      return 0;    
  } else if (sscanf(kbuf, "remove %i", &elem) == 1) {
    remove_from_list(elem);
  } else if (strcmp(kbuf, "cleanup") == 0) {
    clear_list();
  } else {
    printk(KERN_INFO "Modlist: Writing (unrecognized option)\n");
  }

  return len;
}

int append_to_list(int elem) {
  if (lpos >= LIST_LENGTH)
    return 1;

  data[lpos++] = elem;
  return 0;
}

void remove_from_list(int elem) {
  int i;
  int j;

  for (i = 0; i < lpos; i++) {
    if (data[i] == elem) {
      for (j = i; j < lpos; j++) {
        data[j] = data[j + 1]; 
      }
      i--;
      lpos--;
    }
  }
}

void clear_list() {
  lpos = 0;
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
