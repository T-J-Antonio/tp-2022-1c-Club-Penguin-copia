#include "utilsKernel.h"
uint32_t resultOk = 0;

void* recibiendo(void* , t_config*, void*);
void* escuchar_consola(int , void* , t_config* ,int);

uint32_t proximo_pid = 0;

// Inicializamos el kernel como servidor, donde cada hilo va a poder realizar una conexion distinta

int main(void) {
	void* cola_procesos_nuevos;
	t_config* config = config_create("/home/utnso/Documentos/tp-2022-1c-Club-Penguin/Kernel/kernel.config");
	logger = log_create("log.log", "Servidor", 1, LOG_LEVEL_DEBUG);
	char* ip_CPU = config_get_string_value(config, "IP_CPU");
	char* puerto_CPU = config_get_string_value(config, "PUERTO_CPU_DISPATCH");


	char* ip_kernel = config_get_string_value(config, "KERNEL_IP");
	char* puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
	int socket_kernel_escucha = iniciar_servidor(ip_kernel, puerto_escucha);//ip kernel unica, pero el puerto es el del escucha,
	log_info(logger, "Servidor listo para recibir al cliente");
	int conexion = crear_conexion(ip_CPU, puerto_CPU);

	pthread_t socket_escucha_consola;
	void* _f_aux_escucha_consola(void *socket_kernel_escucha){
		escuchar_consola(*(int*)socket_kernel_escucha, cola_procesos_nuevos, config, conexion);
		return NULL;
	}
	int thread = pthread_create(&socket_escucha_consola, NULL, _f_aux_escucha_consola, (void*) &socket_kernel_escucha );

	pthread_join(socket_escucha_consola, NULL);

	return EXIT_SUCCESS;
}


void* recibiendo(void* input, t_config* config, void* cola_procesos_nuevos){

	int* cliente_fd = (int *) input;
	void* buffer_instrucciones;
	pcb* header_proceso = malloc(sizeof(pcb));
	while (1){
		int codigo_de_paquete = recibir_operacion(*cliente_fd);
		switch(codigo_de_paquete) {
		case OPERACION_ENVIO_INSTRUCCIONES:
			buffer_instrucciones = recibir_instrucciones(*cliente_fd);
	//----------------------NOTA--parte de esto deberia ir en teoria en la planificacion de largo plazo no en recibir pero por ahroa queda asi
			header_proceso = crear_header(proximo_pid, buffer_instrucciones, config);
			cola_procesos_nuevos = (void*)header_proceso; // aca deberiamos agregar a la cola
			proximo_pid++;
			send(*cliente_fd, &resultOk, sizeof(uint32_t), 0);
			break;
		}
	}
	return NULL;
}
void* escuchar_consola(int socket_kernel_escucha, void* cola_procesos_nuevos, t_config* config, int conexion){
	while(1){//desde aca
		pthread_t thread_type;
		int cliente_fd = esperar_cliente(socket_kernel_escucha);
		printf("creo\n");
		void* _f_aux(void* cliente_fd){
			recibiendo(cliente_fd, config, cola_procesos_nuevos);
			void* pcb_serializado = serializar_header((pcb*)cola_procesos_nuevos); // por ahora aca, falta semaforos bien hechos
			empaquetar_y_enviar(pcb_serializado, conexion, 2);
			return NULL;
		}
		int thread = pthread_create(&thread_type, NULL, _f_aux, (void*) &cliente_fd );
	}//hasta aca se encarga de escuchar consolas,
 return NULL;
}
