#include "utilsConsola.h"


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

void eliminar_paquete(t_paquete* paquete) {
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

uint32_t convertir_instruccion(char* str){
	if(!strcmp(str, "NO_OP"))
		return NO_OP;
	if(!strcmp(str, "I/O"))
		return I_O;
	if(!strcmp(str, "READ"))
		return READ;
	if(!strcmp(str, "WRITE"))
		return WRITE;
	if(!strcmp(str, "COPY"))
		return COPY;
	if(!strcmp(str, "EXIT"))
		return EXIT;
	return ERROR_OP_DESCONOCIDA;
}

uint32_t agregar_una_instruccion(t_list * lista_ins, void * param, uint32_t flag){
	char * inst = (char *) param;
	instruccion *instruccion_aux =  malloc(sizeof(instruccion));
	instruccion_aux->tam_param =0;
	instruccion_aux->cod_op = 0;
	instruccion_aux->parametros = NULL;
	char **instrucciones = string_split(inst, " ");


	instruccion_aux->cod_op = convertir_instruccion(instrucciones[0]);
	free(instrucciones[0]);

	if(instruccion_aux->cod_op == NO_OP && !flag){
		uint32_t numero_de_veces = (uint32_t)atoi(instrucciones[1]);
		free(instrucciones[1]);
		free(instruccion_aux);
		free(instrucciones);
		return numero_de_veces;
	}
	int j = 1;

	if(instruccion_aux->cod_op == NO_OP && flag){
		instruccion_aux->tam_param = 0;
		while(instrucciones[j]){
			free(instrucciones[j]);
			j++;
		}
	}
	j =1;

	while(instrucciones[j] && !flag){
		uint32_t numero = (uint32_t)atoi(instrucciones[j]);
		instruccion_aux->tam_param += sizeof(uint32_t);
		instruccion_aux->parametros = (uint32_t *)realloc(instruccion_aux->parametros, j*sizeof(uint32_t));
		instruccion_aux->parametros[j-1] = numero;
		free(instrucciones[j]);

		j++;
	}
	list_add(lista_ins, instruccion_aux);
	free(instrucciones);
	return 0;
}

void agregar_instrucciones(t_list * lista_ins, char** instrucciones){
	void _f_aux(char *elem ){
		uint32_t numero_de_veces;
		numero_de_veces = agregar_una_instruccion(lista_ins, elem, 0);	//DETALLE!! funcion para trabajar ya que las commons toman solo un parametro creo la auxiliar para pasar los dos que necesito a la funcion que realiza la logica
		for(uint32_t i = 0; i < numero_de_veces; i++){					//Acá entra sólo si es una NO_OP. Si es NO_OP 5, entrara 5 veces para realizar NO_OP
			agregar_una_instruccion(lista_ins, elem, 1);
		}
		free(elem);
	}
	string_iterate_lines(instrucciones, _f_aux);
}

long int tamanio_del_archivo(FILE * archivo){
	fseek(archivo, 0, SEEK_END);
	long int tamanio_archivo = ftell(archivo);
	fseek(archivo, 0, SEEK_SET);
	return tamanio_archivo;
}

void instruccion_destroyer(void* elem){
	instruccion* una_instruccion = (instruccion*) elem;
	free(una_instruccion->parametros);
	free(una_instruccion);
}

/* Serializa una instruccion, Reasignando el espacio que debera ocupar el stream del buffer, ya que antes
 Solo tenia el tamanio_en_memoria pero no lo que ocupaban las instrucciones */

uint32_t serializar_instruccion(t_buffer* buffer, instruccion* instruccion, uint32_t offset){
	buffer->size += sizeof(uint32_t) * 2 + instruccion->tam_param; // Parámetros
	int cant = instruccion->tam_param/sizeof(uint32_t);
	buffer->stream = (void *)realloc(buffer->stream, offset + (sizeof(uint32_t)*2) + instruccion->tam_param);

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

/* En esta funcion, al serializar las instrucciones, se le asigna el espacio que va a tener el tamanio_en_memoria
El tamanio que va a ocupar la instruccion en si, se lo asigna en la funcion de arriba "serializar_instruccion"
Entonces, por cada instruccion que serialicemos, nos vamos a correr un poco con el offset y vamos a ir agregandolas en seguidilla, primero con su tamanio y luego la instruccion */

t_buffer* serializar_instrucciones(t_list * lista_instrucciones, uint32_t tamanio_en_memoria){
	t_buffer* buffer = malloc(sizeof(t_buffer));								//
	buffer->size = sizeof(uint32_t);											// ----> Les asignamos a cada variable el espacio de memoria que necesitan
	buffer->stream = malloc(sizeof(uint32_t));									//
	uint32_t offset = 0;

	memcpy(buffer->stream, &tamanio_en_memoria, sizeof(uint32_t));				// Copiamos en stream, lo que se encuentra en tamanio memoria pero solo los primeros 4bytes
	offset += sizeof(uint32_t);													// Nos desplazamos la cantidad de memoria que asignamos anteriormente, o sea 4 bytes

	void _f_aux(void* instruccion_recibida){									// _f_aux recibe una instruccion de la lista de instrucciones (Mas abajo en el list_iterate) y envia a serializarla. Una vez que lo hace agrega al offset para desplazarse la cantidad de espacio que ocupo esa instruccion.
		instruccion * ins = (instruccion *) instruccion_recibida;
		offset = serializar_instruccion(buffer, ins, offset);					// Se incrementa el offset para que cuando serializar instruccion tome una nueva instruccion, nos desplacemos y no pisemos la instruccion anterior que serializamos
	}

	list_iterate(lista_instrucciones, _f_aux);									// Cada instruccion en la lista de instrucciones va a realziar la funcion auxiliar
	return buffer;

}

// Con esta funcion, creamos el paquete que va a enviar el buffer
// El buffer contiene las instrucciones en seguidilla sin espacios, al contrario que en un struct. ----> Ejemplo de enviar el struct que toma 10 bytes pero solo hay info de 6 bytes

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

