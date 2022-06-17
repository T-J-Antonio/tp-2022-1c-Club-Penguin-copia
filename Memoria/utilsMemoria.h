#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/collections/queue.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define OPERACION_ENVIO_INSTRUCCIONES 0
#define OPERACION_ENVIO_PCB 1
#define OPERACION_IO 2
#define OPERACION_EXIT 3
#define CPU_LIBRE 4
#define OPERACION_INTERRUPT 5
#define RESPUESTA_INTERRUPT 6
#define MENSAJE_ESTANDAR 10

#define PROCESO_FINALIZADO 101

#define SRT 22
#define FIFO 21

t_log* logger;
pthread_t pasar_a_ready;
typedef struct
{
	uint32_t cod_op;
	uint32_t tam_param;
	uint32_t *parametros;
} instruccion;
typedef struct
{
	uint32_t size;
	void* stream;
} t_buffer;

typedef struct
{
	uint32_t codigo_operacion_de_paquete;
	t_buffer* buffer;
} t_paquete;
typedef struct
{
	uint32_t pid;
	uint32_t tamanio_en_memoria;
	uint32_t tamanio_stream_instrucciones;
	void* instrucciones;
	uint32_t program_counter;
	uint32_t tabla_paginas;
	float estimacion_siguiente;
	float timestamp_inicio_exe;
	float real_actual;
	int socket_consola;
} pcb;

typedef struct{
	uint32_t marco;
	uint32_t numero_de_pagina;
	uint32_t bit_presencia;
	uint32_t bit_de_uso;
	uint32_t bit_modificado;

} tabla_de_segundo_nivel;

t_list* lista_de_tablas_de_primer_nivel;
//FUNCIONES TRAÍDAS DEL TP0

int iniciar_servidor(char*, char*);
int esperar_cliente(int);
int recibir_operacion(int);
//Dados IP y puerto del servidor, realiza el handshake con él para establecer la conexión
int crear_conexion(char* ip, char* puerto);

//Cierra el socket pasado por parámetro
void liberar_conexion(int socket_cliente);

//FUNCIONES PROPIAS
void* recibir_buffer(int*, int);
