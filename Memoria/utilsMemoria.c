#include "utilsMemoria.h"

void eliminar_paquete(t_paquete* paquete) {
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

int iniciar_servidor(char* ip, char* puerto)
{
	int socket_servidor;

	struct addrinfo hints, *servinfo, *p;

	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &servinfo);

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
		send(socket_cliente, &resultError, sizeof(uint32_t), 0);
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
															// Al hacer el recv, el size se pierde, socket_cliente va a apuntar no al inicio, si no 4 bytes desplazados, o sea a donde comienzan las instrucciones
	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);   // Almacena en size lo que hay dentro de los primeros 4 bytes, que es el tamaño que van a ocupar las instrucciones
	buffer = malloc(*size);									// Le asignamos al buffer el tamaño que requerira para almacenar esas instrucciones
	//printf("tam: %d\n", *size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);		// Le damos las instrucciones

	return buffer;
}


int crear_conexion(char *ip, char* puerto) {
	struct addrinfo hints;
	struct addrinfo *server_info;

	uint32_t handshake =1;
	uint32_t result;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	int socket_cliente = 0;
	socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

	// Ahora que tenemos el socket, vamos a conectarlo
	connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen);

	send(socket_cliente, &handshake, sizeof(uint32_t), 0);
	recv(socket_cliente, &result, sizeof(uint32_t), MSG_WAITALL);
	if(!result)
		puts("conexion exitosa");
	else
		puts("conexion fallida");
	freeaddrinfo(server_info);

	return socket_cliente;
}

void liberar_conexion(int socket_cliente) {
	close(socket_cliente);
}


