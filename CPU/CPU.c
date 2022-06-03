#include "utilsCPU.h"
int hay_interrupciones = 0;
sem_t mutex_interrupciones;

void* escuchar_kernel(int, t_config*);
void* escuchar_interrupciones(int, t_config*);
void ciclo_de_instruccion(pcb*, t_config*, int);

int main(){
	t_config* config = config_create("/home/utnso/Documentos/tp-2022-1c-Club-Penguin/CPU/CPU.config");

	logger = log_create("log.log", "Servidor", 1, LOG_LEVEL_DEBUG);

	char* ip_cpu = config_get_string_value(config, "IP_CPU");
	char* puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
	char* puerto_escucha_interrupt = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");

	int socket_cpu_escucha = iniciar_servidor(ip_cpu, puerto_escucha);//ip kernel unica, pero el puerto es el del escucha,
	int socket_cpu_interrupt = iniciar_servidor(ip_cpu, puerto_escucha_interrupt);

	log_info(logger, "CPU listo para recibir al pebete");
	pthread_t socket_escucha_dispatch;
	pthread_t socket_escucha_interrupt;

	sem_init(&mutex_interrupciones, 0, 1);

	void* _f_aux_escucha_kernel(void *socket_cpu_escucha){
		escuchar_kernel(*(int*)socket_cpu_escucha, config);
		return NULL;
	}

	void* _f_aux_escucha_interrupt(void* socket_cpu_interrupt){
		escuchar_interrupciones(*(int*)socket_cpu_interrupt, config);
		return NULL;
	}

	pthread_create(&socket_escucha_dispatch, NULL, _f_aux_escucha_kernel, (void*) &socket_cpu_escucha);
	pthread_join(socket_escucha_dispatch, NULL);

	pthread_create(&socket_escucha_interrupt, NULL, _f_aux_escucha_interrupt, (void*) &socket_cpu_interrupt);
	pthread_join(socket_escucha_interrupt, NULL);
	return 0;
}

void* escuchar_interrupciones(int socket_escucha_interrupciones, t_config* config){
	int cliente_fd = esperar_cliente(socket_escucha_interrupciones); // ¿va adentro o afuera del while?
	while(1){
		int codigo_de_paquete = recibir_operacion(cliente_fd);
		switch(codigo_de_paquete) {
		case OPERACION_INTERRUPT:
			sem_wait(&mutex_interrupciones);
			hay_interrupciones++;
			sem_post(&mutex_interrupciones);
			break;
		}
	}
	return NULL;
}


void* escuchar_kernel(int socket_escucha_dispatch, t_config* config){
	int cliente_fd = esperar_cliente(socket_escucha_dispatch); // ¿va adentro o afuera del while?
	while(1){ //permite escuchar instrucciones infinitamente en vez de una sola vez
		int codigo_de_paquete = recibir_operacion(cliente_fd);
		pcb* recibido = malloc(sizeof(pcb));
		switch(codigo_de_paquete) {
		case OPERACION_ENVIO_PCB:
			recibir_pcb(cliente_fd, recibido);
			for(int i = recibido->program_counter; i < list_size(recibido->instrucciones); i++){
				ciclo_de_instruccion(recibido, config, cliente_fd);//aca cambio el cliente por socket ojo
				instruccion* ejecutada = list_get(recibido->instrucciones, recibido->program_counter - 1);
				//si la instrucción que acabo de ejecutar es I_O o EXIT, no debo seguir el ciclo
				if(ejecutada->cod_op == I_O || ejecutada->cod_op == EXIT) break;
			}
			break;
		}
	}
	return NULL;
}


void ciclo_de_instruccion(pcb* en_ejecucion, t_config* config, int socket_escucha_dispatch){
	int tiempo_espera = config_get_int_value(config, "RETARDO_NOOP") / 1000;
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
		empaquetar_y_enviar_i_o(pcb_actualizado, socket_escucha_dispatch, OPERACION_EXIT, tiempo_bloqueo);
		break;

	case READ:
		//próximamente
		break;
	case WRITE:
		//próximamente
		break;
	case COPY:
		//próximamente
		break;

	case EXIT:
		printf("llegue al exit\n");
		++(en_ejecucion->program_counter);
		pcb_actualizado = serializar_header(en_ejecucion);
		printf("actualice el header\n");
		empaquetar_y_enviar(pcb_actualizado, socket_escucha_dispatch, OPERACION_EXIT);
		printf("emp y env\n");
		break;
	}

	//check interrupt
	sem_wait(&mutex_interrupciones);
	if(hay_interrupciones){
		pcb_actualizado = serializar_header(en_ejecucion);
		if(cod_op == I_O || cod_op == EXIT)
			empaquetar_y_enviar(pcb_actualizado, socket_escucha_dispatch, CPU_LIBRE);
		else
			empaquetar_y_enviar(pcb_actualizado, socket_escucha_dispatch, RESPUESTA_INTERRUPT);
		hay_interrupciones--;
	}
	sem_post(&mutex_interrupciones);

}
