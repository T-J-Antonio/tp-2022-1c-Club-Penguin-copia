#include "utilsCPU.h"



void* escuchar_kernel(int , t_config*);

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


void* escuchar_kernel(int socket_escucha_dispatch, t_config* config){
	int cliente_fd = esperar_cliente(socket_escucha_dispatch);
	int codigo_de_paquete = recibir_operacion(cliente_fd);
	pcb* recibido = malloc(sizeof(pcb));
		switch(codigo_de_paquete) {
		case OPERACION_ENVIO_PCB:
			recibir_pcb(cliente_fd, recibido);
			imprimir_pcb(recibido);

		}
	return NULL;
}


void ciclo_de_instruccion(pcb* en_ejecucion, t_config* config, int socket_escucha_dispatch){
	int tiempo_espera = config_get_int_value(config, "RETARDO_NOOP") / 1000;
	int tiempo_bloqueo = 0;
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
		send(socket_escucha_dispatch, &tiempo_bloqueo, sizeof(uint32_t), 0);
		//¿cómo le decimos al kernel que se bloquee por x tiempo?
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
		//podríamos mandarle PID + lo que hay que hacer, ya que hay múltiples hilos esperando pero un solo socket
		send(socket_escucha_dispatch, &PROCESO_FINALIZADO, sizeof(uint32_t), 0);
		//¿cómo le decimos al kernel que terminó?
		break;
	}

	//check interrupt
}




