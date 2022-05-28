#include "utilsKernel.h"


uint32_t proximo_pid = 0;
uint32_t resultOk = 0;

float estimacion_inicial;
float alfa;

int conexion_cpu_interrupt;

float tiempo_inicio;


pcb* algoritmo_srt();
int main(void) {

	pid_handler = dictionary_create();

	sem_init(mutex_cola_new, 0, 1);

	sem_init(mutex_cola_sus_ready, 0, 1);

	sem_init(mutex_cola_ready, 0, 1);

	sem_init(mutex_grado_de_multiprogramacion, 0, 1);

	sem_init(contador_de_listas_esperando_para_estar_en_ready, 0, 0);


	cola_procesos_nuevos = queue_create();
	cola_procesos_sus_ready = queue_create();
	cola_de_ready = queue_create();


	t_config* config = config_create("/home/utnso/Documentos/tp-2022-1c-Club-Penguin/Kernel/kernel.config");
	logger = log_create("log.log", "Servidor", 1, LOG_LEVEL_DEBUG);

	char* ip_CPU = config_get_string_value(config, "IP_CPU");
	char* puerto_CPU = config_get_string_value(config, "PUERTO_CPU_DISPATCH");

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

	pthread_t recibir_procesos_por_io;
	pthread_create(&recibir_procesos_por_io, NULL, funcion_pasar_a_ready, NULL );


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
		char* string_pid = string_itoa(proximo_pid);

		queue_push(cola_procesos_nuevos, (void*) nuevo_pcb);
		dictionary_put(pid_handler, string_pid, (void*) cliente_fd);

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
	pcb_serializado = serializar_header(proceso_ready); // por ahora aca, falta semaforos bien hechos cambiar para que no retorne nada a ver si se soluciona el error
	empaquetar_y_enviar(pcb_serializado, conexion_CPU_dispatch, OPERACION_ENVIO_PCB);
	//vamos a tener que hacer un free

}


void recibir_pcb_de_cpu( t_config* config, pcb* pcb_ejecutado){
	void* buffer_instrucciones;
	int codigo_de_paquete = recibir_operacion(conexion_CPU_dispatch);
	switch(codigo_de_paquete) {
	case 22:  //debaria ser el caso de recibir interupt
		buffer_instrucciones = recibir_instrucciones(*conexion_CPU_dispatch);// recibir pcb funcion ya definida en cpu, hay que usar esa
	}
}





void planificador_de_corto_plazo(){

	pcb* ejecutado = malloc(sizeof(pcb));

	uint32_t interrupt = 22;

	send(conexion_cpu_interrupt, &interrupt, sizeof(uint32_t), 0);

	//sabemos que vamos a recibir por parte de la cpu, el pcb que se estaba ejecutando, NO SIEMPRE

	float tiempo_de_paro_por_interrupcion = ((float)time(NULL)*1000);
	//restar el tiempo que hizo de lo estimado.

	// en caso de que haya uno mas corto del que se estaba ejecutando, el mismo debera volver a ready;


	//if(se fue antes, seguir sin tenerlo en cuenta

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



