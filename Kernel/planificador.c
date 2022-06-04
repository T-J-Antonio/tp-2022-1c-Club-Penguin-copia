#include "planificador.h"

void* recibiendo(void* input, t_config* config){
	pcb* nuevo_pcb = malloc(sizeof(pcb)); // OJO con que no nos pase q se va el espacio de mem al salir del contexto de la llamada

	int ayuding;
	int* cliente_fd = (int *) input;
	void* buffer_instrucciones;
	int codigo_de_paquete = recibir_operacion(*cliente_fd);
	switch(codigo_de_paquete) {
	case OPERACION_ENVIO_INSTRUCCIONES:
		buffer_instrucciones = recibir_instrucciones(*cliente_fd);
//----------------------Planificador a largo plazo---- agrega a cola de new
		sem_wait(&mutex_cola_new);
		crear_header(proximo_pid, buffer_instrucciones, config, nuevo_pcb, estimacion_inicial);
		nuevo_pcb->socket_consola = *cliente_fd;
		queue_push(cola_procesos_nuevos, (void*) nuevo_pcb);
		proximo_pid++;
		sem_post(&mutex_cola_new);
		sem_post(&contador_de_listas_esperando_para_estar_en_ready);
		sem_getvalue(&contador_de_listas_esperando_para_estar_en_ready, &ayuding);

	}
	return NULL;
}



void* escuchar_consola(int socket_kernel_escucha, t_config* config){
	while(1){//desde aca

		pthread_t thread_type;
		int cliente_fd = esperar_cliente(socket_kernel_escucha);


		void* _f_aux(void* cliente_fd){
			recibiendo(cliente_fd, config);// post recibiendo perdemos las cosas
			printf("recibi\n");
			return NULL;
		}
		pthread_create(&thread_type, NULL, _f_aux, (void*) &cliente_fd );
	}

 return NULL;
}



void* funcion_pasar_a_ready(void* nada){ //necesita un hilo
	bool valor;
	pcb* proceso = malloc(sizeof(pcb));
	while(1){
		sem_wait(&contador_de_listas_esperando_para_estar_en_ready);
		sem_wait(&sem_contador_multiprogramacion);
		printf("intente pasar a ready\n");

		sem_wait(&mutex_cola_sus_ready);

		valor = queue_is_empty(cola_procesos_sus_ready); // devuelve un valor distinto a cero si la cola esta vacia se hace adentro del sem, porque la cola puede modificarse

		if(!valor){
			proceso = queue_pop(cola_procesos_sus_ready);

			sem_wait(&mutex_cola_ready);

			queue_push(cola_de_ready, (void*) proceso);

			sem_post(&mutex_cola_ready);

			sem_post(&mutex_cola_sus_ready);
			sem_post(&binario_lista_ready);
			if (int_modo_planificacion == SRT) planificador_de_corto_plazo();


			return NULL;
		}

		sem_post(&mutex_cola_sus_ready); //doble signal en caso de que entre o no al IF


		sem_wait(&mutex_cola_new);

		valor = queue_is_empty(cola_procesos_nuevos);
		if(!valor){
			proceso = queue_pop(cola_procesos_nuevos);

			sem_wait(&mutex_cola_ready);
			queue_push(cola_de_ready, (void*) proceso);

			sem_post(&mutex_cola_ready);
			sem_post(&binario_lista_ready);
			if (int_modo_planificacion == SRT) planificador_de_corto_plazo();
			if (inicio) {
				planificador_de_corto_plazo();
				inicio =0;
			}
		}

		sem_post(&mutex_cola_new); //aca no pasa lo mismo ya que queremos que siga luego del if, por lo tanto va a pasar por el signal
	}

	return NULL;
}



void pasar_a_running(pcb* proceso_ready){
	proceso_ready->timestamp_inicio_exe = ((float)time(NULL)*1000);
	t_buffer* pcb_serializado = malloc(sizeof(t_buffer));
	pcb_serializado = serializar_header(proceso_ready);
	empaquetar_y_enviar(pcb_serializado, conexion_CPU_dispatch, OPERACION_ENVIO_PCB);
	//vamos a tener que hacer un free

}

void* realizar_io(void* proceso_sin_castear){
	printf("me llamaron a hacer io\n");
	pcb* proceso = (pcb*)proceso_sin_castear;

	float aux [2];
	char* string_pid = string_itoa(proceso->pid);
	void* temporal =  dictionary_get(pid_handler, string_pid);
	memcpy(aux, temporal, sizeof(float)*2);
	char* estado = (char*) dictionary_get(process_state, string_pid);
	float timestamp_aux = ((float)time(NULL))*1000;
	if(strcmp(estado, "blocked")==0){
		int cupo_restante = tiempo_de_espera_max - (timestamp_aux - aux[1]);
		if(cupo_restante > (int)aux[0]){
			usleep(((useconds_t)aux[0]) * (useconds_t)1000);
			hacer_cuenta_srt(proceso);

			sem_wait(&mutex_cola_ready);
			queue_push(cola_de_ready, proceso);
			sem_post(&mutex_cola_ready);
			sem_post(&binario_lista_ready);
			if (int_modo_planificacion == SRT) planificador_de_corto_plazo();
			//ir a ready

		}else{
			usleep(((useconds_t)cupo_restante) * (useconds_t)1000);
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
	while(1){
		int cantidad_en_io = 0;
		sem_wait(&signal_a_io);
		sem_getvalue(&signal_a_io, &cantidad_en_io);
		cantidad_en_io++;
		if(cantidad_en_io){
			sem_wait(&mutex_cola_suspendido);
			printf("intento bloquear");
			if(sem_trywait(&dispositivo_de_io) == 0){
				pthread_t hacer_io;
				void* proceso = queue_pop(cola_de_io);
				pthread_create(&hacer_io, NULL, realizar_io, proceso);
				cantidad_en_io--;

			}else sem_post(&signal_a_io);
			printf("cant io: %d\n",cantidad_en_io);
			for(int i=1; i<=cantidad_en_io; i++ ){
				printf("llego uno aca\n");
				float tiempo_actual = ((float) time(NULL))*1000;
				// checkear si la diferencia entre tiempo de pcb y actual es mayor a la espera maxima, entonces subo el grado de multiprogramacion
				pcb* proceso_sus = (pcb*) queue_pop(cola_de_io);
				char* string_pid = string_itoa(proceso_sus->pid);
				void* temporal = dictionary_get(pid_handler, string_pid);
				float io[2];
				memcpy(io, temporal, sizeof(float)*2);
				printf("%f\n", io[0]);
				if((tiempo_actual - io[1]) > tiempo_de_espera_max) {
					sem_post(&sem_contador_multiprogramacion); //aparte mandar un msg que no conocemos a memoria
					char* string_pid = string_itoa(proceso_sus->pid);
					dictionary_put(process_state, string_pid, "suspended blocked");
				}
				queue_push(cola_de_io, (void*) proceso_sus);
			}
			sem_post(&mutex_cola_suspendido);
		}
	}
	return NULL;
}

void* recibir_pcb_de_cpu(void* nada){
	uint32_t finalizar = 101;

	while(1){
		float io[2] = {0,0};
		int dato;
		pcb* proceso_recibido = malloc(sizeof(pcb));
		int codigo_de_paquete = recibir_operacion(conexion_CPU_dispatch);

		printf("yendo al switch\n");
		switch(codigo_de_paquete) {

			case OPERACION_IO:
				printf("llego para hacer io\n");
				// luego de modificar el pcb hay que calcular el srt para el pcb

				dato = recibir_operacion(conexion_CPU_dispatch);
				printf("dato: %d\n",dato);
				recibir_pcb(conexion_CPU_dispatch, proceso_recibido);

				io[0] = (float)dato;
				io[1] = ((float)time(NULL))*1000;
				char* string_pid = string_itoa(proceso_recibido->pid);
				proceso_recibido->real_actual = io[1] - proceso_recibido->timestamp_inicio_exe;
				sem_wait(&mutex_cola_suspendido);

				dictionary_put(pid_handler, string_pid, (void*) io);
				queue_push(cola_de_io, proceso_recibido);
				sem_post(&mutex_cola_suspendido);

				dictionary_put(process_state, string_pid, "blocked");
				sem_post(&signal_a_io);
				planificador_de_corto_plazo();
				break;

			case OPERACION_EXIT:
				printf("exit\n");
				recibir_pcb(conexion_CPU_dispatch, proceso_recibido);

				//2 send(memoria, borrar_pid, int, 0);
				//recv(memoria, borrar_pid, int, MSG_WAITALL); mensaje de que ya elimino la mem para proceder
				send(proceso_recibido->socket_consola, &finalizar, sizeof(uint32_t), 0); //aviso a la consola que termino su proceso

				sem_post(&sem_contador_multiprogramacion);
				planificador_de_corto_plazo();
				break;

			case OPERACION_INTERRUPT:
				ejecutado = malloc(sizeof(pcb));
				recibir_pcb(conexion_CPU_dispatch, ejecutado);

				ejecutado->estimacion_siguiente = ((float)time(NULL)*1000) - ejecutado->estimacion_siguiente; //son detalles

				flag_respuesta_a_interupcion = 1;
				sem_post(&binario_flag_interrupt);


				break;
			case CPU_LIBRE:

				flag_respuesta_a_interupcion = 2;
				sem_post(&binario_flag_interrupt);


				break;
			}
	}

 return NULL;
}





void planificador_de_corto_plazo(){
	sem_wait(&binario_lista_ready);
	pcb* proceso_a_ejecutar = NULL;
	flag_respuesta_a_interupcion =0;
	uint32_t num_aux = OPERACION_INTERRUPT;
	int flag_vacio = 0;

	if (int_modo_planificacion == SRT){
		printf("llegue\n");
		send(conexion_cpu_interrupt, &num_aux, sizeof(uint32_t), 0);  //srt hace una interrupcion al cpu para desalojar
		printf("mande int, espero rta cpu\n");
		sem_wait(&binario_flag_interrupt);
		printf("recibi respuesta de la cpu\n");
		pcb* candidato_del_stack;
		switch(flag_respuesta_a_interupcion){
			case 1:{
				candidato_del_stack = algoritmo_srt(); //ver quien es el mas corto en la lista de ready
				if(ejecutado->estimacion_siguiente <= candidato_del_stack->estimacion_siguiente){ //aca se fija si el de la cpu es mas corto y lo pone en running
					pasar_a_running(ejecutado);
				}
				else{pasar_a_running(candidato_del_stack);
				remover_de_cola_ready(candidato_del_stack);
				sem_wait(&mutex_cola_ready);
				queue_push(cola_de_ready, ejecutado);
				sem_post(&mutex_cola_ready);
				}

				break;
			}
			case 2:{			//caso en el que la cpu esta vacia y no nos da nada
				candidato_del_stack = algoritmo_srt();
				pasar_a_running(candidato_del_stack);
				remover_de_cola_ready(candidato_del_stack);
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
}


void hacer_cuenta_srt(pcb* proceso_deseado){ //esto hay que llamarlo dentro de los semaforos
	proceso_deseado->estimacion_siguiente = proceso_deseado->real_actual * alfa + (proceso_deseado->estimacion_siguiente * (1 - alfa));
}
