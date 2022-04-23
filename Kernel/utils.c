#include"utils.h"

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
		send(socket_cliente, &resultOk, sizeof(uint32_t), NULL);
	else
		send(socket_cliente, &resultError, sizeof(uint32_t), NULL); // cambiar NULL por 0
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

void recibir_mensaje(int socket_cliente)
{
	int size;
	char* buffer = recibir_buffer(&size, socket_cliente);
	log_info(logger, "Me llego el mensaje %s", buffer);
	free(buffer);
}

t_list* recibir_paquete(int socket_cliente)
{
	int size;
	int desplazamiento = 0;
	void * buffer;
	t_list* valores = list_create();
	int tamanio;

	buffer = recibir_buffer(&size, socket_cliente);
	while(desplazamiento < size)
	{
		memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		char* valor = malloc(tamanio);
		memcpy(valor, buffer+desplazamiento, tamanio);
		desplazamiento+=tamanio;
		list_add(valores, valor);
	}
	free(buffer);
	return valores;
}
t_list* recibir_instrucciones(int socket_cliente){
	int size;
	int desplazamiento = 0;
	void * buffer;
	t_list* instrucciones = list_create();

	buffer = recibir_buffer(&size, socket_cliente);
	while(desplazamiento < size){
		instruccion *recibida = malloc(sizeof(instruccion));
		recibida->parametros = NULL;
		memcpy(&recibida->cod_op, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		memcpy(&recibida->tam_param, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);


		//lista de parametros
		if(recibida->tam_param){
			recibida->parametros = malloc(recibida->tam_param);
			memcpy(recibida->parametros, buffer+desplazamiento, recibida->tam_param);
			desplazamiento+=recibida->tam_param;
		}


		list_add(instrucciones, recibida);
	}
	free(buffer);
	return instrucciones;
}





void imprimir_parametros(void *param){
	uint32_t numero = (uint32_t)atoi(param);
	printf("cod_op: %d\n", numero);
}

void imprimir_lista(void * var){
	instruccion *alo = (instruccion *) var;

	printf("%d\n", alo->cod_op);
	uint32_t i = 0;
	uint32_t cant = alo->tam_param/sizeof(uint32_t);

	while(i<cant){
		printf("num: %d\n", alo->parametros[i]);
		i++;
	}
}
