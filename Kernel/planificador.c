#include "planificador.h"

void* recibiendo(void* input, t_config* config){
	pcb* nuevo_pcb = malloc(sizeof(pcb)); // OJO con que no nos pase q se va el espacio de mem al salir del contexto de la llamada
	//printf("voy a recibir el pcb de consola\n");
	int* cliente_fd = (int *) input;
	void* buffer_instrucciones;
	int codigo_de_paquete = recibir_operacion(*cliente_fd);
	switch(codigo_de_paquete) {
	case OPERACION_ENVIO_INSTRUCCIONES:
		log_info(logger, "Recibidas instrucciones de la consola");
		buffer_instrucciones = recibir_instrucciones(*cliente_fd);
//----------------------Planificador a largo plazo---- agrega a cola de new
		sem_wait(&mutex_cola_new);
		crear_header(proximo_pid, buffer_instrucciones, config, nuevo_pcb, estimacion_inicial);
		log_info(logger, "Creado PCB correspondiente");
		nuevo_pcb->socket_consola = *cliente_fd;
		queue_push(cola_procesos_nuevos, (void*) nuevo_pcb);
		proximo_pid++;
		log_info(logger, "Agregado a la cola de nuevo");
		sem_post(&mutex_cola_new);
		sem_post(&contador_de_listas_esperando_para_estar_en_ready);

	}
	return NULL;
}



void* escuchar_consola(int socket_kernel_escucha, t_config* config){
	while(1){//desde aca

		pthread_t thread_type;
		int cliente_fd = esperar_cliente(socket_kernel_escucha);


		void* _f_aux(void* cliente_fd){
			log_info(logger, "Conectado con una consola");
			recibiendo(cliente_fd, config);// post recibiendo perdemos las cosas
			return NULL;
		}
		pthread_create(&thread_type, NULL, _f_aux, (void*) &cliente_fd );
	}

 return NULL;
}



void* funcion_pasar_a_ready(void* nada){ //aca vamos a tener que mandar a mem la peticion para que arme las tablas de paginas y me retorne el index
	bool valor;
	uint32_t rean_proc = REANUDAR_PROCESO;
	uint32_t crear = CREAR_PROCESO;
	int rta;
	while(1){
		pcb* proceso = malloc(sizeof(pcb));
		sem_wait(&contador_de_listas_esperando_para_estar_en_ready);
		sem_wait(&sem_contador_multiprogramacion);
		sem_wait(&mutex_cola_sus_ready);
		log_info(logger, "Llego un proceso nuevo a la cola de listo");

		valor = queue_is_empty(cola_procesos_sus_ready); // devuelve un valor distinto a cero si la cola esta vacia se hace adentro del sem, porque la cola puede modificarse

		if(!valor){ 
			proceso = (pcb*) queue_pop(cola_procesos_sus_ready);
			//printf("el pid que mando a reanudar es el del proceso: %d\n", proceso->pid);
			send(conexion_memoria, &rean_proc, sizeof(uint32_t), 0);
			send(conexion_memoria, &proceso->pid, sizeof(uint32_t), 0);
			send(conexion_memoria, &proceso->tabla_paginas, sizeof(uint32_t), 0);
			recv(conexion_memoria, &rta, sizeof(uint32_t), MSG_WAITALL);

			sem_wait(&mutex_cola_ready);

			queue_push(cola_de_ready, (void*) proceso);

			sem_post(&mutex_cola_ready);

			sem_post(&mutex_cola_sus_ready);
			sem_post(&binario_lista_ready);
			if (int_modo_planificacion == SRT) sem_post(&binario_plani_corto);

			continue;
		}

		sem_post(&mutex_cola_sus_ready); //doble signal en caso de que entre o no al IF


		sem_wait(&mutex_cola_new);

		valor = queue_is_empty(cola_procesos_nuevos);
		if(!valor){ 
			proceso = (pcb*) queue_pop(cola_procesos_nuevos);
			//printf("trato de crear un nuevo proceso a ready avisando a mem qu haga las est\n");
			send(conexion_memoria, &crear, sizeof(uint32_t), 0);
			send(conexion_memoria, &proceso->pid, sizeof(uint32_t), 0);
			send(conexion_memoria, &proceso->tamanio_en_memoria , sizeof(uint32_t), 0);
			log_info(logger, "Pedida tabla de páginas del proceso");
			recv(conexion_memoria, &proceso->tabla_paginas, sizeof(uint32_t), MSG_WAITALL); //recivo el index de la tabla de paginas
			log_info(logger, "Recibida tabla de páginas del proceso");
			sem_wait(&mutex_cola_ready);
			queue_push(cola_de_ready, (void*) proceso);

			sem_post(&mutex_cola_ready);
			sem_post(&binario_lista_ready);
			if (inicio || int_modo_planificacion == SRT) {
				sem_post(&binario_plani_corto);
				inicio =0;
			}
		}

		sem_post(&mutex_cola_new); //aca no pasa lo mismo ya que queremos que siga luego del if, por lo tanto va a pasar por el signal
	}
	return NULL;
}



void pasar_a_running(pcb* proceso_ready){
	
	//printf("tamanio = %d",proceso_ready->tamanio_stream_instrucciones);
	log_info(logger, "El proceso %d se va a running, su estimacion es: %ld", proceso_ready->pid, proceso_ready->estimacion_siguiente);
	t_buffer* pcb_serializado = malloc(sizeof(t_buffer));
	pcb_serializado = serializar_header(proceso_ready);
	empaquetar_y_enviar(pcb_serializado, conexion_CPU_dispatch, OPERACION_ENVIO_PCB);
	liberar_pcb(proceso_ready);
}

void* realizar_io(void* proceso_sin_castear){
	int sus = SUSPENDER_PROCESO;
	int rta;
	pcb* proceso = (pcb*)proceso_sin_castear;
	log_info(logger, "El proceso %d accede al dispositivo IO\n", proceso->pid);

	long aux [2];
	char* string_pid = string_itoa(proceso->pid);
	void* temporal =  dictionary_get(pid_handler, string_pid);
	memcpy(aux, temporal, sizeof(long)*2);
	char* estado = (char*) dictionary_get(process_state, string_pid);
	long timestamp_aux = currentTimeMillis();
	if(strcmp(estado, "blocked")==0){
		int cupo_restante = tiempo_de_espera_max - (timestamp_aux - aux[1]);
		if(cupo_restante > (int)aux[0]){
			usleep(((useconds_t)aux[0]) * (useconds_t)1000);
			hacer_cuenta_srt(proceso);

			sem_wait(&mutex_cola_ready);
			queue_push(cola_de_ready, proceso); // aca no llamo a mem porque nunca se suspendio
			sem_post(&mutex_cola_ready);
			sem_post(&binario_lista_ready);

			if (int_modo_planificacion == SRT) {
				sem_post(&binario_plani_corto);
				//printf("llame a corto p\n");
			}
			
			//ir a ready

		}else{
			usleep(((useconds_t)cupo_restante) * (useconds_t)1000);
			// avisar a memoria que se suspende el proceso!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			send(conexion_memoria, &sus, sizeof(int), 0);
			send(conexion_memoria, &proceso->tabla_paginas, sizeof(uint32_t), 0);
			recv(conexion_memoria, &rta, sizeof(int), MSG_WAITALL);
			log_info(logger, "Proceso bloqueado");
			dictionary_put(process_state, string_pid, "suspended blocked");
			sem_post(&sem_contador_multiprogramacion);
			usleep((((useconds_t)aux[0])- (useconds_t)cupo_restante)* (useconds_t)1000);
			dictionary_put(process_state, string_pid, "suspended ready");
			if (int_modo_planificacion == SRT) hacer_cuenta_srt(proceso);
			sem_wait(&mutex_cola_sus_ready);
			queue_push(cola_procesos_sus_ready, proceso);
			sem_post(&mutex_cola_sus_ready);
			sem_post(&contador_de_listas_esperando_para_estar_en_ready);
			//pasar a sus ready
			//NOTA IMPORTANTE AGREGAR SEMAFOROS A TODOS LOS DICCIONARIOS TMB!!!!!
		}


	}else {
		usleep(((useconds_t)aux[0]) * (useconds_t)1000);
		dictionary_put(process_state, string_pid, "suspended ready");
		if (int_modo_planificacion == SRT) hacer_cuenta_srt(proceso);
		sem_wait(&mutex_cola_sus_ready);
		queue_push(cola_procesos_sus_ready, proceso);
		sem_post(&mutex_cola_sus_ready);
		sem_post(&contador_de_listas_esperando_para_estar_en_ready);
	}

	sem_post(&dispositivo_de_io);//libero el disp de io
	return NULL;
}

void* dispositivo_io(void* nada){ //ESTE HAY QUE HACERLE UN HILO
	int sus = SUSPENDER_PROCESO;
	int rta;
	while(1){
		int cantidad_en_io = 0;
		sem_wait(&signal_a_io);
		sem_getvalue(&signal_a_io, &cantidad_en_io);
		//printf("a hacer iooooooooooooooooooooooooooooo");
		cantidad_en_io++;
		if(cantidad_en_io){
			sem_wait(&mutex_cola_suspendido);
			if(sem_trywait(&dispositivo_de_io) == 0){
				pthread_t hacer_io;
				void* proceso = queue_pop(cola_de_io);
				pthread_create(&hacer_io, NULL, realizar_io, proceso);
				cantidad_en_io--;

			}else sem_post(&signal_a_io);
			for(int i=1; i<=cantidad_en_io; i++ ){
				usleep(((useconds_t)5) * (useconds_t)1000);
				long tiempo_actual = currentTimeMillis();
				// checkear si la diferencia entre tiempo de pcb y actual es mayor a la espera maxima, entonces subo el grado de multiprogramacion
				pcb* proceso_sus = (pcb*) queue_pop(cola_de_io);
				char* string_pid = string_itoa(proceso_sus->pid);
				long* io  = (long*) dictionary_get(pid_handler, string_pid);
				char* estado = (char*) dictionary_get(process_state, string_pid);
				
				if((tiempo_actual - io[1]) > tiempo_de_espera_max && strcmp(estado, "blocked")==0) {
					sem_post(&sem_contador_multiprogramacion);
					// avisar a memoria que se suspende el proceso!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
					send(conexion_memoria, &sus, sizeof(int), 0);
					send(conexion_memoria, &proceso_sus->tabla_paginas, sizeof(uint32_t), 0);
					recv(conexion_memoria, &rta, sizeof(int), MSG_WAITALL);
					dictionary_put(process_state, string_pid, "suspended blocked");
				}
				free(string_pid);
				queue_push(cola_de_io, (void*) proceso_sus);
				//free(proceso_sus);
			}
			sem_post(&mutex_cola_suspendido);
		}
	}
	return NULL;
}

void* recibir_pcb_de_cpu(void* nada){
	uint32_t finalizar = 101;

	while(1){
		//printf("espero a recibir op\n");
		long io[2];
		int dato;
		int fin_proc = FINALIZAR_PROCESO;
		int rta;
		pcb* proceso_recibido = malloc(sizeof(pcb));
		int codigo_de_paquete = recibir_operacion(conexion_CPU_dispatch);
		log_info(logger, "Operación recibida: %d", codigo_de_paquete);
		switch(codigo_de_paquete) {

			case OPERACION_IO:

				dato = recibir_operacion(conexion_CPU_dispatch);
				recibir_pcb(conexion_CPU_dispatch, proceso_recibido);

				io[0] = (long)dato;
				io[1] = currentTimeMillis();
				char* string_pid = string_itoa(proceso_recibido->pid);
				proceso_recibido->real_actual = io[1] - proceso_recibido->timestamp_inicio_exe;
				log_info(logger, "vengo a hacer io: %d, mi nuevo real actual es: %ld", proceso_recibido->pid, proceso_recibido->real_actual);
				sem_wait(&mutex_cola_suspendido);

				dictionary_put(pid_handler, string_pid, (void*) io);
				queue_push(cola_de_io, proceso_recibido);
				sem_post(&mutex_cola_suspendido);

				dictionary_put(process_state, string_pid, "blocked");
				sem_post(&signal_a_io);
				sem_post(&binario_plani_corto);//aca esta el error ahora esta distito
				//printf("recibir intento planificar\n");
				break;

			case OPERACION_EXIT:
				recibir_pcb(conexion_CPU_dispatch, proceso_recibido);

				send(conexion_memoria, &fin_proc, sizeof(uint32_t), 0);  //MANDAR A MEMORIA QUE FINALIZA EL PROCESO!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				send(conexion_memoria, &proceso_recibido->tabla_paginas, sizeof(uint32_t), 0);  
				send(conexion_memoria, &proceso_recibido->tamanio_en_memoria, sizeof(uint32_t), 0); 
				recv(conexion_memoria, &rta, sizeof(uint32_t), MSG_WAITALL); 
				
				send(proceso_recibido->socket_consola, &finalizar, sizeof(uint32_t), 0); //aviso a la consola que termino su proceso
				log_info(logger, "finalizo el proceso: %d", proceso_recibido->pid);
				liberar_pcb(proceso_recibido);
				sem_post(&sem_contador_multiprogramacion);
				sem_post(&binario_plani_corto);
				break;

			case RESPUESTA_INTERRUPT:
				flag_respuesta_a_interrupcion = 1;
				proceso_recibido = malloc(sizeof(pcb));
				recibir_pcb(conexion_CPU_dispatch, proceso_recibido);

				log_info(logger, "El proceso %d fue interrumpido y su est es originalmente:%ld", proceso_recibido->pid, proceso_recibido->estimacion_siguiente);

				proceso_recibido->real_actual += currentTimeMillis() - proceso_recibido->timestamp_inicio_exe;
				sem_wait(&mutex_respuesta_interrupt);
				queue_push(rta_int, (void*) proceso_recibido);
				sem_post(&mutex_respuesta_interrupt);
				sem_post(&binario_flag_interrupt);

				break;
			case CPU_LIBRE:
				//printf("caso vacio\n");
				flag_respuesta_a_interrupcion = 2;
				sem_post(&binario_flag_interrupt);

				break;
			}
	}
 return NULL;
}





void* planificador_de_corto_plazo(void* nada){
	while(1){
		sem_wait(&binario_plani_corto);
		log_info(logger, "El planificador de corto plazo fue llamado");
		sem_wait(&binario_lista_ready);
		//printf("hay algo para hacer\n");
		pcb* proceso_a_ejecutar = NULL;
		flag_respuesta_a_interrupcion = 0;
		int num_aux = OPERACION_INTERRUPT;
		int flag_vacio = 0;

		if (int_modo_planificacion == SRT){
			send(conexion_cpu_interrupt, &num_aux, sizeof(int), 0);  //srt hace una interrupcion al cpu para desalojar
			log_info(logger, "El planificador SRT envía interrupción a CPU");
			sem_wait(&binario_flag_interrupt);
			log_info(logger, "Respuesta de CPU recibida");
			pcb* candidato_del_stack;



			switch(flag_respuesta_a_interrupcion){
				case 1:{
					sem_wait(&mutex_respuesta_interrupt);
					pcb* ejecutado = (pcb*) queue_pop(rta_int);
					sem_post(&mutex_respuesta_interrupt);
					log_info(logger, "se me interrumpio: %d, mi nueva estimacion es: %ld", ejecutado->pid, ejecutado->estimacion_siguiente);
					log_info(logger, "caso 1 desaloje al proceso: %d", ejecutado->pid);
					candidato_del_stack = algoritmo_srt(); //ver quien es el mas corto en la lista de ready
					log_info(logger, "El planificador desalojado tiene estimacion: %ld, y el candidato: %ld", ejecutado->estimacion_siguiente, candidato_del_stack->estimacion_siguiente);
					if((ejecutado->estimacion_siguiente - ((currentTimeMillis()) - ejecutado->timestamp_inicio_exe)) <= (candidato_del_stack->estimacion_siguiente - candidato_del_stack->real_actual)){ //aca se fija si el de la cpu es mas corto y lo pone en running
						log_info(logger, "el que pasa a running es: %d, gano el que estaba en cpu", ejecutado->pid);
						pasar_a_running(ejecutado);
						sem_post(&binario_lista_ready);
						//liberar_pcb(candidato_del_stack);
					}
					else{
						log_info(logger, "el que pasa a running es: %d", candidato_del_stack->pid);
						remover_de_cola_ready(candidato_del_stack);
						candidato_del_stack->timestamp_inicio_exe = (currentTimeMillis());
						pasar_a_running(candidato_del_stack);
						sem_wait(&mutex_cola_ready);
						queue_push(cola_de_ready, ejecutado);
						sem_post(&mutex_cola_ready);
						sem_post(&binario_lista_ready);
					}

					break;
				}
				case 2:{			//caso en el que la cpu esta vacia y no nos da nada
					log_info(logger, "caso 2 la cpu esta vacia");
					candidato_del_stack = algoritmo_srt();
					remover_de_cola_ready(candidato_del_stack);
					candidato_del_stack->timestamp_inicio_exe = (currentTimeMillis());
					pasar_a_running(candidato_del_stack);
					break;
				}
			}


		}else if (int_modo_planificacion == FIFO){
			sem_wait(&mutex_cola_ready);
			flag_vacio = queue_is_empty(cola_de_ready);
			if(!flag_vacio) proceso_a_ejecutar = (pcb*) queue_pop(cola_de_ready);
			sem_post(&mutex_cola_ready);
			if(!flag_vacio) pasar_a_running(proceso_a_ejecutar);
		}
	}
	
	return NULL;
}



pcb* algoritmo_srt(){
	int tamanio;
	int iterador = 1;

	pcb* auxiliar1 = malloc(sizeof(pcb));
	pcb* auxiliar2 = malloc(sizeof(pcb));


	sem_wait(&mutex_cola_ready);
	tamanio = queue_size(cola_de_ready);
	auxiliar1 = queue_pop(cola_de_ready);
	queue_push(cola_de_ready, (void*) auxiliar1);

	while(iterador < tamanio){
		iterador++;
		auxiliar2 = queue_pop(cola_de_ready);
		queue_push(cola_de_ready, (void*) auxiliar2);

		if(!(auxiliar1->estimacion_siguiente <= auxiliar2->estimacion_siguiente)) auxiliar1 = auxiliar2;
	}
	sem_post(&mutex_cola_ready);
	
	return auxiliar1;

}

void remover_de_cola_ready(pcb* item){
	pcb* auxiliar = malloc(sizeof(pcb));
	sem_wait(&mutex_cola_ready);
	int tamanio = queue_size(cola_de_ready);
	int iterador = 1;
	while(iterador <= tamanio){
		iterador++;
		auxiliar = queue_peek(cola_de_ready);
		if(auxiliar->pid == item->pid){
			queue_pop(cola_de_ready);
		}else{
			auxiliar = queue_pop(cola_de_ready);
			queue_push(cola_de_ready,(void*) auxiliar);
		}
	}
	sem_post(&mutex_cola_ready);
	//liberar_pcb(auxiliar); DUDA SI DEBERIA IR O NO creo que si
}


void hacer_cuenta_srt(pcb* proceso_deseado){ //esto hay que llamarlo dentro de los semaforos
	proceso_deseado->estimacion_siguiente = proceso_deseado->real_actual * alfa + (proceso_deseado->estimacion_siguiente * (1 - alfa));
	proceso_deseado->real_actual = 0;
	log_info(logger, "recalcule la estimacion del proceso: %d, su nueva estimacion es: %ld", proceso_deseado->pid, proceso_deseado->estimacion_siguiente);
	
}
