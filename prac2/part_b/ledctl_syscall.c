#include <linux/kernel.h> 
#include <linux/syscalls.h> 
#include <asm-generic/errno.h>
#include <linux/init.h>
#include <linux/tty.h>      /* For fg_console */
#include <linux/kd.h>       /* For KDSETLED */
#include <linux/vt_kern.h>

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

SYSCALL_DEFINE1(ledctl, unsigned int, leds) {
	int ret;
	unsigned int newMask;

	ret = 0;

	kbd_driver = get_kbd_driver_handler();
	printk(KERN_INFO "Ledctl: Call started\n"); 

	printk(KERN_INFO "Ledctl: Writing %d", leds);
	newMask = transform_mask(leds);
	printk(KERN_INFO "Ledctl: Mask transformed from %d to %d\n", leds, newMask);	
	if ((ret = set_leds(kbd_driver, newMask)) != 0) {
		printk(KERN_INFO "Ledctl: Call to set_leds failed\n");
		return ret;
	}

	printk(KERN_INFO "Ledctl: Call ended\n"); 
	return ret;
}
