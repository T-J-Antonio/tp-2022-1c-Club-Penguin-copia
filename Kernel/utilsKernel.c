#include "utilsKernel.h"


int iniciar_servidor(void)
{
	int socket_servidor;

	struct addrinfo hints, *servinfo, *p;

	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(IP, PUERTO, &hints, &servinfo);

	socket_servidor = socket(servinfo->ai_family,
							servinfo->ai_socktype,
							servinfo->ai_protocol);


	bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen);

	listen(socket_servidor, SOMAXCONN);

	freeaddrinfo(servinfo);
	log_trace(logger, "Listo para escuchar a mi cliente");

	return socket_servidor;
}

int esperar_cliente(int socket_servidor)
{
	uint32_t handshake;
	uint32_t resultOk = 0;
	uint32_t resultError = -1;
	int socket_cliente = accept(socket_servidor, NULL, NULL);

	recv(socket_cliente, &handshake, sizeof(uint32_t), MSG_WAITALL);
	if(handshake ==1)
		send(socket_cliente, &resultOk, sizeof(uint32_t), 0);
	else
		send(socket_cliente, &resultError, sizeof(uint32_t), 0); // cambiar NULL por 0
	log_info(logger, "Se conecto un cliente!");

	return socket_cliente;
}

int recibir_operacion(int socket_cliente)
{
	int cod_op;
	if(recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
		return cod_op;
	else
	{
		close(socket_cliente);
		return -1;
	}
}

void* recibir_buffer(int* size, int socket_cliente)
{
	void * buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

t_list* recibir_instrucciones(int socket_cliente, uint32_t* tamanio_en_memoria){
	int size;
	uint32_t offset = 0;
	void * buffer;
	t_list* instrucciones = list_create();

	buffer = recibir_buffer(&size, socket_cliente);
	memcpy(tamanio_en_memoria, buffer, sizeof(uint32_t));
	offset+=sizeof(uint32_t);
	while(offset < size){
		instruccion *recibida = malloc(sizeof(instruccion));
		recibida->parametros = NULL;
		memcpy(&recibida->cod_op, buffer + offset, sizeof(uint32_t));
		offset+=sizeof(uint32_t);
		memcpy(&recibida->tam_param, buffer + offset, sizeof(uint32_t));
		offset+=sizeof(uint32_t);


		//lista de parametros
		if(recibida->tam_param){
			recibida->parametros = malloc(recibida->tam_param);
			memcpy(recibida->parametros, buffer+offset, recibida->tam_param);
			offset+=recibida->tam_param;
		}


		list_add(instrucciones, recibida);
	}
	free(buffer);
	return instrucciones;
}

void imprimir_instruccion(void * var){
	instruccion *alo = (instruccion *) var;

	printf("codigo de operacion: %d\n", alo->cod_op);
	uint32_t i = 0;
	uint32_t cant = alo->tam_param/sizeof(uint32_t);

	while(i<cant){
		printf("parametro: %d\n", alo->parametros[i]);
		i++;
	}
}

