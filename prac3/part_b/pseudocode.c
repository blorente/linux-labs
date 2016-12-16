mutex mtx;
condvar prod,cons;
int prod_count = 0,cons_count = 0;
cbuffer_t* cbuffer;

void fifoproc_open(bool abre_para_lectura) {
	if (abre_para_lectura) {
		/* Bloqueo hasta que haya productores */
		lock(mtx);
		
		/* Notificar que ya hay un consumidor */
		cons_count++;

		/* Esperar hasta que haya productores */
		while(prod_count == 0) {
			cond_wait(cons);
		}

		cond_broadcast(prod);

		unlock(mtx);
	} else {
		/* Bloqueo hasta que haya consumidores */
		lock(mtx);
		
		/* Notificar que hay un escritor */
		prod_count++;

		/* Esperar hasta que haya consumidores */
		while(cons_count == 0) {
			cond_wait(prod);
		}

		cond_broadcast(cons);

		unlock(mtx);
	}
}

int fifoproc_write(char* buff, int len) {
	char kbuffer[MAX_KBUF];
	if (len > MAX_CBUFFER_LEN || len > MAX_KBUF) { return Error;}
	if (copy_from_user(kbuffer, buff, len)) { return Error;}
	lock(mtx);

	/* Esperar hasta que haya hueco para insertar (debe haber consumidores) */
	while (nr_gaps_cbuffer_t(cbuffer) < len && cons_count > 0){
		cond_wait(prod,mtx);
	}

	/* Producir */
	insert_items_cbuffer_t(cbuffer,kbuffer,len);

	/* Despertar a posible consumidor bloqueado */
	cond_signal(cons);

	unlock(mtx);
	return len;
}

int fifoproc_read(const char* buff, int len) {
	char kbuffer[MAX_KBUF];
	if (len > MAX_CBUFFER_LEN || len > MAX_KBUF) { return Error;}
	lock(mtx);

	/* Señalar que vamos a esperar */
	cons_count++;

	/* Esperar a que haya suficientes items */
	while(size_cbuffer_t(cbuffer) < len) {
		cond_wait(cons, mtx);
	}

	/* Consumir */
	remove_items_cbuffer_t(cbuffer, kbuffer, len);
	copy_to_user(buff, kbuffer, len);

	/* Señalar que hemos consumido */
	cond_signal(prod);

	unlock(mtx);
	return len;
}

void fifoproc_release(bool lectura) {
	if (lectura) {
		lock(mtx);
		cons_count--;

		/* Puede que tengamos una ruptura de pipe */
		if (cons_count == 0) {
			cond_signal(prod);
		}
		unlock(mtx);
	} else {
		lock(mtx);
		prod_count--;
		unlock(mtx);
	}
}

