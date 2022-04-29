#include "utilsKernel.h"
uint32_t resultOk = 0;

void* recibiendo(void*);
void* escuchar_consola(int);

uint32_t proximo_pid = 0;

int main(void) {
	t_config* config = config_create("/home/utnso/Documentos/tp-2022-1c-Club-Penguin/Kernel/kernel.config");
	logger = log_create("log.log", "Servidor", 1, LOG_LEVEL_DEBUG);

	char* ip_kernel = config_get_string_value(config, "KERNEL_IP");
	char* puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
	int socket_kernel_escucha = iniciar_servidor(ip_kernel, puerto_escucha);//ip kernel unica, pero el puerto es el del escucha,
	log_info(logger, "Servidor listo para recibir al cliente");

	pthread_t socket_escucha_consola;
	int thread = pthread_create(&socket_escucha_consola, NULL, escuchar_consola, (void*) &socket_kernel_escucha );

	return EXIT_SUCCESS;
}

void* recibiendo(void* input, t_config* config){

	int* cliente_fd = (int *) input;
	void* buffer_instrucciones;
	pcb* header_proceso = malloc(sizeof(pcb));
	while (1) {
		int codigo_de_paquete = recibir_operacion(*cliente_fd);
		switch (codigo_de_paquete) {
		case OPERACION_ENVIO_INSTRUCCIONES:
			buffer_instrucciones = recibir_instrucciones(*cliente_fd);
	//----------------------NOTA -- parte de esto deberia ir en teoria en la planificacion de largo plazo no en recibir pero por ahroa queda asi
			header_proceso = crear_header(proximo_pid, buffer_instrucciones, config);
			proximo_pid++;
			printf("tamaño en mem: %d\n", header_proceso->tamanio_en_memoria);
			printf("próxima estimación: %f\n", header_proceso->estimacion_siguiente);
			printf("PID: %d\n", header_proceso->pid);
			send(*cliente_fd, &resultOk, sizeof(uint32_t), 0);
			break;
		}
	}
	return NULL;
}
void* escuchar_consola(int socket_kernel_escucha){
	while(1){//desde aca
		pthread_t thread_type;
		int cliente_fd = esperar_cliente(socket_kernel_escucha);
		printf("creo\n");
		int thread = pthread_create(&thread_type, NULL, recibiendo, (void*) &cliente_fd );
	}//hasta aca se encarga de escuchar consolas,
 return NULL;

