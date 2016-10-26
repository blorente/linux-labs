#include <linux/kernel.h>
#include <linux/syscalls.h>

SYSCALL_DEFINE0 (lin_hello) {
	printk(KERN_DEBUG "Hello Kernel\n");
	return 0;
}