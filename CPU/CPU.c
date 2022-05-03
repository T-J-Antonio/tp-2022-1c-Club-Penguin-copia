#include"utilsCPU.h"


void* escuchar_kernel(int , t_config*);
void imprimir_pcb(pcb* );
void imprimir_instruccion(void * );
pcb* recibir_pcb(int );

int main(){
	t_config* config = config_create("/home/utnso/Documentos/tp-2022-1c-Club-Penguin/CPU/CPU.config");

	logger = log_create("log.log", "Servidor", 1, LOG_LEVEL_DEBUG);

	char* ip_cpu = config_get_string_value(config, "IP_CPU");
	char* puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
	int socket_cpu_escucha = iniciar_servidor(ip_cpu, puerto_escucha);//ip kernel unica, pero el puerto es el del escucha,

	log_info(logger, "CPU listo para recibir al pebete");
	pthread_t socket_escucha_dispatch;

	void* _f_aux_escucha_kernel(void *socket_cpu_escucha){
		escuchar_kernel(*(int*)socket_cpu_escucha, config);
		return NULL;
	}
	int thread = pthread_create(&socket_escucha_dispatch, NULL, _f_aux_escucha_kernel, (void*) &socket_cpu_escucha );

	pthread_join(socket_escucha_dispatch, NULL);
	return 0;
}


pcb* recibir_pcb(int socket_cliente){
	int size;
	uint32_t offset = 0;
	uint32_t offset_instrucciones = 0;
	void * buffer;
	t_list* instrucciones = list_create();
	pcb *pcb_recibido = malloc(sizeof(pcb));

	buffer = recibir_buffer(&size, socket_cliente); // almacena en size el tamanio de todo el buffer y ademas guarda en buffer todo el stream
	memcpy(&pcb_recibido->pid, buffer, sizeof(uint32_t));
	offset+=sizeof(uint32_t);

	memcpy(&pcb_recibido->tamanio_en_memoria, buffer + offset, sizeof(uint32_t));
	offset+=sizeof(uint32_t);

	memcpy(&pcb_recibido->tamanio_stream_instrucciones, buffer + offset, sizeof(uint32_t));
	offset+=sizeof(uint32_t);

	while(offset_instrucciones < pcb_recibido->tamanio_stream_instrucciones){
		instruccion *ins_recibida = malloc(sizeof(instruccion));
		ins_recibida->parametros = NULL;
		memcpy(&ins_recibida->cod_op, buffer + offset + offset_instrucciones, sizeof(uint32_t));
		offset_instrucciones+=sizeof(uint32_t);
		memcpy(&ins_recibida->tam_param, buffer + offset + offset_instrucciones, sizeof(uint32_t));
		offset_instrucciones+=sizeof(uint32_t);


		//lista de parametros
		if(ins_recibida->tam_param){
			ins_recibida->parametros = malloc(ins_recibida->tam_param);
			memcpy(ins_recibida->parametros, buffer + offset + offset_instrucciones, ins_recibida->tam_param);
			offset_instrucciones+=ins_recibida->tam_param;
		}

		list_add(instrucciones, ins_recibida);
	}

	pcb_recibido->instrucciones = (void *) instrucciones; // podemos cambiar la def de pcb dentro del cpu para no tener que castear a void, en realidad seria mas logico usar queue asi usamos pop y borra la ins usada

	offset += offset_instrucciones;

	memcpy(&pcb_recibido->program_counter, buffer + offset, sizeof(uint32_t));
	offset+=sizeof(uint32_t);

	memcpy(&pcb_recibido->tamanio_paginas, buffer + offset, sizeof(uint32_t));
	offset+=sizeof(uint32_t);

	memcpy(pcb_recibido->tabla_paginas, buffer + offset, pcb_recibido->tamanio_paginas);
	offset+=pcb_recibido->tamanio_paginas;

	memcpy(&pcb_recibido->estimacion_siguiente, buffer + offset, sizeof(float));
	offset+=sizeof(float);

	free(buffer);

	return pcb_recibido;
}

void imprimir_instruccion(void * var){
	instruccion *alo = (instruccion *) var;

	printf("codigo de operacion: %d\n", alo->cod_op);
	uint32_t i = 0;
	uint32_t cant = alo->tam_param/sizeof(uint32_t);

	while(i<cant){
		printf("parametro: %d\n", alo->parametros[i]);
		i++;
	}
}



void imprimir_pcb(pcb* recepcion){
	printf("pid: %d\n", recepcion->pid);
	printf("tamanio_en_memoria: %d\n", recepcion->tamanio_en_memoria);
	printf("tamanio_stream_instrucciones: %d\n", recepcion->tamanio_stream_instrucciones);
	list_iterate((t_list*)recepcion->instrucciones, imprimir_instruccion);
	printf("program_counter: %d\n", recepcion->program_counter);
	printf("tamanio_paginas: %d\n", recepcion->tamanio_paginas);
	printf("estimacion_siguiente: %f\n", recepcion->estimacion_siguiente);

}




void* escuchar_kernel(int socket_escucha_dispatch, t_config* config){
	while(1){//desde aca
		pthread_t thread_type;
		int cliente_fd = esperar_cliente(socket_escucha_dispatch);
		printf("creo\n");
		void* _f_aux(void* cliente_fd){
			pcb* recepcion = malloc(sizeof(pcb));
			recepcion = recibir_pcb(*(int*)cliente_fd);
			imprimir_pcb(recepcion);
			return NULL;
		}
		int thread = pthread_create(&thread_type, NULL, _f_aux, (void*) &cliente_fd );
	}//hasta aca se encarga de escuchar consolas,
 return NULL;
}
