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
#include <sys/time.h>

#define OPERACION_ENVIO_INSTRUCCIONES 0
#define OPERACION_ENVIO_PCB 1
#define OPERACION_IO 2
#define OPERACION_EXIT 3
#define CPU_LIBRE 4
#define OPERACION_INTERRUPT 5
#define RESPUESTA_INTERRUPT 6
#define CREAR_PROCESO 7
#define SUSPENDER_PROCESO 8
#define FINALIZAR_PROCESO 9
#define ACCESO_A_1RA_TABLA 10
#define ACCESO_A_2DA_TABLA 11
#define LECTURA_EN_MEMORIA 12
#define ESCRITURA_EN_MEMORIA 13
#define REEMPLAZO_DE_PAGINA 14
#define REANUDAR_PROCESO 14

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
	long estimacion_siguiente;
	long timestamp_inicio_exe;
	long real_actual;
	int socket_consola;
} pcb;



sem_t contador_de_listas_esperando_para_estar_en_ready; //indicar al plani largo cuando el grado de multiprogramacion decrece y puede poner uno en ready

sem_t sem_contador_multiprogramacion;

sem_t mutex_cola_ready; //mutex para trabajar sobre la lista de ready

sem_t mutex_grado_de_multiprogramacion; //mutex para trabajar sobre la varianle global "grado de multiprogramacion"

sem_t mutex_cola_new; //mutex para trabajar sobre la lista de new

sem_t mutex_cola_sus_ready;

sem_t mutex_cola_suspendido;

sem_t mutex_respuesta_interrupt;

sem_t binario_flag_interrupt;

sem_t actualmente_replanificando;

sem_t signal_a_io;

sem_t dispositivo_de_io;

sem_t binario_lista_ready;
sem_t binario_plani_corto;

t_queue* cola_procesos_nuevos;

t_queue* cola_procesos_sus_ready;

t_queue* cola_de_ready;

t_queue* rta_int;

t_queue* cola_de_io;

t_dictionary* pid_handler;

t_dictionary* process_state;

int inicio;

int conexion_CPU_dispatch;
int conexion_memoria;

uint32_t proximo_pid;
uint32_t resultOk;

long estimacion_inicial;
double alfa;

int flag_respuesta_a_interrupcion; //CORREGIR INTERRUPCION CON DOBLE R
int conexion_cpu_interrupt;
long tiempo_de_espera_max;
long tiempo_inicio;
int int_modo_planificacion;

long currentTimeMillis();

//FUNCIONES TRAÍDAS DEL TP0

int iniciar_servidor(char*, char*);
int esperar_cliente(int);
int recibir_operacion(int);
//Dados IP y puerto del servidor, realiza el handshake con él para establecer la conexión
int crear_conexion(char* ip, char* puerto);

//Cierra el socket pasado por parámetro
void liberar_conexion(int socket_cliente);

//FUNCIONES PROPIAS
t_buffer* serializar_header(pcb*);
void* recibir_buffer(int*, int);
void* recibir_instrucciones(int);
void crear_header(uint32_t, void*, t_config*, pcb*, long);
void empaquetar_y_enviar(t_buffer*, int, uint32_t);

void recibir_pcb(int , pcb* );
void liberar_pcb(pcb*);
