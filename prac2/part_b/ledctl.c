#include <linux/module.h> 
#include <asm-generic/errno.h>
#include <linux/init.h>
#include <linux/tty.h>      /* For fg_console */
#include <linux/kd.h>       /* For KDSETLED */
#include <linux/vt_kern.h>

// For mod test, remove later!
#include <linux/proc_fs.h>

#define ALL_LEDS_ON 0x7
#define ALL_LEDS_OFF 0

#define MAX_CHAR 100

static struct proc_dir_entry *proc_entry;

struct tty_driver* kbd_driver= NULL;

/* Get driver handler */
struct tty_driver* get_kbd_driver_handler(void) {
	printk(KERN_INFO "Ledctl: loading\n");
	printk(KERN_INFO "Ledctl: fgconsole is %x\n", fg_console);
	return vc_cons[fg_console].d->port.tty->driver;
}

/* Set led state to that specified by mask */
static inline int set_leds(struct tty_driver* handler, unsigned int mask) {
	return (handler->ops->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED,mask);
}

static ssize_t ledctl_write(struct file *filp, const char __user *buf, size_t len, loff_t *off) {
	char *kbuf;
	int written_bytes;
	int led_mask;

	if ((*off) > 0)
    	return 0;

	written_bytes = len < MAX_CHAR ? len + 1 : MAX_CHAR;
	kbuf = (char *)vmalloc(written_bytes + 1);
	if (!kbuf) {		
		printk(KERN_INFO "Ledctl: Not enough memory!\n");
		return -ENOMEM;
	}

	if (copy_from_user( &kbuf[0], buf, written_bytes )) {
		printk(KERN_INFO "Ledctl: Panic! (failed copy_from_user)\n");
		vfree(kbuf);
		return -EFAULT;
	}

	kbuf[written_bytes] = '\0';

	if (sscanf(kbuf, "%x", &led_mask) == 1) {
		printk(KERN_INFO "Ledctl: Writing %d", led_mask);
		if (set_leds(kbd_driver, led_mask) != 0) {
			printk(KERN_INFO "Ledctl: Call to set_leds failed\n");
		}
	}

	vfree(kbuf);
	return written_bytes;
}

static const struct file_operations proc_entry_fops = {
    .write = ledctl_write,
};

static int __init ledctl_init(void) {	
	kbd_driver= get_kbd_driver_handler();
	set_leds(kbd_driver, ALL_LEDS_ON); 

   // Test /proc entry
	proc_entry = proc_create( "ledctl", 0666, NULL, &proc_entry_fops );
	if (proc_entry == NULL) {
	  printk(KERN_INFO "Ledctl: Can't create /proc entry\n");
	  return -ENOMEM;
	} else {
	  printk(KERN_INFO "Ledctl: Module loaded\n");
	}  
	return 0;
}

static void __exit ledctl_exit(void) {
    set_leds(kbd_driver,ALL_LEDS_OFF); 
}

module_init(ledctl_init);
module_exit(ledctl_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Ledctl");
