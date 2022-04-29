



































//t_list* recibir_instrucciones(int socket_cliente, uint32_t* tamanio_en_memoria){
//	int size;
//	uint32_t offset = 0;
//	void * buffer;
//	t_list* instrucciones = list_create();
//
//	buffer = recibir_buffer(&size, socket_cliente);
//	memcpy(tamanio_en_memoria, buffer, sizeof(uint32_t));
//	offset+=sizeof(uint32_t);
//	while(offset < size){
//		instruccion *recibida = malloc(sizeof(instruccion));
//		recibida->parametros = NULL;
//		memcpy(&recibida->cod_op, buffer + offset, sizeof(uint32_t));
//		offset+=sizeof(uint32_t);
//		memcpy(&recibida->tam_param, buffer + offset, sizeof(uint32_t));
//		offset+=sizeof(uint32_t);
//
//
//		//lista de parametros
//		if(recibida->tam_param){
//			recibida->parametros = malloc(recibida->tam_param);
//			memcpy(recibida->parametros, buffer+offset, recibida->tam_param);
//			offset+=recibida->tam_param;
//		}
//
//
//		list_add(instrucciones, recibida);
//	}
//	free(buffer);
//	return instrucciones;
//}
//
//void imprimir_instruccion(void * var){
//	instruccion *alo = (instruccion *) var;
//
//	printf("codigo de operacion: %d\n", alo->cod_op);
//	uint32_t i = 0;
//	uint32_t cant = alo->tam_param/sizeof(uint32_t);
//
//	while(i<cant){
//		printf("parametro: %d\n", alo->parametros[i]);
//		i++;
//	}
//}
