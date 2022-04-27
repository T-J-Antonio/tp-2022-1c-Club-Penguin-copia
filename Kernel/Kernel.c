#include "utilsKernel.h"
uint32_t resultOk = 0;

void* recibiendo(void*);

uint32_t proximo_pid = 0;

int main(void) {
	logger = log_create("log.log", "Servidor", 1, LOG_LEVEL_DEBUG);
	int server_fd = iniciar_servidor();
	log_info(logger, "Servidor listo para recibir al cliente");

	while(1){
		pthread_t thread_type;
		int cliente_fd = esperar_cliente(server_fd);
		printf("creo\n");
		int thread = pthread_create(&thread_type, NULL, recibiendo, (void*) &cliente_fd );
	}



	return EXIT_SUCCESS;
}

void* recibiendo(void* input){
	t_config* config = config_create("/home/utnso/Documentos/tp-2022-1c-Club-Penguin/Kernel/kernel.config");
	int* cliente_fd = (int *) input;
	t_list* lista_ins;
	uint32_t tamanio_en_memoria =0;
	pcb* header_proceso = malloc(sizeof(pcb));
	while (1) {
		int codigo_de_paquete = recibir_operacion(*cliente_fd);
		switch (codigo_de_paquete) {
		case OPERACION_ENVIO_INSTRUCCIONES:
			lista_ins = recibir_instrucciones(*cliente_fd, &tamanio_en_memoria);
			header_proceso = crear_header(proximo_pid, tamanio_en_memoria, lista_ins, config);
			proximo_pid++;
			printf("tamaño en mem: %d\n", header_proceso->tamanio_en_memoria);
			list_iterate(header_proceso->instrucciones, imprimir_instruccion);
			printf("próxima estimación: %f\n", header_proceso->estimacion_siguiente);
			printf("PID: %d\n", header_proceso->pid);
			send(*cliente_fd, &resultOk, sizeof(uint32_t), 0);
			break;
		}
	}
	return NULL;
}
