#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>

MODULE_LICENSE("GPL");

#define BUFFER_LENGTH 50

static struct proc_dir_entry *proc_entry;

static ssize_t fifoproc_write(struct file *filp, const char __user *buf, size_t len, loff_t *off) {  
  if ((*off) > 0) /* The application can write in this entry just once !! */
    return 0;
   
  *off+=len;            /* Update the file pointer */
  
  return len;
}

static ssize_t fifoproc_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {  
  int nr_bytes;
  
  if ((*off) > 0) /* Tell the application that there is nothing left to read */
      return 0;
    
  (*off)+=len;  /* Update the file pointer */

  return nr_bytes; 
}

static const struct file_operations proc_entry_fops = {
    .read = fifoproc_read,
    .write = fifoproc_write,    
};

int init_fifoproc_module( void )
{
  int ret = 0;

  proc_entry = proc_create( "fifoproc", 0666, NULL, &proc_entry_fops);
  if (proc_entry == NULL) {
    ret = -ENOMEM;
    printk(KERN_INFO "fifoproc: Can't create /proc entry\n");
  } else {
    printk(KERN_INFO "fifoproc: Module loaded\n");
  }
  return ret;
}


void exit_fifoproc_module( void )
{
  remove_proc_entry("fifoproc", NULL);
  printk(KERN_INFO "fifoproc: Module unloaded.\n");
}


module_init( init_fifoproc_module );
module_exit( exit_fifoproc_module );
