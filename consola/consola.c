#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <commons/string.h>
#include <commons/collections/list.h>

#define ERROR_ARGUMENTOS 1
#define ERROR_ARCHIVO 2

typedef struct INSTRUCCION{
	char* identificador;
	t_list *parametros;
}instruccion;




void imprimir_parametros(void *param);
void imprimir_lista(void * var);
void parametro_destroyer(void* elem);
void instruccion_destroyer(void* elem);
long int tamanio_del_archivo(FILE *);
void agregar_instrucciones(t_list * , char** );




int main (int argc, char** argv) {
	if (argc != 3) return ERROR_ARGUMENTOS; //ERROR: cant. errónea de argumentos

	char* path = argv[1];
	int tamanio = atoi(argv[2]);

	t_list * lista_instrucciones = list_create();

	char* archivo_string = NULL;
	char** archivo_dividido = NULL;

	FILE* archivo_instrucciones = fopen(path, "rt");
	if (!archivo_instrucciones) return ERROR_ARCHIVO; //ERROR: el archivo no pudo abrirse
	long int tamanio_archivo = tamanio_del_archivo(archivo_instrucciones);

	archivo_string = malloc(tamanio_archivo + 1);
	fread(archivo_string, tamanio_archivo, 1, archivo_instrucciones);
	fclose(archivo_instrucciones);

	archivo_dividido = string_split(archivo_string, "\n");

	agregar_instrucciones(lista_instrucciones, archivo_dividido);

	list_iterate(lista_instrucciones, imprimir_lista);

	free(archivo_string);
	free(archivo_dividido);
	list_destroy_and_destroy_elements(lista_instrucciones, instruccion_destroyer);

	return 0;
}

void agrego_instruccion(t_list * lista_ins, void * param){
	char * inst = (char *) param;
	instruccion *instruccion_aux =  malloc(sizeof(instruccion));

	char **instrucciones = string_split(inst, " ");
	free(param);

	instruccion_aux->identificador = malloc(strlen(instrucciones[0])+1);
	strcpy(instruccion_aux->identificador, instrucciones[0]);
	free(instrucciones[0]);

	instruccion_aux->parametros = list_create();
	int j =1;
	while(instrucciones[j]) {
		void *parametro_aux = malloc(strlen(instrucciones[j])+1);

		strcpy(parametro_aux, instrucciones[j]);
		free(instrucciones[j]);
		list_add(instruccion_aux->parametros, parametro_aux);

		j++;
	}

	list_add(lista_ins, instruccion_aux);
	free(instrucciones);
}


void agregar_instrucciones(t_list * lista_ins, char** instrucciones){
	void _f_aux(char *elem ){
		agrego_instruccion(lista_ins, elem);
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

	printf("%s\n", alo->identificador);
	list_iterate(alo->parametros, imprimir_parametros);
}

void parametro_destroyer(void* elem){
	uint32_t* un_parametro = (uint32_t*) elem;
	free(un_parametro);
}

void instruccion_destroyer(void* elem){
	instruccion* una_instruccion = (instruccion*) elem;
	free(una_instruccion->identificador);
	list_destroy_and_destroy_elements(una_instruccion->parametros, parametro_destroyer);
	free(una_instruccion);
}
