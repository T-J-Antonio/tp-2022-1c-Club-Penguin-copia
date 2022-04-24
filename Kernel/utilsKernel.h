#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include<assert.h>

#define IP "127.0.0.1"
#define PUERTO "8000"

#define OPERACION_ENVIO_INSTRUCCIONES 0

t_log* logger;

typedef struct
{
	uint32_t cod_op;
	uint32_t tam_param;
	uint32_t *parametros;
} instruccion;


//FUNCIONES TRAÍDAS DEL TP0

int iniciar_servidor(void);
int esperar_cliente(int);
int recibir_operacion(int);

//FUNCIONES PROPIAS

void* recibir_buffer(int*, int);
t_list* recibir_instrucciones(int);
void imprimir_parametros(void *);
void imprimir_instruccion(void *);
