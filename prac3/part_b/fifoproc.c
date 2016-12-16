#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <asm-generic/uaccess.h>
#include <asm-generic/errno.h>
#include <linux/semaphore.h>
#include "cbuffer.h"

MODULE_LICENSE("GPL");

#define BUFFER_LENGTH 50
#define MAX_CHARS_KBUF 50

static struct proc_dir_entry *proc_entry;
struct semaphore sem_mutex; //mutex mtx;
struct semaphore sem_prod, sem_cons;//condvar prod,cons;
int prod_count = 0,cons_count = 0;
int nr_prod_waiting = 0,nr_cons_waiting = 0;
static cbuffer_t* cbuffer;

static int fifoproc_open(struct inode *inode, struct file *file) {
	if (file->f_mode & FMODE_READ) {
		printk(KERN_INFO "fifoproc - READ: Starting open...\n");
		/* Bloqueo hasta que haya productores */

		/* 1.- Adquirir mutex */
		// lock(&mutex)
		if(down_interruptible(&sem_mutex)) {
			return -EINTR;
		}
		
		/* 2.- Registrarse como consumidor */
		cons_count++;

		/* 3.- condvar hasta que haya productores */
		// while(prod_count == 0) {
		// 	cond_wait(cons);
		// }
		printk(KERN_INFO "fifoproc - READ: Waiting for productors...\n");
		while(prod_count == 0) {			
			nr_cons_waiting++;
			up(&sem_mutex);
			printk(KERN_INFO "fifoproc - READ: Registering in the queue...\n");
			if (down_interruptible(&sem_cons)) {
				down(&sem_mutex);
				nr_cons_waiting--;
				up(&sem_mutex);
				return -EINTR;
			}
			printk(KERN_INFO "fifoproc - READ: Checking the condition...\n");
			if (down_interruptible(&sem_mutex))
				return -EINTR;
		}

		/* 4.- Notificar que hay un consumidor */
		// cond_broadcast(prod);
		printk(KERN_INFO "fifoproc - READ: Notifying producers...\n");
		while (nr_prod_waiting > 0) {
			up(&sem_prod);
			nr_prod_waiting--;
		}

		/* 5.- Soltar Mutex */
		// unlock(&mutex);
		printk(KERN_INFO "fifoproc - READ: Releasing mutex...\n");
		up(&sem_mutex);

		printk(KERN_INFO "fifoproc - READ: Done!\n");
	} else {
		printk(KERN_INFO "fifoproc - WRITE: Starting open...\n");
		/* Bloqueo hasta que haya consumidores */

		/* 1.- Adquirir mutex */
		// lock(&mutex)
		if(down_interruptible(&sem_mutex)) {
			return -EINTR;
		}

		/* 2.- Registrarse como productor */
		prod_count++;

		/* 3.- condvar hasta que haya consumidores */
		// while(cons_count == 0) {
		// 	cond_wait(prod);
		// }
		printk(KERN_INFO "fifoproc - WRITE: Waiting for consumers...\n");
		while(cons_count == 0) {			
			nr_prod_waiting++;
			up(&sem_mutex);			
			printk(KERN_INFO "fifoproc - WRITE: Registering in the queue...\n");
			if (down_interruptible(&sem_prod)) {
				down(&sem_mutex);
				nr_prod_waiting--;
				up(&sem_mutex);
				return -EINTR;
			}
			printk(KERN_INFO "fifoproc - WRITE: Checking the condition...\n");
			if (down_interruptible(&sem_mutex))
				return -EINTR;
		}

		/* 4.- Notificar que hay un productor */
		// cond_broadcast(prod);
		printk(KERN_INFO "fifoproc - WRITE: Notifying consumers...\n");
		while (nr_cons_waiting > 0) {
			up(&sem_cons);
			nr_cons_waiting--;
		}
		
		/* 5.- Soltar Mutex */
		// unlock(&mutex);
		printk(KERN_INFO "fifoproc - WRITE: Releasing mutex...\n");
		up(&sem_mutex);

		printk(KERN_INFO "fifoproc - WRITE: Done!\n");
	}
	return 0;
}

static int fifoproc_release(struct inode *inode, struct file *file) {
	if (file->f_mode & FMODE_READ) {
		/* 1.- Adquirir mutex */
		// lock(mtx);
		if(down_interruptible(&sem_mutex)) {
			return -EINTR;
		}

		/* 2.- Darse de baja como consumidor */
		printk(KERN_INFO "fifoproc - READ: Sign Out...\n");
		cons_count--;
		
		/* 3.- Devolver mutex */
		// unlock(mtx);
		printk(KERN_INFO "fifoproc - READ: Releasing mutex...\n");
		up(&sem_mutex);
	} else {
		/* 1.- Adquirir mutex */
		// lock(mtx);
		if(down_interruptible(&sem_mutex)) {
			return -EINTR;
		}

		/* 2.- Darse de baja como consumidor */
		printk(KERN_INFO "fifoproc - WRITE: Sign Out...\n");
		prod_count--;
		
		/* 3.- Devolver mutex */
		// unlock(mtx);
		printk(KERN_INFO "fifoproc - WRITE: Releasing mutex...\n");
		up(&sem_mutex);
	}
	return 0;
}

static ssize_t fifoproc_write(struct file *filp, const char __user *buf, size_t len, loff_t *off) {  
	char kbuffer[MAX_CHARS_KBUF];

	if ((*off) > 0)
		return 0;

	if (len > MAX_CHARS_KBUF - 1) {
		return -ENOSPC;
	}
	if (copy_from_user( kbuffer, buf, len )) {
		return -EFAULT;
	}

	kbuffer[len] ='\0'; 
	*off+=len;
	printk(KERN_INFO "fifoproc - WRITE: Waiting to insert [%s]\n", kbuffer);

	/* 1.- Adquirir mutex */
	// lock(&mutex)
	if(down_interruptible(&sem_mutex)) {
		return -EINTR;
	}

	/* 2.- Esperar hasta que haya hueco para insertar (debe haber consumidores) */
	//while (nr_gaps_cbuffer_t(cbuffer) < len && cons_count > 0){
	//	cond_wait(prod,mtx);
	//}
	while (nr_gaps_cbuffer_t(cbuffer) < len && cons_count > 0){
		nr_prod_waiting++;
		up(&sem_mutex);

		/* Bloqueo en cola de espera */		
		if (down_interruptible(&sem_prod)){
			down(&sem_mutex);
			nr_prod_waiting--;
			up(&sem_mutex);		
			return -EINTR;
		}

		if (down_interruptible(&sem_mutex)){
			return -EINTR;
		}	
	}

	/* 3.- Producir */
	printk(KERN_INFO "fifoproc - WRITE: Inserting into buffer...\n");
	insert_items_cbuffer_t(cbuffer,kbuffer,len);

	/* 4.- Despertar a posible consumidor bloqueado */
	// cond_signal(cons);
	if (nr_cons_waiting > 0) {
		up(&sem_cons);	
		nr_cons_waiting--;
  	}
	
	/* 5.- Soltar Mutex */
	// unlock(&mutex);
	printk(KERN_INFO "fifoproc - WRITE: Releasing mutex...\n");
	up(&sem_mutex);

	return len;
}

static ssize_t fifoproc_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {  
	int nr_bytes = len > BUFFER_LENGTH ? BUFFER_LENGTH : len;
	char kbuffer[BUFFER_LENGTH];
  
	if ((*off) > 0) 
		return 0;

	/* 1.- Adquirir mutex */
	// lock(&mutex)
	if (down_interruptible(&sem_mutex)) {
		return -EINTR;
	}

	/* 2.- Bloquearse mientras buffer no contenga los elementos que necesitamos */
	//while(size_cbuffer_t(cbuffer) < nr_bytes) {
	//	cond_wait(cons, mtx);
	//}
	while (size_cbuffer_t(cbuffer) < nr_bytes) {
		nr_cons_waiting++;
		up(&sem_mutex);

		if (down_interruptible(&sem_cons)){
			down(&sem_mutex);
			nr_cons_waiting--;
			up(&sem_mutex);		
			return -EINTR;
		}	

		if (down_interruptible(&sem_mutex)){
			return -EINTR;
		}
	}

	/* 3.- Consumir */
	remove_items_cbuffer_t(cbuffer, kbuffer, nr_bytes);
  
	/* 4.- Despertar a los consumidores bloqueados (si hay alguno) */
	// cond_signal(prod);
	if (nr_prod_waiting > 0) {
		up(&sem_prod);	
		nr_prod_waiting--;
	}

	/* 5.- Salir de la sección crítica */	
	// unlock(mtx);
	up(&sem_mutex);

	/* 6.- Escribir el valor en el espacio de usuario */
	if (len < nr_bytes)
		return -ENOSPC;

	if (copy_to_user(buf, kbuffer, nr_bytes))
		return -EINVAL;

	(*off)+=nr_bytes;

	return nr_bytes; 
}

static const struct file_operations proc_entry_fops = {
	.open = fifoproc_open,
	.release = fifoproc_release,
    .read = fifoproc_read,
    .write = fifoproc_write,    
};

int init_fifoproc_module( void ) {
	int ret = 0;

	sema_init(&sem_mutex, 1);
	sema_init(&sem_prod, 0);
	sema_init(&sem_cons, 0);

	cbuffer = create_cbuffer_t(BUFFER_LENGTH);
	if (cbuffer == NULL) {
		ret = -ENOMEM;
		printk(KERN_INFO "fifoproc: Cannot allocate cbuffer\n");
	} else {
		proc_entry = proc_create("fifoproc", 0666, NULL, &proc_entry_fops);
		if (proc_entry == NULL) {
			ret = -ENOMEM;
			destroy_cbuffer_t(cbuffer);
			printk(KERN_INFO "fifoproc: Can't create /proc entry\n");
		} else {
			printk(KERN_INFO "fifoproc: Module loaded\n");
		}
	}
	return ret;
}


void exit_fifoproc_module( void ) {
	destroy_cbuffer_t(cbuffer);
	remove_proc_entry("fifoproc", NULL);
	printk(KERN_INFO "fifoproc: Module unloaded.\n");
}


module_init( init_fifoproc_module );
module_exit( exit_fifoproc_module );
