#include "utilsCPU.h"

long currentTimeMillis() {
  struct timeval time;
  gettimeofday(&time, NULL);

  return time.tv_sec * 1000 + time.tv_usec / 1000;
}

int iniciar_servidor(char* ip, char* puerto){
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
	t_paquete_i_o* paquete = malloc(sizeof(t_paquete_i_o));
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
	//printf("a enviar: %d\n", buffer->size + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t));
	int enviado = send(socket, a_enviar, buffer->size + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t), 0);
	//printf("enviado: %d\n", enviado);

	// No nos olvidamos de liberar la memoria que ya no usaremos
	free(a_enviar);
	eliminar_paquete_i_o(paquete);
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

	memcpy(&pcb_recibido->tabla_paginas, buffer + offset, sizeof(uint32_t));
	offset+=sizeof(uint32_t);

	memcpy(&pcb_recibido->estimacion_siguiente, buffer + offset, sizeof(long));
	offset+=sizeof(long);

	memcpy(&pcb_recibido->timestamp_inicio_exe, buffer + offset, sizeof(long));
	offset+=sizeof(long);

	memcpy(&pcb_recibido->real_actual, buffer + offset, sizeof(long));
	offset+=sizeof(long);

	memcpy(&pcb_recibido->socket_consola, buffer + offset, sizeof(int));

	free(buffer);

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
	uint32_t buffer_size = 6*sizeof(uint32_t) + header->tamanio_stream_instrucciones + 3*sizeof(long); //aca dudo si esta bien el calculo dejamos el tam paginas

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

	list_iterate(header->instrucciones, _f_aux);								// Cada instruccion en la lista de instrucciones va a realziar la funcion auxiliar

	memcpy(buffer->stream + offset, &header->program_counter, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(buffer->stream + offset, &header->tabla_paginas, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(buffer->stream + offset, &header->estimacion_siguiente, sizeof(long));
	offset += sizeof(long);

	memcpy(buffer->stream + offset, &header->timestamp_inicio_exe, sizeof(long));
	offset += sizeof(long);

	memcpy(buffer->stream + offset, &header->real_actual, sizeof(long));
	offset += sizeof(long);

	memcpy(buffer->stream + offset, &header->socket_consola, sizeof(int));

	return buffer;

}

void instruccion_destroyer(void* elem){
	instruccion* una_instruccion = (instruccion*) elem;
	free(una_instruccion->parametros);
	free(una_instruccion);
}

void liberar_pcb(pcb* pcb){
	list_destroy_and_destroy_elements(pcb->instrucciones, instruccion_destroyer);
	free(pcb);
}

void eliminar_paquete(t_paquete* paquete) {
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

void eliminar_paquete_i_o(t_paquete_i_o* paquete) {
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

uint32_t obtener_direccion_fisica(uint32_t direccion_logica, uint32_t tabla_paginas, uint32_t pid){
	
	uint32_t nro_pagina = numero_pagina(direccion_logica);
	uint32_t direccion_fisica;

	if(esta_en_TLB(nro_pagina)){
		uint32_t marco = obtener_marco_de_TLB(nro_pagina); //hay que marcarle un nuevo timestamp a la tlb ya que se referencio denuevo
		uint32_t desplazamiento = desplazamiento_memoria(direccion_logica, nro_pagina);
		direccion_fisica = marco * tamanio_pagina + desplazamiento; // Tamanio pagina viene desde memoria.config aca es marco o nro de marco, antes decia nro marco pero esa var no existe
	}
	else {
		uint32_t tabla_1er_nivel = entrada_tabla_1er_nivel(nro_pagina);
		//printf("nro de indice: %d\n", tabla_1er_nivel);
		//printf("tabla pags: %d\n", tabla_paginas);
		uint32_t index_tabla_2do_nivel = primer_acceso_a_memoria(tabla_paginas, tabla_1er_nivel);
		
		uint32_t tabla_2do_nivel = entrada_tabla_2do_nivel(nro_pagina);
		uint32_t nro_marco = segundo_acceso_a_memoria(index_tabla_2do_nivel, tabla_2do_nivel, nro_pagina, pid);
	
		uint32_t desplazamiento = desplazamiento_memoria(direccion_logica, nro_pagina);
		direccion_fisica = nro_marco * tamanio_pagina + desplazamiento;
		//guardar_en_TLB(nro_pagina, nro_marco); aca hay que tener cuidado ya que si la actualizo, no deberia guardar porque seria al dope esto se comenta y se delega ala func 2do acceso ya que podemos hacerlo de ahi y facilitar la logica
	}

	return direccion_fisica;
}

uint32_t algoritmo_reemplazo_to_int(char* str){
	if(strcmp(str, "FIFO")==0) return REEMPLAZO_FIFO;
	else return REEMPLAZO_LRU;
}

void guardar_en_TLB(uint32_t nro_pagina, uint32_t nro_marco) {
	entrada_tlb* nueva_entrada = malloc(sizeof(entrada_tlb));
	entrada_tlb* entrada_a_reemplazar;
	
	nueva_entrada->pagina = nro_pagina;
	nueva_entrada->marco = nro_marco;
	nueva_entrada->timestamp = currentTimeMillis();
	
	if(queue_size(TLB) < cantidad_entradas_TLB) {
		queue_push(TLB, nueva_entrada);
	}
	
	else {
		switch(algoritmo_reemplazo_TLB)
		{
			case REEMPLAZO_FIFO:
				entrada_a_reemplazar = queue_pop(TLB);
				queue_push(TLB, nueva_entrada);
				free(entrada_a_reemplazar);
				break;
			case REEMPLAZO_LRU:
				algoritmo_LRU(nueva_entrada);
				break;
		}
	}
}

uint32_t esta_en_TLB(uint32_t nro_pagina){
	uint32_t se_encontro = 0;	
	for(uint32_t i = 0; i < queue_size(TLB); i++){
		entrada_tlb* una_entrada = queue_pop(TLB);
		if (una_entrada->pagina == nro_pagina){
			una_entrada->timestamp = currentTimeMillis();
			se_encontro = 1;
		}
		queue_push(TLB, una_entrada);
	}
	return se_encontro;
}

void actualizar_TLB(uint32_t nro_pagina, uint32_t nro_marco){
	for(uint32_t i = 0; i < queue_size(TLB); i++){
		entrada_tlb* una_entrada = queue_pop(TLB);
		if (una_entrada->marco == nro_marco){
			una_entrada->pagina = nro_pagina;
			una_entrada->timestamp = currentTimeMillis();
		}
		queue_push(TLB, una_entrada);
	}
}

uint32_t numero_pagina(uint32_t direccion_logica){
	// el tamaño página se solicita a la memoria al comenzar la ejecución
	return direccion_logica / tamanio_pagina; // ---> la división ya retorna el entero truncado a la unidad
}

uint32_t entrada_tabla_1er_nivel(uint32_t nro_pagina){
	// la cant. de entradas se solicita a la memoria
	return nro_pagina / cant_entradas_por_tabla;
}

uint32_t entrada_tabla_2do_nivel(uint32_t nro_pagina){
	return nro_pagina % cant_entradas_por_tabla;
}

uint32_t desplazamiento_memoria(uint32_t direccion_logica, uint32_t nro_pagina){
	return direccion_logica - nro_pagina * tamanio_pagina;
}

// Con este algoritmo, buscamos aquella entrada que este hace mas tiempo en la TLB para asi reemplazarla
void algoritmo_LRU(entrada_tlb* nueva_entrada){
	entrada_tlb* auxiliar1 = malloc(sizeof(entrada_tlb));
	entrada_tlb* auxiliar2 = malloc(sizeof(entrada_tlb));

	uint32_t tamanio = queue_size(TLB);
	auxiliar1 = queue_pop(TLB);

	for(int i = 1; i < tamanio; i++){
		auxiliar2 = queue_pop(TLB);

		if(auxiliar1->timestamp > auxiliar2->timestamp) {
			queue_push(TLB, auxiliar1);
			auxiliar1 = auxiliar2;
		}
// Si el aux1 es más nuevo en la TLB que el aux2, pushea el auxiliar 1 y le asigna el más viejo, o sea auxiliar 2. 
// De esta forma, en auxiliar 1 nos queda la entrada que esta hace mas tiempo en TLB para volver a comparar
		else
			queue_push(TLB, auxiliar2);
	}
	
	queue_push(TLB, nueva_entrada);
}

uint32_t obtener_marco_de_TLB(uint32_t nro_pagina_a_buscar){
	uint32_t auxiliar_marco = -1;
	for(uint32_t i = 0; i < queue_size(TLB); i++){
		entrada_tlb* una_entrada = queue_pop(TLB);
		if (una_entrada->pagina == nro_pagina_a_buscar){
			una_entrada->timestamp = currentTimeMillis();
			auxiliar_marco = una_entrada->marco;
		}
		queue_push(TLB, una_entrada);
	}
	return auxiliar_marco;
}

uint32_t primer_acceso_a_memoria(uint32_t tabla_paginas, uint32_t entrada_tabla_1er_nivel){
	uint32_t index_tabla_2do_nivel;
	int acc1ra = ACCESO_A_1RA_TABLA;


	send(conexion_memoria, &acc1ra, sizeof(uint32_t), 0);
	send(conexion_memoria, &tabla_paginas, sizeof(uint32_t), 0);
	send(conexion_memoria, &entrada_tabla_1er_nivel, sizeof(uint32_t), 0);

	recv(conexion_memoria, &index_tabla_2do_nivel, sizeof(uint32_t), MSG_WAITALL);

	return index_tabla_2do_nivel;
}

uint32_t segundo_acceso_a_memoria(uint32_t index_tabla_2do_nivel, uint32_t entrada_tabla_2do_nivel, uint32_t nro_pagina, uint32_t pid){
	uint32_t nro_marco;
	int acc2da = ACCESO_A_2DA_TABLA;

	send(conexion_memoria, &acc2da, sizeof(uint32_t), 0);
	send(conexion_memoria, &index_tabla_2do_nivel, sizeof(uint32_t), 0);
	send(conexion_memoria, &entrada_tabla_2do_nivel, sizeof(uint32_t), 0);
	send(conexion_memoria, &pid, sizeof(uint32_t), 0);

	uint32_t codigo = recibir_operacion(conexion_memoria);
	if (codigo == ACCESO_A_2DA_TABLA){
		recv(conexion_memoria, &nro_marco, sizeof(uint32_t), MSG_WAITALL);
		guardar_en_TLB(nro_pagina, nro_marco);
	}
	else if (codigo == REEMPLAZO_DE_PAGINA){
		recv(conexion_memoria, &nro_marco, sizeof(uint32_t), MSG_WAITALL);
		actualizar_TLB(nro_pagina, nro_marco);
	}		

	return nro_marco;
};

uint32_t leer_posicion_de_memoria(uint32_t direccion_fisica, uint32_t pid, uint32_t nro_pag ){
	int lec = LECTURA_EN_MEMORIA;

	send(conexion_memoria, &lec, sizeof(uint32_t), 0);
	send(conexion_memoria, &direccion_fisica, sizeof(uint32_t), 0);
	send(conexion_memoria, &pid, sizeof(uint32_t), 0);
	send(conexion_memoria, &nro_pag, sizeof(uint32_t), 0);
	uint32_t dato_leido = 0;

	recv(conexion_memoria, &dato_leido, sizeof(uint32_t), MSG_WAITALL);

	return dato_leido;
}

void vaciar_tlb(){
	while(queue_size(TLB) > 0){
		entrada_tlb* una_entrada = queue_pop(TLB);
		free(una_entrada);
	}
}

void escribir_en_posicion_de_memoria(uint32_t direccion_fisica, uint32_t dato_a_escribir, uint32_t pid, uint32_t nro_pag ){
	int esc = ESCRITURA_EN_MEMORIA;

	send(conexion_memoria, &esc, sizeof(uint32_t), 0);
	send(conexion_memoria, &direccion_fisica, sizeof(uint32_t), 0);
	send(conexion_memoria, &dato_a_escribir, sizeof(uint32_t), 0);
	send(conexion_memoria, &pid, sizeof(uint32_t), 0);
	send(conexion_memoria, &nro_pag, sizeof(uint32_t), 0);

	uint32_t nro_marco;
	
	recv(conexion_memoria, &nro_marco, sizeof(uint32_t), MSG_WAITALL);
}
