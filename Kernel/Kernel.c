#include "utilsKernel.h"
uint32_t resultOk = 0;

void* recibiendo(void* , t_config*, pcb*);

sem_t* contador_de_listas_esperando_para_estar_en_ready; //indicar al plani largo cuando el grado de multiprogramacion decrece y puede poner uno en ready

sem_t* sem_contador_multiprogramacion;

sem_t* mutex_lista_ready; //mutex para trabajar sobre la lista de ready

sem_t* mutex_grado_de_multiprogramacion; //mutex para trabajar sobre la varianle global "grado de multiprogramacion"

sem_t* mutex_cola_new; //mutex para trabajar sobre la lista de new

t_queue* cola_procesos_nuevos;

int** pid_handler;


//funcion_pasar_a_ready(){
//	wait(contador_de_listas_esperando_para_estar_en_ready)
//	wait(sem_contador_multiprogramacion)
//	che la lista de sus-ready tiene algo
//	else lista de new
//} sus-ready / new



void* escuchar_consola(int , pcb* , t_config* ,int);
int main(void) {
	cola_procesos_nuevos = queue_create();
	pid_handler = NULL;

	sem_init(contador_de_listas_esperando_para_estar_en_ready, 0, 0);

	sem_init(sem_contador_multiprogramacion, 0, 1); //este valor hay que sacarlo del config

	sem_init(mutex_lista_ready, 0, 1);

	sem_init(mutex_grado_de_multiprogramacion, 0, 1);

	sem_init(mutex_cola_new, 0, 1);




	t_config* config = config_create("/home/utnso/Documentos/tp-2022-1c-Club-Penguin/Kernel/kernel.config");
	logger = log_create("log.log", "Servidor", 1, LOG_LEVEL_DEBUG);
	char* ip_CPU = config_get_string_value(config, "IP_CPU");
	char* puerto_CPU = config_get_string_value(config, "PUERTO_CPU_DISPATCH");


	char* ip_kernel = config_get_string_value(config, "IP_KERNEL");
	char* puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
	int socket_kernel_escucha = iniciar_servidor(ip_kernel, puerto_escucha);//ip kernel unica, pero el puerto es el del escucha,
	log_info(logger, "Servidor listo para recibir al cliente");
	int conexion = crear_conexion(ip_CPU, puerto_CPU);

	void* _f_aux_escucha_consola(void *socket_kernel_escucha){
		escuchar_consola(*(int*)socket_kernel_escucha, cola_procesos_nuevos, config, conexion);
		return NULL;
	}
	pthread_t socket_escucha_consola;
	pthread_create(&socket_escucha_consola, NULL, _f_aux_escucha_consola, (void*) &socket_kernel_escucha );

	pthread_t pasar_a_ready;
	pthread_create(&pasar_a_ready, NULL, Template, (void*) &hayquever );

	pthread_join(socket_escucha_consola, NULL);

	return EXIT_SUCCESS;
}



uint32_t proximo_pid = 0;

// Inicializamos el kernel como servidor, donde cada hilo va a poder realizar una conexion distinta

void* recibiendo(void* input, t_config* config){
	pcb* nuevo_pcb = malloc(sizeof(pcb));


	int* cliente_fd = (int *) input;
	void* buffer_instrucciones;
	int codigo_de_paquete = recibir_operacion(*cliente_fd);
	switch(codigo_de_paquete) {
	case OPERACION_ENVIO_INSTRUCCIONES:
		buffer_instrucciones = recibir_instrucciones(*cliente_fd);
//----------------------NOTA--parte de esto deberia ir en teoria en la planificacion de largo plazo no en recibir pero por ahroa queda asi
		crear_header(proximo_pid, buffer_instrucciones, config, nuevo_pcb);

		sem_wait(mutex_cola_new);
		queue_push(cola_procesos_nuevos, (void*) nuevo_pcb);
		pid_handler = realloc(pid_handler, sizeof(pid_handler) + sizeof(cliente_fd));
		sem_post(mutex_cola_new);

		proximo_pid++;
		send(*cliente_fd, &resultOk, sizeof(uint32_t), 0);

	}
	return NULL;
}
void* escuchar_consola(int socket_kernel_escucha, t_config* config, int conexion){
	while(1){//desde aca
		pthread_t thread_type;
		int cliente_fd = esperar_cliente(socket_kernel_escucha);
		void* _f_aux(void* cliente_fd){
			recibiendo(cliente_fd, config);// post recibiendo perdemos las cosas






			//IR A RUNNING
			t_buffer* pcb_serializado = malloc(sizeof(t_buffer));
			pcb_serializado = serializar_header(cola_procesos_nuevos); // por ahora aca, falta semaforos bien hechos cambiar para que no retorne nada a ver si se soluciona el error
			empaquetar_y_enviar(pcb_serializado, conexion, OPERACION_ENVIO_PCB);
			return NULL;
		}
		pthread_create(&thread_type, NULL, _f_aux, (void*) &cliente_fd );
	}//hasta aca se encarga de escuchar consolas,
 return NULL;
}
