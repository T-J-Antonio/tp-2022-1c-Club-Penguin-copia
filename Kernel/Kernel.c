#include "planificador.h"

int main(void) {
	pid_handler = dictionary_create();
	process_state = dictionary_create();

	proximo_pid = 0;
	resultOk = 0;

	sem_init(mutex_cola_new, 0, 1);
	sem_init(mutex_cola_sus_ready, 0, 1);
	sem_init(mutex_cola_ready, 0, 1);
	sem_init(contador_de_listas_esperando_para_estar_en_ready, 0, 0);
	sem_init(binario_flag_interrupt, 0, 0);
	sem_init(mutex_cola_suspendido, 0, 1);
	sem_init(signal_a_io, 0, 0);
	sem_init(dispositivo_de_io, 0, 1);


	cola_procesos_nuevos = queue_create();
	cola_procesos_sus_ready = queue_create();
	cola_de_ready = queue_create();
	cola_de_io = queue_create();


	t_config* config = config_create("/home/utnso/Documentos/tp-2022-1c-Club-Penguin/Kernel/kernel.config");
	logger = log_create("log.log", "Servidor", 1, LOG_LEVEL_DEBUG);

	char* ip_CPU = config_get_string_value(config, "IP_CPU");
	char* puerto_CPU = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
	char* modo_de_planificacion = config_get_string_value(config, "ALGORITMO_PLANIFICACION");

	if(strcmp(modo_de_planificacion, "FIFO") ==0)int_modo_planificacion = 21;
	if(strcmp(modo_de_planificacion, "SRT") ==0)int_modo_planificacion = 22;

	tiempo_de_espera_max = (float) config_get_int_value(config, "TIEMPO_MAXIMO_BLOQUEADO");


	int grado_de_multiprogramacion = config_get_int_value(config, "GRADO_MULTIPROGRAMACION");
	sem_init(sem_contador_multiprogramacion, 0, grado_de_multiprogramacion);

	char* ip_kernel = config_get_string_value(config, "IP_KERNEL");
	char* puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
	int socket_kernel_escucha = iniciar_servidor(ip_kernel, puerto_escucha);//ip kernel unica, pero el puerto es el del escucha,

	char* puerto_interrupt = config_get_string_value(config, "PUERTO_CPU_INTERRUPT");
	conexion_cpu_interrupt = iniciar_servidor(ip_CPU, puerto_interrupt);//conexion al puerto de cpu para interrumpir

	estimacion_inicial = (float) config_get_int_value(config, "ESTIMACION_INICIAL");
	alfa = (float) config_get_int_value(config, "ALFA");

	conexion_CPU_dispatch = crear_conexion(ip_CPU, puerto_CPU);

	void* _f_aux_escucha_consola(void *socket_kernel_escucha){
		escuchar_consola(*(int*)socket_kernel_escucha, config);
		return NULL;
	}



	pthread_t socket_escucha_consola;
	pthread_create(&socket_escucha_consola, NULL, _f_aux_escucha_consola, (void*) &socket_kernel_escucha );

	pthread_t pasar_a_ready;
	pthread_create(&pasar_a_ready, NULL, funcion_pasar_a_ready, NULL );

	pthread_t recibir_procesos_por_cpu;
	pthread_create(&recibir_procesos_por_cpu, NULL, recibir_pcb_de_cpu, NULL );


	pthread_join(socket_escucha_consola, NULL);

	return EXIT_SUCCESS;
}
