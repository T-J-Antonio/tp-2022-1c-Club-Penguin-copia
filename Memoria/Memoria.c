#include "utilsMemoria.h"

void* escuchar(int, t_config* );
void* recibiendo(void*, t_config* );
void iniciar_estructuras();

int tam_memoria;
int tam_pagina;
int entradas_por_tabla;
int retardo_swap;

t_list* lista_de_tablas_de_primer_nivel;
t_list* lista_de_tablas_de_segundo_nivel;

int main(){
	t_config* config = config_create("/home/utnso/Documentos/tp-2022-1c-Club-Penguin/Memoria/memoria.config");
	logger = log_create("log.log", "Servidor", 1, LOG_LEVEL_DEBUG);

	char* ip_memoria = config_get_string_value(config, "IP_MEMORIA");
	char* puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");

	tam_memoria = config_get_int_value(config, "TAM_MEMORIA");
	tam_pagina = config_get_int_value(config, "TAM_PAGINA");
	entradas_por_tabla = config_get_int_value(config, "ENTRADAS_POR_TABLA");
	retardo_swap = config_get_int_value(config, "RETARDO_SWAP");



	int socket_cpu_escucha = iniciar_servidor(ip_memoria, puerto_escucha);

	log_info(logger, "memoria lista para recibir al request");
	pthread_t socket_escucha_memoria;

	void* _f_aux_escucha(void* socket_cpu_interrupt){
		escuchar(*(int*)socket_cpu_interrupt, config);
		return NULL;
	}

	pthread_create(&socket_escucha_memoria, NULL, _f_aux_escucha, (void*) &socket_cpu_escucha);

	pthread_join(socket_escucha_dispatch, NULL);
	return 0;
}

void* atender_cpu(void* input){
	int* cliente_fd = (int *) input;
	//aca hay que mandarle al cpu los parametros de la memoria que necesiten para los calculos y atender cualquier msg de la cpu
	int respuesta_generica = 10;
	while(1){
		int codigo_de_paquete = recibir_operacion(*cliente_fd);
		switch(codigo_de_paquete) {
		case MENSAJE_ESTANDAR:
			send(cliente_fd, &respuesta_generica, sizeof(int), 0);
			break;
		}
	}
	return NULL;
}
	
void* atender_kernel(void* input){
	int* cliente_fd = (int *) input;
	// atender cualquier msg del kernel
	int respuesta_generica = 10;
	
	
	while(1){
		int codigo_de_paquete = recibir_operacion(*cliente_fd);
		switch(codigo_de_paquete) {
		case MENSAJE_ESTANDAR:
			send(cliente_fd, &respuesta_generica, sizeof(int), 0);
			break;
		}
	}
	return NULL;
}

void* escuchar(int socket_escucha_memoria){

	pthread_t thread_type;
	int cliente_fd = esperar_cliente(socket_kernel_escucha);
	pthread_create(&thread_type, NULL, atender_cpu, (void*) &cliente_fd );

	int kernel_conexion = esperar_cliente(socket_memoria_memoria);
	pthread_t thread_kernel;
	pthread_create(&thread_kernel, NULL, atender_cpu, (void*) &cliente_fd );
 	return NULL;
}

void crear_tablas_de_2do_nivel(int cantidad_de_entradas_de_paginas_2do_nivel, t_list* lista_de_tablas_de_primer_nivel){
	int iterador = 0;
	int contador = 0;
	uint32_t cantidad_de_entradas_de_paginas_1er_nivel = cantidad_de_entradas_de_paginas_2do_nivel / entradas_por_tabla;
	if((cantidad_de_entradas_de_paginas_2do_nivel % entradas_por_tabla) > 0){
		cantidad_de_entradas_de_paginas_1er_nivel++;
	}
	while(iterador < cantidad_de_entradas_de_paginas_1er_nivel){
		t_list* lista_de_paginas_2do_nivel = list_create();
		int i;
		for(i = 0; i < entradas_por_tabla && contador < cantidad_de_entradas_de_paginas_2do_nivel; i++){
			contador++;
			tabla_de_segundo_nivel* pagina = malloc(sizeof(tabla_de_segundo_nivel));
			pagina->marco = -1; // Es -1 ya que no tiene ningun marco asignado el proceso cuando recien se crea
			pagina->numero_de_pagina = i;
			pagina->bit_presencia = 0;
			pagina->bit_de_uso = 0;
			pagina->bit_modificado = 0;
			list_add(lista_de_paginas_2do_nivel, pagina);
		}
		uint32_t index = list_size(lista_de_tablas_2do_nivel);
		list_add(lista_de_tablas_2do_nivel, lista_de_paginas_2do_nivel);
		list_add(lista_de_tablas_de_primer_nivel, &index);
		iterador++;
	}
}

uint32_t crear_proceso(int tamanio_en_memoria){
	uint32_t cantidad_de_entradas_de_paginas_2do_nivel = tamanio_en_memoria / tam_pagina;
	if((tamanio_en_memoria % tam_pagina) > 0){
		cantidad_de_entradas_de_paginas_2do_nivel++;
	}
	t_list* lista_1er_nivel_proceso = list_create();
	crear_tablas_de_2do_nivel(cantidad_de_entradas_de_paginas_2do_nivel, lista_1er_nivel_proceso);
	uint32_t tabla_pags = list_size(tablas_1er_nivel); // retornar el index de la tabla de paginas al kernel para que se agregue a la pcb
	list_add(tablas_1er_nivel, lista_1er_nivel_proceso);
}
