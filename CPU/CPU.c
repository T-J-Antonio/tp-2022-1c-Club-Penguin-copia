#include "utilsCPU.h"
int hay_interrupciones = 0;
int cliente_dispatch;
uint32_t cpu_libre = CPU_LIBRE;
int tiempo_espera;
uint32_t tamanio_pagina;
uint32_t cant_entradas_por_tabla;

void* escuchar_kernel(int, t_config*);
void* escuchar_interrupciones(int, t_config*);
void ciclo_de_instruccion(pcb*, int);


int main(){
	t_config* config = config_create("/home/utnso/Documentos/tp-2022-1c-Club-Penguin/CPU/CPU.config");

	logger = log_create("log.log", "Servidor", 1, LOG_LEVEL_DEBUG);

	char* ip_cpu = config_get_string_value(config, "IP_CPU");
	char* puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
	char* puerto_escucha_interrupt = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");
	tiempo_espera = config_get_int_value(config, "RETARDO_NOOP") / 1000;
	algoritmo_reemplazo_TLB = algoritmo_reemplazo_to_int(config_get_string_value(config, "REEMPLAZO_TLB"));

/* ====== Aca iria la conexion con memoria, para que me pase ademas el tamanio_pagina y cant_entradas_por_tabla ====== */

	TLB = queue_create();

	int socket_cpu_escucha = iniciar_servidor(ip_cpu, puerto_escucha);//ip kernel unica, pero el puerto es el del escucha,
	int socket_cpu_interrupt = iniciar_servidor(ip_cpu, puerto_escucha_interrupt);

	log_info(logger, "CPU listo para recibir al pebete");
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
			printf("se cayo el socket\n");
			return NULL;
		}
		sem_wait(&mutex_interrupciones);
		printf("llego int\n");
		sem_getvalue(&cpu_en_running, &estado_cpu);
		switch(codigo_de_paquete) {
		case OPERACION_INTERRUPT:
			printf("espere el semaforo\n");
			if(!estado_cpu){
				printf("lugar deseado\n");
				int cant = send(cliente_dispatch, &cpu_libre, sizeof(uint32_t), 0);
				printf("cantidad int: %d",cant);
				printf("hice el send");
			}else{
				printf("lugar no deseado\n");
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
			printf("se cayo el socket dispatch\n");
			return NULL;
		}
		sem_post(&cpu_en_running);
		pcb* recibido = malloc(sizeof(pcb));
		switch(codigo_de_paquete) {
		case OPERACION_ENVIO_PCB:
			recibir_pcb(socket_escucha_dispatch, recibido);
			for(int i = recibido->program_counter; i < list_size(recibido->instrucciones); i++){
				ciclo_de_instruccion(recibido, socket_escucha_dispatch);//aca cambio el cliente por socket ojo
				instruccion* ejecutada = list_get(recibido->instrucciones, recibido->program_counter - 1);
				//si la instrucción que acabo de ejecutar es I_O o EXIT, no debo seguir el ciclo
				if(ejecutada->cod_op == I_O || ejecutada->cod_op == EXIT){
					liberar_pcb(recibido);
					sem_wait(&cpu_en_running);
					break;
				}
			}
			break;
		}
	}
	return NULL;
}


void ciclo_de_instruccion(pcb* en_ejecucion, int socket_escucha_dispatch){
	int tiempo_bloqueo = 0;
	t_buffer* pcb_actualizado = NULL;

	//fetch
	instruccion* a_ejecutar = list_get(en_ejecucion->instrucciones, en_ejecucion->program_counter);

	//decode
	int cod_op = a_ejecutar->cod_op;

	//fetch operands
	if(cod_op == COPY) {
		//próximamente
	}

	//execute
	switch(cod_op){
	case NO_OP:
		sleep(tiempo_espera);
		++(en_ejecucion->program_counter);
		break;

	case I_O:
		++(en_ejecucion->program_counter);
		tiempo_bloqueo = a_ejecutar->parametros[0];
		pcb_actualizado = serializar_header(en_ejecucion);
		empaquetar_y_enviar_i_o(pcb_actualizado, socket_escucha_dispatch, OPERACION_IO, tiempo_bloqueo);
		break;

	case READ:
		//próximamente
		// obtener_direccion_fisica(direccion_logica, en_ejecucion->tabla_paginas);
		// si no está en TLB:
		// hacer cuentas y devolver
		break;
	case WRITE:
		//próximamente
		// obtener_direccion_fisica(direccion_logica, en_ejecucion->tabla_paginas);
		// si no está en TLB:
		// hacer cuentas y devolver
		break;
	case COPY:
		//próximamente
		// obtener_direccion_fisica(direccion_logica, en_ejecucion->tabla_paginas);
		// si no está en TLB:
		// hacer cuentas y devolver
		// Copy va a hacer dos accesos a memoria, primero para leer el dato de una direccion, 
		// y despues para escribir el dato en la otra direccion
		break;

	case EXIT:
		++(en_ejecucion->program_counter);
		pcb_actualizado = serializar_header(en_ejecucion);
		empaquetar_y_enviar(pcb_actualizado, socket_escucha_dispatch, OPERACION_EXIT);
		break;
	}

	//check interrupt

	sem_wait(&mutex_interrupciones);
	if(hay_interrupciones){

		if(cod_op == I_O || cod_op == EXIT)
			send(socket_escucha_dispatch, &cpu_libre, sizeof(uint32_t), 0);
		else{
			pcb_actualizado = serializar_header(en_ejecucion);
			empaquetar_y_enviar(pcb_actualizado, socket_escucha_dispatch, RESPUESTA_INTERRUPT);
		}
		hay_interrupciones--;
	}
	sem_post(&mutex_interrupciones);

}


