#include "utilsKernel.h"
#include<pthread.h>
uint32_t resultOk = 0;

void* recibiendo(void* input){
	int* cliente_fd = (int *) input;
	t_list* lista_ins;
	uint32_t tamanio_en_memoria =0;
	while (1) {
		int codigo_de_Paquete = recibir_operacion(*cliente_fd);
		switch (codigo_de_Paquete) {
		case OPERACION_ENVIO_INSTRUCCIONES:
			lista_ins = recibir_instrucciones(*cliente_fd, &tamanio_en_memoria);
			printf("tamanio en mem: %d \n", tamanio_en_memoria);
			list_iterate(lista_ins, imprimir_instruccion);
			send(*cliente_fd, &resultOk, sizeof(uint32_t), 0);
			break;
		case -1:
			log_error(logger, "el cliente se desconecto. Terminando servidor");
			return EXIT_FAILURE;
		default:
			log_warning(logger,"Operacion desconocida. No quieras meter la pata");
			break;
		}
	}
	return 0;
}


int main(void) {
	logger = log_create("log.log", "Servidor", 1, LOG_LEVEL_DEBUG);
	int server_fd = iniciar_servidor();
	log_info(logger, "Servidor listo para recibir al cliente");

	while(1){
		pthread_t thread_type;
		int cliente_fd = esperar_cliente(server_fd);
		int thread = pthread_create(&thread_type, NULL, recibiendo, (void*) cliente_fd );
	}



	return EXIT_SUCCESS;
}
