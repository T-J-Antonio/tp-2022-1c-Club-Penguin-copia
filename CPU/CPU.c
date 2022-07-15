#include "utilsCPU.h"

void* escuchar_kernel(int, t_config*);
void* escuchar_interrupciones(int, t_config*);
void ciclo_de_instruccion(pcb*, int);
int hay_interrupciones = 0;
int se_fue_por_int = 0;
uint32_t cpu_libre = 4;

int main(){
	t_config* config = config_create("/home/utnso/Documentos/tp-2022-1c-Club-Penguin/CPU/CPU.config");

	logger = log_create("CPU.log", "CPU", 1, LOG_LEVEL_DEBUG);

	char* ip_cpu = config_get_string_value(config, "IP_CPU");
	char* ip_memoria = config_get_string_value(config, "IP_MEMORIA");
	char* puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
	char* puerto_escucha_interrupt = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");
	char* puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
	cantidad_entradas_TLB = config_get_int_value(config, "ENTRADAS_TLB");
	tiempo_espera = config_get_int_value(config, "RETARDO_NOOP") / 1000;
	algoritmo_reemplazo_TLB = algoritmo_reemplazo_to_int(config_get_string_value(config, "REEMPLAZO_TLB"));

	conexion_memoria = crear_conexion(ip_memoria, puerto_memoria);
	recv(conexion_memoria, &tamanio_pagina, sizeof(uint32_t), MSG_WAITALL);
	recv(conexion_memoria, &cant_entradas_por_tabla, sizeof(uint32_t), MSG_WAITALL);

	TLB = queue_create();

	int socket_cpu_escucha = iniciar_servidor(ip_cpu, puerto_escucha);//ip kernel unica, pero el puerto es el del escucha,
	int socket_cpu_interrupt = iniciar_servidor(ip_cpu, puerto_escucha_interrupt);

	//log_info(logger, "CPU listo para recibir al pebete");
	pthread_t socket_escucha_dispatch;
	pthread_t socket_escucha_interrupt;

	sem_init(&mutex_interrupciones, 0, 1);
	sem_init(&cpu_en_running, 0, 0);

	void* _f_aux_escucha_kernel(void *socket_cpu_escucha){
		escuchar_kernel(*(int*)socket_cpu_escucha, config);
		return NULL;
	}

	void* _f_aux_escucha_interrupt(void* socket_cpu_interrupt){
		escuchar_interrupciones(*(int*)socket_cpu_interrupt, config);
		return NULL;
	}
	cliente_dispatch = esperar_cliente(socket_cpu_escucha);
	pthread_create(&socket_escucha_dispatch, NULL, _f_aux_escucha_kernel, (void*) &cliente_dispatch);
	pthread_create(&socket_escucha_interrupt, NULL, _f_aux_escucha_interrupt, (void*) &socket_cpu_interrupt);

	pthread_join(socket_escucha_dispatch, NULL);
	return 0;
}

void* escuchar_interrupciones(int socket_escucha_interrupciones, t_config* config){
	int cliente_fd = esperar_cliente(socket_escucha_interrupciones);

	while(1){
		int estado_cpu = 0;
		int codigo_de_paquete = recibir_operacion(cliente_fd);
		if (codigo_de_paquete == -1){
			log_info(logger, "Error de socket");
			return NULL;
		}
		sem_wait(&mutex_interrupciones);
		log_info(logger, "Recibida interrupción de CPU");
		sem_getvalue(&cpu_en_running, &estado_cpu);
		switch(codigo_de_paquete) {
		case OPERACION_INTERRUPT:
			//printf("espere el semaforo\n");
			if(!estado_cpu){
				//printf("lugar deseado\n");
				int cant = send(cliente_dispatch, &cpu_libre, sizeof(uint32_t), 0);
				//printf("cantidad int: %d",cant);
				//printf("hice el send");
			}else{
				//printf("lugar no deseado\n");
				hay_interrupciones++;
			}
			sem_post(&mutex_interrupciones);
			break;
		}
	}
	return NULL;
}


void* escuchar_kernel(int socket_escucha_dispatch, t_config* config){
	while(1){

		int codigo_de_paquete = recibir_operacion(socket_escucha_dispatch);
		if(codigo_de_paquete == -1){
			log_info(logger, "Error de socket");
			return NULL;
		}
		sem_post(&cpu_en_running);
		pcb* recibido = malloc(sizeof(pcb));
		switch(codigo_de_paquete) {
		case OPERACION_ENVIO_PCB:
			recibir_pcb(socket_escucha_dispatch, recibido);
			log_info(logger, "A cpu le llego el proceso: %d", recibido->pid);
			log_info(logger, "el proceso recibido tiene inicio: %ld y estimacion: %ld", recibido->timestamp_inicio_exe, recibido->estimacion_siguiente);
			for(int i = recibido->program_counter; i < list_size(recibido->instrucciones); i++){
				ciclo_de_instruccion(recibido, socket_escucha_dispatch);//aca cambio el cliente por socket ojo
				instruccion* ejecutada = list_get(recibido->instrucciones, recibido->program_counter - 1);
				//si la instrucción que acabo de ejecutar es I_O o EXIT, no debo seguir el ciclo
				if(ejecutada->cod_op == I_O || ejecutada->cod_op == EXIT || se_fue_por_int){
					liberar_pcb(recibido);
					sem_wait(&cpu_en_running);
					se_fue_por_int = 0;
					break;
				}
			}
			break;
		}
	}
	return NULL;
}


void ciclo_de_instruccion(pcb* proceso_en_ejecucion, int socket_escucha_dispatch){
	int tiempo_bloqueo = 0;
	t_buffer* pcb_actualizado = NULL;
	uint32_t direccion_fisica, dato_leido, nro_pagina;

	//fetch
	instruccion* instruccion_a_ejecutar = list_get(proceso_en_ejecucion->instrucciones, proceso_en_ejecucion->program_counter);

	//decode
	int cod_op = instruccion_a_ejecutar->cod_op;

	//fetch operands
	if(cod_op == COPY) {
		direccion_fisica = obtener_direccion_fisica(instruccion_a_ejecutar->parametros[1], proceso_en_ejecucion->tabla_paginas, proceso_en_ejecucion->pid); 
		nro_pagina = numero_pagina(instruccion_a_ejecutar->parametros[1]);
		dato_leido = leer_posicion_de_memoria(direccion_fisica, proceso_en_ejecucion->pid, nro_pagina);
	}

	//execute
	switch(cod_op){
	case NO_OP:
		log_info(logger, "el proceso: %d hizo noop", proceso_en_ejecucion->pid);
		sleep(tiempo_espera);
		++(proceso_en_ejecucion->program_counter);
		break;

	case I_O:
		++(proceso_en_ejecucion->program_counter);
		tiempo_bloqueo = instruccion_a_ejecutar->parametros[0];
		log_info(logger, "el proceso: %d se va a hacer io", proceso_en_ejecucion->pid);
		pcb_actualizado = serializar_header(proceso_en_ejecucion);
		empaquetar_y_enviar_i_o(pcb_actualizado, socket_escucha_dispatch, OPERACION_IO, tiempo_bloqueo);
		vaciar_tlb();
		break;

	case READ:
		direccion_fisica = obtener_direccion_fisica(instruccion_a_ejecutar->parametros[0], proceso_en_ejecucion->tabla_paginas, proceso_en_ejecucion->pid);
		nro_pagina = numero_pagina(instruccion_a_ejecutar->parametros[0]);
		dato_leido = leer_posicion_de_memoria(direccion_fisica, proceso_en_ejecucion->pid, nro_pagina);
		printf("El dato leído es: %d\n", dato_leido);
		++(proceso_en_ejecucion->program_counter);
		break;
	case WRITE:
		direccion_fisica = obtener_direccion_fisica(instruccion_a_ejecutar->parametros[0], proceso_en_ejecucion->tabla_paginas, proceso_en_ejecucion->pid);
		nro_pagina = numero_pagina(instruccion_a_ejecutar->parametros[0]);
		escribir_en_posicion_de_memoria(direccion_fisica, instruccion_a_ejecutar->parametros[1], proceso_en_ejecucion->pid, nro_pagina);
		++(proceso_en_ejecucion->program_counter);
		break;
	case COPY:
		// En fetch operands obtuvimos la direccion fisica con la direccion_logica_origen y despues leimos lo que esta
		// en esa direccion fisica; ahora obtenemos la direccion fisica de la direccion_logica_destino y despues 
		// escribimos el dato que leimos en esa direccion
		direccion_fisica = obtener_direccion_fisica(instruccion_a_ejecutar->parametros[0], proceso_en_ejecucion->tabla_paginas, proceso_en_ejecucion->pid);
		nro_pagina = numero_pagina(instruccion_a_ejecutar->parametros[0]);
		escribir_en_posicion_de_memoria(direccion_fisica, dato_leido, proceso_en_ejecucion->pid, nro_pagina);
		++(proceso_en_ejecucion->program_counter);
		break;

	case EXIT:
		++(proceso_en_ejecucion->program_counter);
		log_info(logger, "finaliza el proceso: %d", proceso_en_ejecucion->pid);
		pcb_actualizado = serializar_header(proceso_en_ejecucion);
		empaquetar_y_enviar(pcb_actualizado, socket_escucha_dispatch, OPERACION_EXIT);
		vaciar_tlb();
		break;
	}

	//check interrupt

	sem_wait(&mutex_interrupciones);
	if(hay_interrupciones){

		if(cod_op == I_O || cod_op == EXIT){
			send(socket_escucha_dispatch, &cpu_libre, sizeof(uint32_t), 0);
			log_info(logger, "llego int pero estoy libre");
		}
		else{
			log_info(logger, "llego int y desalojo al proceso: %d", proceso_en_ejecucion->pid);
			pcb_actualizado = serializar_header(proceso_en_ejecucion);
			empaquetar_y_enviar(pcb_actualizado, socket_escucha_dispatch, RESPUESTA_INTERRUPT);
			vaciar_tlb();
			se_fue_por_int = 1;
		}
		hay_interrupciones--;
	}
	sem_post(&mutex_interrupciones);

}


