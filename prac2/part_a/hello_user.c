#include <linux/errno.h>
#include <sys/syscall.h>
#include <linux/unistd.h>
#include <stdio.h>

#ifdef __i386__
#define __NR_hello XXX
#else
#define __NR_hello XXX
#endif

long lin_hello( void ) {
	return (long) syscall(__NR_hello);
}

int main() {
	return (int) lin_hello();
}