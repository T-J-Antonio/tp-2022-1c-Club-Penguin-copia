#include "utilsKernel.h"


uint32_t proximo_pid = 0;
uint32_t resultOk = 0;

float estimacion_inicial;
float alfa;

int flag_respuesta_a_interupcion;
int conexion_cpu_interrupt;
float tiempo_de_espera_max;
pcb* ejecutado;
float tiempo_inicio;
int int_modo_planificacion;

pcb* algoritmo_srt();


int main(void) {
	pid_handler = dictionary_create();

	process_state = dictionary_create();

	sem_init(mutex_cola_new, 0, 1);

	sem_init(mutex_cola_sus_ready, 0, 1);

	sem_init(mutex_cola_ready, 0, 1);

	sem_init(contador_de_listas_esperando_para_estar_en_ready, 0, 0);

	sem_init(binario_flag_interrupt, 0, 0);

	sem_init(mutex_cola_suspendido, 0, 1);

	sem_init(signal_a_io, 0, 0);

	sem_init(dispositivo_de_io, 0, 1);


	cola_procesos_nuevos = queue_create();
	cola_procesos_sus_ready = queue_create();
	cola_de_ready = queue_create();
	cola_de_io = queue_create();


	t_config* config = config_create("/home/utnso/Documentos/tp-2022-1c-Club-Penguin/Kernel/kernel.config");
	logger = log_create("log.log", "Servidor", 1, LOG_LEVEL_DEBUG);

	char* ip_CPU = config_get_string_value(config, "IP_CPU");
	char* puerto_CPU = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
	char* modo_de_planificacion = config_get_string_value(config, "ALGORITMO_PLANIFICACION");

	if(strcmp(modo_de_planificacion, "FIFO") ==0)int_modo_planificacion = 21;
	if(strcmp(modo_de_planificacion, "SRT") ==0)int_modo_planificacion = 22;

	tiempo_de_espera_max = (float) config_get_int_value(config, "TIEMPO_MAXIMO_BLOQUEADO");


	int grado_de_multiprogramacion = config_get_int_value(config, "GRADO_MULTIPROGRAMACION");
	sem_init(sem_contador_multiprogramacion, 0, grado_de_multiprogramacion);

	char* ip_kernel = config_get_string_value(config, "IP_KERNEL");
	char* puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
	int socket_kernel_escucha = iniciar_servidor(ip_kernel, puerto_escucha);//ip kernel unica, pero el puerto es el del escucha,

	char* puerto_interrupt = config_get_string_value(config, "PUERTO_CPU_INTERRUPT");
	conexion_cpu_interrupt = iniciar_servidor(ip_CPU, puerto_interrupt);//conexion al puerto de cpu para interrumpir

	estimacion_inicial = (float) config_get_int_value(config, "ESTIMACION_INICIAL");
	alfa = (float) config_get_int_value(config, "ALFA");

	conexion_CPU_dispatch = crear_conexion(ip_CPU, puerto_CPU);

	void* _f_aux_escucha_consola(void *socket_kernel_escucha){
		escuchar_consola(*(int*)socket_kernel_escucha, config);
		return NULL;
	}



	pthread_t socket_escucha_consola;
	pthread_create(&socket_escucha_consola, NULL, _f_aux_escucha_consola, (void*) &socket_kernel_escucha );

	pthread_t pasar_a_ready;
	pthread_create(&pasar_a_ready, NULL, funcion_pasar_a_ready, NULL );

	pthread_t recibir_procesos_por_cpu;
	pthread_create(&recibir_procesos_por_cpu, NULL, recibir_pcb_de_cpu, NULL );


	pthread_join(socket_escucha_consola, NULL);

	return EXIT_SUCCESS;
}





void* recibiendo(void* input, t_config* config){
	pcb* nuevo_pcb = malloc(sizeof(pcb)); // OJO con que no nos pase q se va el espacio de mem al salir del contexto de la llamada


	int* cliente_fd = (int *) input;
	void* buffer_instrucciones;
	int codigo_de_paquete = recibir_operacion(*cliente_fd);
	switch(codigo_de_paquete) {
	case OPERACION_ENVIO_INSTRUCCIONES:
		buffer_instrucciones = recibir_instrucciones(*cliente_fd);
//----------------------Planificador a largo plazo---- agrega a cola de new

		sem_wait(mutex_cola_new);

		crear_header(proximo_pid, buffer_instrucciones, config, nuevo_pcb, estimacion_inicial);
		//char* string_pid = string_itoa(proximo_pid);

		queue_push(cola_procesos_nuevos, (void*) nuevo_pcb);
		//dictionary_put(pid_handler, string_pid, (void*) cliente_fd); aca vamos a agregarlo al pcb no hace falta

		proximo_pid++;

		sem_post(contador_de_listas_esperando_para_estar_en_ready);

		sem_post(mutex_cola_new);

//--------------------fin de cola de new------------------------------------------

		//send(*cliente_fd, &resultOk, sizeof(uint32_t), 0); va a ser llamado en caso de finalizacion

	}
	return NULL;
}



void* escuchar_consola(int socket_kernel_escucha, t_config* config){
	while(1){//desde aca

		pthread_t thread_type;
		int cliente_fd = esperar_cliente(socket_kernel_escucha);

		void* _f_aux(void* cliente_fd){
			recibiendo(cliente_fd, config);// post recibiendo perdemos las cosas
			return NULL;
		}

		pthread_create(&thread_type, NULL, _f_aux, (void*) &cliente_fd );
	}

 return NULL;
}



void* funcion_pasar_a_ready(void* nada){
	bool valor;
	pcb* proceso = malloc(sizeof(pcb));

	sem_wait(contador_de_listas_esperando_para_estar_en_ready);
	sem_wait(sem_contador_multiprogramacion);


	sem_wait(mutex_cola_sus_ready);

	valor = queue_is_empty(cola_procesos_sus_ready); // devuelve un valor distinto a cero si la cola esta vacia se hace adentro del sem, porque la cola puede modificarse

	if(!valor){
		proceso = queue_pop(cola_procesos_sus_ready);

		sem_wait(mutex_cola_ready);

		queue_push(cola_de_ready, (void*) proceso);

		sem_post(mutex_cola_ready);

		sem_post(mutex_cola_sus_ready);

		return NULL;
	}

	sem_post(mutex_cola_sus_ready); //doble signal en caso de que entre o no al IF


	sem_wait(mutex_cola_new);

	valor = queue_is_empty(cola_procesos_nuevos);
	if(!valor){
		proceso = queue_pop(cola_procesos_nuevos);

		sem_wait(mutex_cola_ready);

		queue_push(cola_de_ready, (void*) proceso);

		sem_post(mutex_cola_ready);
	}

	sem_post(mutex_cola_new); //aca no pasa lo mismo ya que queremos que siga luego del if, por lo tanto va a pasar por el signal


	return NULL;
}



void pasar_a_running(pcb* proceso_ready){
	float tiempo_de_inicio = ((float)time(NULL)*1000); // cuando modifiquemos el pcb vamos a tener que guardar esta variable
	t_buffer* pcb_serializado = malloc(sizeof(t_buffer));
	pcb_serializado = serializar_header(proceso_ready);
	empaquetar_y_enviar(pcb_serializado, conexion_CPU_dispatch, OPERACION_ENVIO_PCB);
	//vamos a tener que hacer un free

}

void realizar_io(pcb* proceso){
	float aux [2];
	char* string_pid = string_itoa(proceso->pid);
	aux = (float*) dictionary_get(pid_handler, string_pid);
	char* estado = (char*) dictionary_get(process_state, string_pid);
	float timestamp_aux = ((float)time(NULL))*1000;
	if(strcmp(estado, "blocked")==0){
		float cupo_restante = tiempo_de_espera_max - (timestamp_aux - aux[1]);
		if(cupo_restante < aux[0]){
			usleep(cupo_restante);
			//aca tmb falta mutex
			queue_push(cola_de_ready, proceso);
			//ir a ready

		}else{
			usleep(cupo_restante);
			dictionary_put(process_state, string_pid, "suspended blocked");
			sem_post(sem_contador_multiprogramacion);
			usleep(aux[0]- cupo_restante);
			dictionary_put(process_state, string_pid, "suspended ready");
			//se que falta el mutex va para dsp
			queue_push(cola_procesos_sus_ready, proceso);
			//pasar a sus ready
			//NOTA IMPORTANTE AGREGAR SEMAFOROS A TODOS LOS DICCIONARIOS TMB!!!!!
		}


	}else {
		usleep(aux[0]);
		dictionary_put(process_state, string_pid, "suspended ready");
		//se que falta el mutex va para dsp
		queue_push(cola_procesos_sus_ready, proceso);
		//pasar a sus ready
	} //hacer espera

	sem_post(dispositivo_de_io);//libero el disp de io
}

void* dispositivo_io(){
	int cantidad_en_io;
	sem_getvalue(signal_a_io, &cantidad_en_io);
	if(cantidad_en_io){
		sem_wait(mutex_cola_suspendido);
		if(sem_trywait(dispositivo_de_io) == 0){
			//llamar a un hilo que se encargue de hacer que ese proceso haga su espera, y al terminar signal del semaforo
			queue_pop(cola_de_io);
			cantidad_en_io--;

		}
		for(int i=0; i<cantidad_en_io; i++ ){
			float tiempo_actual = ((float) time(NULL))*1000;
			// checkear si la diferencia entre tiempo de pcb y actual es mayor a la espera maxima, entonces subo el grado de multiprogramacion
			pcb* proceso_sus = queue_pop(cola_de_io);
			char* string_pid = string_itoa(proceso_sus->pid);
			float io[2] = (float*) dictionary_get(pid_handler, string_pid );
			if((tiempo_actual - io[1]) > tiempo_de_espera_max) {
				sem_post(sem_contador_multiprogramacion); //aparte mandar un msg que no conocemos a memoria
				char* string_pid = string_itoa(proceso_sus->pid);
				dictionary_put(process_state, string_pid, "suspended blocked");
			}
			queue_push(cola_de_io, (void*) proceso_sus);
		}
		sem_post(mutex_cola_suspendido);
	}



	return NULL;
}

void* recibir_pcb_de_cpu( t_config* config, pcb* pcb_ejecutado){
	int codigo_de_paquete = recibir_operacion(conexion_CPU_dispatch);
	pcb* proceso_recibido = malloc(sizeof(pcb));
	float io[2];
	uint32_t aux;
	int semaforo;
	while(1){								
		switch(codigo_de_paquete) {

		case OPERACION_IO:
			// luego de modificar el pcb hay que calcular el srt para el pcb

			recv(conexion_CPU_dispatch, &aux, sizeof(uint32_t), MSG_WAITALL);

			recibir_pcb(conexion_CPU_dispatch, proceso_recibido);

			io[0] = (float)aux;
			io[1] = ((float)time(NULL))*1000;
			char* string_pid = string_itoa(proceso_recibido->pid);

			sem_wait(mutex_cola_suspendido);

			dictionary_put(pid_handler, string_pid, (void*) io);
			queue_push(cola_de_io, proceso_recibido);

			sem_post(mutex_cola_suspendido);

			dictionary_put(process_state, string_pid, "blocked");

			sem_post(signal_a_io);
			break;

		case OPERACION_EXIT:

			recibir_pcb(conexion_CPU_dispatch, proceso_recibido);

			//2 sendv(memoria, borrar_pid, int, 0);
			//recv(memoria, borrar_pid, int, MSG_WAITALL); mensaje de que ya elimino la mem para proceder
			//sendv(proceso_recibido->socket, 1, int, 0); aviso a la consola que termino su proceso

			sem_post(sem_contador_multiprogramacion);
			break;

		case OPERACION_INTERRUPT:
			ejecutado = malloc(sizeof(pcb));
			recibir_pcb(conexion_CPU_dispatch, ejecutado);

			ejecutado->estimacion_siguiente = ((float)time(NULL)*1000) - ejecutado->estimacion_siguiente; //son detalles

			flag_respuesta_a_interupcion = 1;
			sem_post(binario_flag_interrupt);


			break;
		case CPU_LIBRE:

			flag_respuesta_a_interupcion = 2;
			sem_post(binario_flag_interrupt);


			break;
		}
	}

 return NULL;
}





void planificador_de_corto_plazo(){

	uint32_t interrupt = 22;
	flag_respuesta_a_interupcion =0;


	if (int_modo_planificacion == SRT){
		send(conexion_cpu_interrupt, &interrupt, sizeof(uint32_t), 0);
		sem_wait(binario_flag_interrupt);
		pcb* candidato_del_stack;
		switch(flag_respuesta_a_interupcion){
			case 1:{
				candidato_del_stack = algoritmo_srt();
				if(ejecutado->estimacion_siguiente <= candidato_del_stack){
					pasar_a_running(ejecutado);
					sem_wait(mutex_cola_ready);
					queue_push(cola_de_ready, candidato_del_stack);
					sem_post(mutex_cola_ready);
				}
				else{pasar_a_running(candidato_del_stack);
				sem_wait(mutex_cola_ready);
				queue_push(cola_de_ready, ejecutado);
				sem_post(mutex_cola_ready);
				}

				break;
				}
			case 2:{
				candidato_del_stack = algoritmo_srt();
				pasar_a_running(candidato_del_stack);
				break;
				}
			}


	}else if (int_modo_planificacion == FIFO){
		sem_wait(mutex_cola_ready);
		pcb* proceso_a_ejecutar = (pcb*) queue_pop(cola_de_ready);
		sem_post(mutex_cola_ready);
		pasar_a_running(proceso_a_ejecutar);
	}
}



pcb* algoritmo_srt(){
	t_queue* aux = queue_create();
	int primer_proceso;
	int tamanio;
	int iterador = 1;

	int pid_menor;

	pcb* auxiliar1 = malloc(sizeof(pcb));
	pcb* auxiliar2 = malloc(sizeof(pcb));


	sem_wait(mutex_cola_ready);
	tamanio = queue_size(cola_de_ready);

	auxiliar1 = queue_pop(cola_de_ready);

	primer_proceso = auxiliar1->pid;

	queue_push(aux, (void*) auxiliar1);

	while(iterador < tamanio){
		iterador++;
		auxiliar2 = queue_pop(cola_de_ready);
		queue_push(aux, (void*) auxiliar2);

		if(auxiliar1->estimacion_siguiente <= auxiliar2->estimacion_siguiente){
			pid_menor = auxiliar1->pid;
		}
		else {
			pid_menor = auxiliar2->pid;
			auxiliar1 = auxiliar2;
		}
	}
		iterador =1;



		while(iterador <= tamanio){
			auxiliar2 = queue_peek(aux);
			if(auxiliar2->pid == pid_menor && auxiliar2->pid == primer_proceso){
				queue_pop(aux);
				cola_de_ready = aux;
				return auxiliar1; //termino
			}
			else if(auxiliar2->pid == pid_menor){
				queue_pop(aux);
				auxiliar2 = queue_peek(aux);

			}
			if(auxiliar2->pid == primer_proceso){
				cola_de_ready = aux;
				return auxiliar1;

			}else{
				auxiliar2 = queue_pop(aux);
				queue_push(aux,(void*) auxiliar2);
			}
		}

		return auxiliar1;

}



