#include <linux/kernel.h> 
#include <linux/syscalls.h> 
#include <asm-generic/errno.h>
#include <linux/init.h>
#include <linux/tty.h>      /* For fg_console */
#include <linux/kd.h>       /* For KDSETLED */
#include <linux/vt_kern.h>

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

static unsigned int transform_mask(unsigned int original) {
	int num, caps, scroll;
	
	num = (original >> 2) & 1;
	caps = (original >> 1) & 1;
	scroll = original & 1;
	return (caps << 2) + (num << 1) + scroll;
}

static ssize_t ledctl_write(unsigned int led_mask) {
	int res = 0;
	printk(KERN_INFO "Ledctl: Writing %d", led_mask);
	if (res = set_leds(kbd_driver, transform_mask(led_mask)) != 0) {
		printk(KERN_INFO "Ledctl: Call to set_leds failed\n");
		return res;
	}

	return 0;
}

static const struct file_operations proc_entry_fops = {
    .write = ledctl_write,
};

static int __init ledctl_init(void) {	
	kbd_driver = get_kbd_driver_handler();
	printk(KERN_INFO "Ledctl: Module loaded\n"); 
	return 0;
}

static void __exit ledctl_exit(void) {
    set_leds(kbd_driver,ALL_LEDS_OFF); 
}

SYSCALL_DEFINE1(ledctl, unsigned int, leds) {
	int ret = 0;

	if (ret = ledctl_init() != 0) {
		return ret;
	}
	if (ret = ledctl_write(leds)) {
		return ret;
	}

	ledctl_exit();
	return 0;
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Ledctl");
