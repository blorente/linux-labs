obj-m = modtimer.o #modtimer.c no ha de existir
modtimer-objs = timermod.o cbuffer.o
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean