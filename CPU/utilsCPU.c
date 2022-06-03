#include "utilsCPU.h"
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
	buffer = malloc(*size);									 // Le asignamos al buffer el tamaño que requerira para almacenar esas instrucciones
	printf("tam: %d\n", *size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);		 // Le damos las instrucciones

	return buffer;
}

void* recibir_instrucciones(int socket_cliente){
	int size;
	void * buffer;
	void* buffer_aux;

	buffer = recibir_buffer(&size, socket_cliente);
	buffer_aux = malloc(sizeof(uint32_t) + size);
	memcpy(buffer_aux, &size, sizeof(uint32_t));
	memcpy(buffer_aux + sizeof(uint32_t), buffer, size);


	free(buffer);
	return buffer_aux;
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

void empaquetar_y_enviar(t_buffer* buffer, int socket, uint32_t codigo_operacion){
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion_de_paquete = codigo_operacion;
	paquete->buffer = buffer;

	//  					 lista  codigo_operacion_de_paquete   tamaño_lista
	void* a_enviar = malloc(buffer->size + sizeof(uint32_t) + sizeof(uint32_t));
	uint32_t offset = 0;

	memcpy(a_enviar + offset, &(paquete->codigo_operacion_de_paquete), sizeof(int));
	offset += sizeof(int);
	memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

	send(socket, a_enviar, buffer->size + sizeof(uint32_t) + sizeof(uint32_t), 0);


	// No nos olvidamos de liberar la memoria que ya no usaremos
	free(a_enviar);
	eliminar_paquete(paquete);
}


void empaquetar_y_enviar_i_o(t_buffer* buffer, int socket, uint32_t codigo_operacion, uint32_t tiempo_bloqueo){
	t_paquete_i_o* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion_de_paquete = codigo_operacion;
	paquete->tiempo_bloqueo = tiempo_bloqueo;
	paquete->buffer = buffer;

	//  					 lista  codigo_operacion_de_paquete   tamaño_lista	  tiempo_bloqueo
	void* a_enviar = malloc(buffer->size + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t));
	uint32_t offset = 0;

	memcpy(a_enviar + offset, &(paquete->codigo_operacion_de_paquete), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(a_enviar + offset, &(paquete->tiempo_bloqueo), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

	send(socket, a_enviar, buffer->size + sizeof(uint32_t) + sizeof(uint32_t), 0);


	// No nos olvidamos de liberar la memoria que ya no usaremos
	free(a_enviar);
	//eliminar_paquete(paquete);  hay que hacer un eliminarpaqueteio
}

void recibir_pcb(int socket_cliente, pcb* pcb_recibido){
	int size;
	uint32_t offset = 0;
	uint32_t offset_instrucciones = 0;
	void * buffer;
	t_list* instrucciones = list_create();

	buffer = recibir_buffer(&size, socket_cliente); // almacena en size el tamanio de todo el buffer y ademas guarda en buffer todo el stream
	memcpy(&pcb_recibido->pid, buffer, sizeof(uint32_t));
	offset+=sizeof(uint32_t);

	memcpy(&pcb_recibido->tamanio_en_memoria, buffer + offset, sizeof(uint32_t));
	offset+=sizeof(uint32_t);

	memcpy(&pcb_recibido->tamanio_stream_instrucciones, buffer + offset, sizeof(uint32_t));
	offset+=sizeof(uint32_t);

	while(offset_instrucciones < pcb_recibido->tamanio_stream_instrucciones){
		instruccion *ins_recibida = malloc(sizeof(instruccion));
		ins_recibida->parametros = NULL;
		memcpy(&ins_recibida->cod_op, buffer + offset + offset_instrucciones, sizeof(uint32_t));
		offset_instrucciones+=sizeof(uint32_t);
		memcpy(&ins_recibida->tam_param, buffer + offset + offset_instrucciones, sizeof(uint32_t));
		offset_instrucciones+=sizeof(uint32_t);


		//lista de parametros
		if(ins_recibida->tam_param){
			ins_recibida->parametros = malloc(ins_recibida->tam_param);
			memcpy(ins_recibida->parametros, buffer + offset + offset_instrucciones, ins_recibida->tam_param);
			offset_instrucciones+=ins_recibida->tam_param;
		}

		list_add(instrucciones, ins_recibida);
	}

	pcb_recibido->instrucciones = instrucciones;

	offset += offset_instrucciones;

	memcpy(&pcb_recibido->program_counter, buffer + offset, sizeof(uint32_t));
	offset+=sizeof(uint32_t);

	memcpy(&pcb_recibido->tamanio_paginas, buffer + offset, sizeof(uint32_t));
	offset+=sizeof(uint32_t);

	memcpy(pcb_recibido->tabla_paginas, buffer + offset, pcb_recibido->tamanio_paginas);
	offset+=pcb_recibido->tamanio_paginas;

	memcpy(&pcb_recibido->estimacion_siguiente, buffer + offset, sizeof(float));
	offset+=sizeof(float);

	memcpy(&pcb_recibido->timestamp_inicio_exe, buffer + offset, sizeof(float));
	offset+=sizeof(float);

	memcpy(&pcb_recibido->real_actual, buffer + offset, sizeof(float));
	offset+=sizeof(float);

	memcpy(&pcb_recibido->socket_consola, buffer + offset, sizeof(int));

	free(buffer);

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



void imprimir_pcb(pcb* recepcion){
	printf("pid: %d\n", recepcion->pid);
	printf("tamanio_en_memoria: %d\n", recepcion->tamanio_en_memoria);
	list_iterate(recepcion->instrucciones, imprimir_instruccion);
	printf("program_counter: %d\n", recepcion->program_counter);
	printf("tamanio_paginas: %d\n", recepcion->tamanio_paginas);
	printf("estimacion_siguiente: %f\n", recepcion->estimacion_siguiente);

}

uint32_t serializar_instruccion(t_buffer* buffer, instruccion* instruccion, uint32_t offset){
	//buffer->size += sizeof(uint32_t) * 2 + instruccion->tam_param; // Parámetros
	int cant = instruccion->tam_param/sizeof(uint32_t);
	//buffer->stream = (void *)realloc(buffer->stream, offset + (sizeof(uint32_t)*2) + instruccion->tam_param);

	memcpy(buffer->stream + offset, &instruccion->cod_op, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(buffer->stream + offset, &instruccion->tam_param, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	for(int i = 0; i < cant; i++){
		memcpy(buffer->stream + offset, &instruccion->parametros[i], sizeof(uint32_t));
		offset += sizeof(uint32_t);
	}

	return offset;
}

t_buffer* serializar_header(pcb* header){
	t_buffer* buffer = malloc(sizeof(t_buffer));
	uint32_t offset = 0;
	uint32_t buffer_size = 6*sizeof(uint32_t) + header->tamanio_stream_instrucciones + header->tamanio_paginas + 3*sizeof(float);

	buffer->size = buffer_size; // Parámetros

	buffer->stream = malloc(buffer->size);

	memcpy(buffer->stream + offset, &header->pid, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(buffer->stream + offset, &header->tamanio_en_memoria, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(buffer->stream + offset, &header->tamanio_stream_instrucciones, sizeof(uint32_t));
	offset += sizeof(uint32_t);


	void _f_aux(void* instruccion_recibida){									// _f_aux recibe una instruccion de la lista de instrucciones (Mas abajo en el list_iterate) y envia a serializarla. Una vez que lo hace agrega al offset para desplazarse la cantidad de espacio que ocupo esa instruccion.
		instruccion * ins = (instruccion *) instruccion_recibida;
		offset = serializar_instruccion(buffer, ins, offset);					// Se incrementa el offset para que cuando serializar instruccion tome una nueva instruccion, nos desplacemos y no pisemos la instruccion anterior que serializamos
	}

	list_iterate(header->instrucciones, _f_aux);									// Cada instruccion en la lista de instrucciones va a realziar la funcion auxiliar

	memcpy(buffer->stream + offset, &header->program_counter, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(buffer->stream + offset, &header->tamanio_paginas, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(buffer->stream + offset, header->tabla_paginas, header->tamanio_paginas);
	offset += header->tamanio_paginas;

	memcpy(buffer->stream + offset, &header->estimacion_siguiente, sizeof(float));
	offset += sizeof(float);

	memcpy(buffer->stream + offset, &header->timestamp_inicio_exe, sizeof(float));
	offset += sizeof(float);

	memcpy(buffer->stream + offset, &header->real_actual, sizeof(float));
	offset += sizeof(float);

	memcpy(buffer->stream + offset, &header->socket_consola, sizeof(int));

	return buffer;

}
