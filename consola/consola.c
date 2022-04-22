#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "utils.h"
#include <string.h>
#include <commons/string.h>
#include <commons/collections/list.h>

#define EXITO 0
#define ERROR_ARGUMENTOS 1
#define ERROR_ARCHIVO 2

#define OPERACION_ENVIO_INSTRUCCIONES 0

enum operaciones
{
	NO_OP = 0,
	I_O = 1,
	READ = 2,
	WRITE = 3,
	COPY = 4,
	EXIT = 5
};

typedef struct {
	uint32_t cod_op;
	uint32_t tam_param;
	uint32_t *parametros;
} instruccion;

typedef struct {
    uint32_t size; // Tamaño del payload
    void* stream; // Payload
} t_buffer_instrucciones;

typedef struct {
    uint32_t codigo_operacion_de_paquete;
    t_buffer_instrucciones* buffer;
} t_paquete_instrucciones;

//declaracion de funciones
void imprimir_parametros(void *param);
void imprimir_lista(void * var);
void instruccion_destroyer(void* elem);
long int tamanio_del_archivo(FILE *);
void agregar_instrucciones(t_list * , char** );
uint32_t convertir_instruccion(char*);
uint32_t serializar_instruccion(t_buffer_instrucciones*, instruccion*, uint32_t);
void * serializar_instrucciones(t_list *, int);




int main (int argc, char** argv) {
	int conexion = crear_conexion("127.0.0.1", "4444");
	if (argc != 3) return ERROR_ARGUMENTOS; 											//ERROR: cant. errónea de argumentos
	char* path = argv[1];
	int tamanio = atoi(argv[2]);

	t_list * lista_instrucciones = list_create();										//creo la lista que va a contener todos los struct instruccion


	FILE* archivo_instrucciones = fopen(path, "rt"); 									//abro el archivo entero
	if (!archivo_instrucciones) return ERROR_ARCHIVO; 									//ERROR: el archivo no pudo abrirse
	long int tamanio_archivo = tamanio_del_archivo(archivo_instrucciones); 				//veo su tamaño

	char* archivo_string = malloc(tamanio_archivo + 1); 								//asigno memoria donde va a parar lo leído
	fread(archivo_string, tamanio_archivo, 1, archivo_instrucciones); 					//leo
	if(archivo_string == NULL) return ERROR_ARCHIVO;
	fclose(archivo_instrucciones);														//cierro
	archivo_string[tamanio_archivo] = '\0';												// arreglo de un posible catastrofe

	char** archivo_dividido = string_split(archivo_string, "\n");						//separo lo leído por línea ya que cada línea es una instrucción y prosigo a liberar la memoria de lo leído
	free(archivo_string);

	agregar_instrucciones(lista_instrucciones, archivo_dividido);						//función para ir agregando cosas a la lista de structs seguido de la liberación del archivo dividido
	free(archivo_dividido);


	serializar_instrucciones(lista_instrucciones, conexion);
	 
	list_destroy_and_destroy_elements(lista_instrucciones, instruccion_destroyer);

	return EXITO;
}




//definición de funciones

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
	return 100;
}

uint32_t agregar_una_instruccion(t_list * lista_ins, void * param, uint32_t flag){
	char * inst = (char *) param;
	instruccion *instruccion_aux =  malloc(sizeof(instruccion));
	instruccion_aux->tam_param =0;
	instruccion_aux->cod_op = 0;
	instruccion_aux->parametros = NULL;
	char **instrucciones = string_split(inst, " ");
	free(param);
	
// aca se agrega el string, habria que cambiar a enum
	
	instruccion_aux->cod_op = convertir_instruccion(instrucciones[0]);
	
	if(instruccion_aux->cod_op == NO_OP && !flag){
		uint32_t numero_de_veces = (uint32_t)atoi(instrucciones[1]);
		free(instrucciones[1]);
		return numero_de_veces;
	}
	
	//esto no cambia
	
	free(instrucciones[0]);


	int j = 1;
	while(instrucciones[j]) {
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
		for(uint32_t i = 0; i < numero_de_veces; i++){					//Acá entra sólo si es una NO_OP
			agregar_una_instruccion(lista_ins, elem, 1);
		}
	}
	string_iterate_lines(instrucciones, _f_aux);
}

long int tamanio_del_archivo(FILE * archivo){
	fseek(archivo, 0, SEEK_END);
	long int tamanio_archivo = ftell(archivo);
	fseek(archivo, 0, SEEK_SET);
	return tamanio_archivo;
}

void imprimir_parametros(void *param){
	uint32_t numero = (uint32_t)atoi(param);
	printf("%d", numero);
}

void imprimir_lista(void * var){
	instruccion *alo = (instruccion *) var;

	printf("%d\n", alo->cod_op);
	int i = 0;
	int cant = alo->tam_param/sizeof(uint32_t);
	while(i<cant){
		printf("num: %d", alo->parametros[i]);
		i++;
	}
}

void instruccion_destroyer(void* elem){
	instruccion* una_instruccion = (instruccion*) elem;
	free(una_instruccion->parametros);
	free(una_instruccion);
}

uint32_t serializar_instruccion(t_buffer_instrucciones* buffer, instruccion* instruccion, uint32_t offset){
	buffer->size += sizeof(uint32_t) * 2 + instruccion->tam_param; // Parámetros
	int cant = instruccion->tam_param/sizeof(uint32_t);

	buffer->stream = (void *)realloc(buffer->stream, offset + (2*sizeof(uint32_t)) + cant );

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


void* serializar_instrucciones(t_list * lista_instrucciones, int socket){
	t_buffer_instrucciones* buffer = malloc(sizeof(t_buffer_instrucciones));
	buffer->size = 0;
	buffer->stream = NULL;
	uint32_t offset = 0;

	void _f_aux(void* instruccion_recibida){
		instruccion * ins = (instruccion *) instruccion_recibida;
		offset = serializar_instruccion(buffer, ins, offset);
	}

	list_iterate(lista_instrucciones, _f_aux);
	





	t_paquete_instrucciones* paquete = malloc(sizeof(t_paquete_instrucciones));
	paquete->codigo_operacion_de_paquete = OPERACION_ENVIO_INSTRUCCIONES;
	paquete->buffer = buffer;

	//                        lista  codigo_operacion_de_paquete   tamaño_lista
	void* a_enviar = malloc(buffer->size + sizeof(uint32_t) + sizeof(uint32_t));
	offset = 0;

	memcpy(a_enviar + offset, &(paquete->codigo_operacion_de_paquete), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);
	
	send(socket, a_enviar, buffer->size + sizeof(uint32_t) + sizeof(uint32_t), 0);

	// No nos olvidamos de liberar la memoria que ya no usaremos
	free(a_enviar);
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);

	
	return buffer;
}
