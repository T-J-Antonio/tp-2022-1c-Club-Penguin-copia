#ifndef UTILS_H_
#define UTILS_H_

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include<string.h>
#include<assert.h>

#define IP "127.0.0.1"
#define PUERTO "8000"
typedef struct {
	uint32_t cod_op;
	uint32_t tam_param;
	uint32_t *parametros;
} instruccion;
typedef enum
{
	OPERACION_ENVIO_INSTRUCCIONES,
	MENSAJE,
	PAQUETE
}op_code;

t_log* logger;

void* recibir_buffer(int*, int);

int iniciar_servidor(void);
int esperar_cliente(int);
t_list* recibir_paquete(int);
void recibir_mensaje(int);
int recibir_operacion(int);
t_list* recibir_instrucciones(int);
void imprimir_parametros(void *);
void imprimir_lista(void *);

#endif /* UTILS_H_ */
