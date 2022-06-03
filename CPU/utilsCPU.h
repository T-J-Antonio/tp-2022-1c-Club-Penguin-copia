#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <assert.h>
#include <pthread.h>
#include <semaphore.h>

#define OPERACION_ENVIO_INSTRUCCIONES 0
#define OPERACION_ENVIO_PCB 1
#define OPERACION_IO 2
#define OPERACION_EXIT 3
#define CPU_LIBRE 4
#define OPERACION_INTERRUPT 5
#define RESPUESTA_INTERRUPT 6

t_log* logger;

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
	uint32_t codigo_operacion_de_paquete;
	uint32_t tiempo_bloqueo;
	t_buffer* buffer;
} t_paquete_i_o;

typedef struct
{
	uint32_t pid;
	uint32_t tamanio_en_memoria;
	uint32_t tamanio_stream_instrucciones;
	t_list* instrucciones;
	uint32_t program_counter;
	uint32_t tamanio_paginas;
	void* tabla_paginas; //el tipo de dato se va a definir cuando hagamos la memoria
	float estimacion_siguiente;
	float timestamp_inicio_exe;
	float real_actual;
	int socket_consola;
} pcb;

enum operaciones
{
	NO_OP = 0,
	I_O = 1,
	READ = 2,
	WRITE = 3,
	COPY = 4,
	EXIT = 5
};

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
void* recibir_instrucciones(int);
pcb* crear_header(uint32_t, uint32_t, t_list*, t_config*);
void empaquetar_y_enviar(t_buffer*, int, uint32_t);

//Función similar a la anterior pero sirve para mandar con tiempo de bloqueo
void empaquetar_y_enviar_i_o(t_buffer*, int, uint32_t, uint32_t);
void imprimir_pcb(pcb* );
void imprimir_instruccion(void * );
void recibir_pcb(int , pcb* );
//Dada una instrucción, la añade al buffer parte por parte y retorna el offset generado por esta adición
uint32_t serializar_instruccion(t_buffer*, instruccion*, uint32_t);
t_buffer* serializar_header(pcb* );
//Dada una lista de instrucciones, construye un buffer con ellas y lo retorna
t_buffer * serializar_instrucciones(t_list *, uint32_t);
