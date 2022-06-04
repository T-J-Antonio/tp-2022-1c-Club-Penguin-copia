#include "utilsMemoria.h"

void* escuchar(int , t_config* );
void* recibiendo(void* , t_config* );

int main(){
	t_config* config = config_create("/home/utnso/Documentos/tp-2022-1c-Club-Penguin/Memoria/memoria.config");

	logger = log_create("log.log", "Servidor", 1, LOG_LEVEL_DEBUG);

	char* ip_memoria = config_get_string_value(config, "IP_MEMORIA");
	char* puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");

	int socket_cpu_escucha = iniciar_servidor(ip_memoria, puerto_escucha);//ip kernel unica, pero el puerto es el del escucha,

	log_info(logger, "memoria lista para recibir al requests");
	pthread_t socket_escucha_dispatch;

	void* _f_aux_escucha(void* socket_cpu_interrupt){
		escuchar(*(int*)socket_cpu_interrupt, config);
		return NULL;
	}

	pthread_create(&socket_escucha_dispatch, NULL, _f_aux_escucha, (void*) &socket_cpu_escucha);

	pthread_join(socket_escucha_dispatch, NULL);
	return 0;
}

void* recibiendo(void* input, t_config* config){

	int* cliente_fd = (int *) input;
	int respuesta_generica = 10;
	int codigo_de_paquete = recibir_operacion(*cliente_fd);
	switch(codigo_de_paquete) {
	case MENSAJE_ESTANDAR:
		send(cliente_fd, &respuesta_generica, sizeof(int), 0);
		break;
	}
	return NULL;
}

void* escuchar(int socket_kernel_escucha, t_config* config){
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
