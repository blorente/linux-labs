#include "fifoproc.h"

static fifo_data_t fifo_data_base;

static int fifoproc_open(struct inode *inode, struct file *file) {

	fifo_data_t *fifo_data = (fifo_data_t*)PDE_DATA(file->f_inode);

	/* 1.- Adquirir mutex */
	// lock(&mutex)
	if(down_interruptible(&(fifo_data->sem_mutex))) {
		return -EINTR;
	}
	if (file->f_mode & FMODE_READ) {
		printk(KERN_INFO "fifoproc - READ: Starting open...\n");
		/* Bloqueo hasta que haya productores */
		
		/* 2.- Registrarse como consumidor */
		fifo_data->cons_count++;

		/* 3.- condvar hasta que haya productores */
		// while(prod_count == 0) {
		// 	cond_wait(cons);
		// }
		printk(KERN_INFO "fifoproc - READ: Waiting for productors...\n");
		while(fifo_data->prod_count == 0) {			
			fifo_data->nr_cons_waiting++;
			up(&(fifo_data->sem_mutex));
			printk(KERN_INFO "fifoproc - READ: Registering in the queue...\n");
			if (down_interruptible(&(fifo_data->sem_cons))) {
				down(&(fifo_data->sem_mutex));
				fifo_data->nr_cons_waiting--;
				up(&(fifo_data->sem_mutex));
				return -EINTR;
			}
			printk(KERN_INFO "fifoproc - READ: Checking the condition...\n");
			if (down_interruptible(&(fifo_data->sem_mutex)))
				return -EINTR;
		}

		/* 4.- Notificar que hay un consumidor */
		// cond_signal(prod);
		printk(KERN_INFO "fifoproc - READ: Notifying producers...\n");
		if (fifo_data->nr_prod_waiting > 0) {
			up(&(fifo_data->sem_prod));
			fifo_data->nr_prod_waiting--;
		}

		printk(KERN_INFO "fifoproc - READ: Done!\n");
	} else {
		printk(KERN_INFO "fifoproc - WRITE: Starting open...\n");
		/* Bloqueo hasta que haya consumidores */

		/* 2.- Registrarse como productor */
		fifo_data->prod_count++;

		/* 3.- condvar hasta que haya consumidores */
		// while(cons_count == 0) {
		// 	cond_wait(prod);
		// }
		printk(KERN_INFO "fifoproc - WRITE: Waiting for consumers...\n");
		while(fifo_data->cons_count == 0) {			
			fifo_data->nr_prod_waiting++;
			up(&(fifo_data->sem_mutex));			
			printk(KERN_INFO "fifoproc - WRITE: Registering in the queue...\n");
			if (down_interruptible(&(fifo_data->sem_prod))) {
				down(&(fifo_data->sem_mutex));
				fifo_data->nr_prod_waiting--;
				up(&(fifo_data->sem_mutex));
				return -EINTR;
			}
			printk(KERN_INFO "fifoproc - WRITE: Checking the condition...\n");
			if (down_interruptible(&(fifo_data->sem_mutex)))
				return -EINTR;
		}

		/* 4.- Notificar que hay un productor */
		// cond_signal(prod);
		printk(KERN_INFO "fifoproc - WRITE: Notifying consumers...\n");
		if (fifo_data->nr_cons_waiting > 0) {
			up(&(fifo_data->sem_cons));
			fifo_data->nr_cons_waiting--;
		}

		printk(KERN_INFO "fifoproc - WRITE: Done!\n");
	}

	/* 5.- Soltar Mutex */
	// unlock(&mutex);
	printk(KERN_INFO "fifoproc: Releasing mutex...\n");
	up(&(fifo_data->sem_mutex));
	return 0;
}

static int fifoproc_release(struct inode *inode, struct file *file) {

	fifo_data_t *fifo_data = (fifo_data_t*)PDE_DATA(file->f_inode);

	/* 1.- Adquirir mutex */
	// lock(mtx);
	if(down_interruptible(&(fifo_data->sem_mutex))) {
		return -EINTR;
	}
	if (file->f_mode & FMODE_READ) {

		/* 2.- Darse de baja como consumidor */
		printk(KERN_INFO "fifoproc - READ: Sign Out...\n");
		fifo_data->cons_count--;	

		/* 3.- Señalizar productores */
		// cond_signal(prod);
		printk(KERN_INFO "fifoproc - READ: Notifying producers...\n");
		if (fifo_data->nr_prod_waiting > 0) {
			up(&(fifo_data->sem_prod));
			fifo_data->nr_prod_waiting--;
		}	
		
	} else {
		/* 2.- Darse de baja como productor */
		printk(KERN_INFO "fifoproc - WRITE: Sign Out...\n");
		fifo_data->prod_count--;

		/* 3.- Señalizar productores */
		// cond_signal(cons);
		printk(KERN_INFO "fifoproc - WRITE: Notifying consumers...\n");
		if (fifo_data->nr_cons_waiting > 0) {
			up(&(fifo_data->sem_cons));
			fifo_data->nr_cons_waiting--;
		}

	}
	/* 4.- Si es necesario, vaciar el buffer */
	if (fifo_data->cons_count == 0 && fifo_data->prod_count == 0) {
		clear_cbuffer_t(fifo_data->cbuffer);
	}

	/* 5.- Devolver mutex */
	// unlock(mtx);
	printk(KERN_INFO "fifoproc: Releasing mutex...\n");
	up(&(fifo_data->sem_mutex));
	return 0;
}

static ssize_t fifoproc_write(struct file *filp, const char __user *buf, size_t len, loff_t *off) {  
	char kbuffer[MAX_CHARS_KBUF];
	fifo_data_t *fifo_data = (fifo_data_t*)PDE_DATA(filp->f_inode);

	//if ((*off) > 0)
	//	return 0;

	if (len > MAX_CHARS_KBUF || len > BUFFER_LENGTH) {
		return -ENOSPC;
	}
	if (copy_from_user( kbuffer, buf, len )) {
		return -EFAULT;
	}

	//*off+=len;
	printk(KERN_INFO "fifoproc - WRITE: Waiting to insert [%i] elems\n", len);

	/* 1.- Adquirir mutex */
	// lock(&mutex)
	if(down_interruptible(&(fifo_data->sem_mutex))) {
		return -EINTR;
	}

	/* 2.- Esperar hasta que haya hueco para insertar (debe haber consumidores) */
	//while (nr_gaps_cbuffer_t(cbuffer) < len && cons_count > 0){
	//	cond_wait(prod,mtx);
	//}
	while ((BUFFER_LENGTH - size_cbuffer_t(fifo_data->cbuffer)) < len && fifo_data->cons_count > 0) {
		printk(KERN_INFO "fifoproc - WRITE: Not enough gaps...\n");
		
		fifo_data->nr_prod_waiting++;
		up(&(fifo_data->sem_mutex));

		/* Bloqueo en cola de espera */		
		if (down_interruptible(&(fifo_data->sem_prod))) {
			down(&(fifo_data->sem_mutex));
			fifo_data->nr_prod_waiting--;
			up(&(fifo_data->sem_mutex));		
			return -EINTR;
		}

		if (down_interruptible(&(fifo_data->sem_mutex))) {
			return -EINTR;
		}	
	}

	/* 3.- Producir */
	printk(KERN_INFO "fifoproc - WRITE: Inserting into buffer...\n");
	insert_items_cbuffer_t(fifo_data->cbuffer, kbuffer, len);

	/* 4.- Despertar a posible consumidor bloqueado */
	// cond_signal(cons);
	if (fifo_data->nr_cons_waiting > 0) {
		up(&(fifo_data->sem_cons));	
		fifo_data->nr_cons_waiting--;
  	}
	
	/* 5.- Soltar Mutex */
	// unlock(&mutex);
	printk(KERN_INFO "fifoproc - WRITE: Releasing mutex...\n");
	up(&(fifo_data->sem_mutex));

	return len;
}

static ssize_t fifoproc_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {  
	int nr_bytes = len > BUFFER_LENGTH ? BUFFER_LENGTH : len;
	char kbuffer[BUFFER_LENGTH];

	fifo_data_t *fifo_data = (fifo_data_t*)PDE_DATA(filp->f_inode);
  
	if ((*off) > 0) 
		return 0;

	/* 1.- Adquirir mutex */
	// lock(&mutex)
	if (down_interruptible(&(fifo_data->sem_mutex))) {
		return -EINTR;
	}

	/* 2.- Bloquearse mientras buffer no contenga los elementos que necesitamos */
	//while(size_cbuffer_t(cbuffer) < nr_bytes) {
	//	cond_wait(cons, mtx);
	//}
	while (size_cbuffer_t(fifo_data->cbuffer) < nr_bytes && fifo_data->prod_count > 0) {
		printk(KERN_INFO "fifoproc - READ: Buffer empty...\n");	
		fifo_data->nr_cons_waiting++;
		up(&(fifo_data->sem_mutex));

		if (down_interruptible(&(fifo_data->sem_cons))) {
			down(&(fifo_data->sem_mutex));
			fifo_data->nr_cons_waiting--;
			up(&(fifo_data->sem_mutex));		
			return -EINTR;
		}	

		printk(KERN_INFO "fifoproc - READ: Checking condvar...\n");	
		if (down_interruptible(&(fifo_data->sem_mutex))) {
			return -EINTR;
		}
	}

	/* 3.- Consumir */
	if(fifo_data->prod_count == 0 && is_empty_cbuffer_t(fifo_data->cbuffer)) {	
		// unlock(mtx);
		up(&(fifo_data->sem_mutex));
		return 0;
	}

	printk(KERN_INFO "fifoproc - READ: Reading from buffer...\n");	
	remove_items_cbuffer_t(fifo_data->cbuffer, kbuffer, nr_bytes);
  
	/* 4.- Despertar a los productores bloqueados (si hay alguno) */
	// cond_signal(prod);
	if (fifo_data->nr_prod_waiting > 0) {
		up(&(fifo_data->sem_prod));	
		fifo_data->nr_prod_waiting--;
	}

	/* 5.- Salir de la sección crítica */	
	// unlock(mtx);
	up(&(fifo_data->sem_mutex));

	/* 6.- Escribir el valor en el espacio de usuario */
	if (len < nr_bytes)
		return -ENOSPC;

	if (copy_to_user(buf, kbuffer, nr_bytes))
		return -EINVAL;

	return nr_bytes; 
}

static const struct file_operations proc_entry_fops = {
	.open = fifoproc_open,
	.release = fifoproc_release,
    .read = fifoproc_read,
    .write = fifoproc_write,    
};

int init_fifo(fifo_data_t *fifo, char * name) {
	int ret = 0;
	struct proc_dir_entry *entry = NULL;

	sema_init(&(fifo->sem_mutex), 1);
	sema_init(&(fifo->sem_prod), 0);
	sema_init(&(fifo->sem_cons), 0);

	fifo->cbuffer = create_cbuffer_t(BUFFER_LENGTH);
	if (fifo->cbuffer == NULL) {
		ret = -ENOMEM;
		printk(KERN_INFO "fifoproc: Cannot allocate cbuffer for fifo %s\n", name);
	} else {
		entry = proc_create_data(name, 0666, NULL, &proc_entry_fops, fifo);
		if (entry == NULL) {
			ret = -ENOMEM;
			destroy_cbuffer_t(fifo->cbuffer);
			printk(KERN_INFO "fifoproc: Can't create /proc entry\n");
		} else {
			fifo->proc_entry = entry;
			fifo->name = name;			
			printk(KERN_INFO "fifoproc: Entry %s created succesfully\n", fifo->name);
		}
	}

	return ret;
}

int init_fifoproc_module( void ) {
	int ret = 0;

	ret = init_fifo(&fifo_data_base, "fifoproc");

	if (ret == 0) {
		printk(KERN_INFO "fifoproc: Module loaded\n");
	}

	return ret;
}


void exit_fifoproc_module( void ) {
	destroy_cbuffer_t(fifo_data_base.cbuffer);
	remove_proc_entry("fifoproc", NULL);
	printk(KERN_INFO "fifoproc: Module unloaded.\n");
}


module_init( init_fifoproc_module );
module_exit( exit_fifoproc_module );
