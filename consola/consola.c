#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <commons/string.h>
#include <commons/collections/list.h>

typedef struct INSTRUCCION{
	char* identificador;
	t_list *parametros;
}instruccion;

void imprimir_parametros(void *param){
	uint32_t numero = (uint32_t)atoi(param);
	printf("%d", numero);
}

void imprimir_lista(void * var){
	instruccion *alo = (instruccion *) var;

	printf("%s\n", alo->identificador);
	list_iterate(alo->parametros, imprimir_parametros);
}

int main (int argc, char** argv) {
	if (argc != 3) return 1; //ERROR: cant. errónea de argumentos
	char* path = argv[1];
	int tamanio = atoi(argv[2]);
	FILE* archivo_instrucciones = fopen(path, "rt+");

	t_list * lista_instrucciones = list_create();

	char* archivo_string = string_new();
	char** archivo_dividido = NULL;
	char** instrucciones = NULL;

	fseek(archivo_instrucciones, 0, SEEK_END);
	long int tamanio_archivo = ftell(archivo_instrucciones);
	fseek(archivo_instrucciones, 0, SEEK_SET);

	archivo_string = malloc(tamanio_archivo + 1);
	fread(archivo_string, tamanio_archivo, 1, archivo_instrucciones);

	archivo_dividido = string_split(archivo_string, "\n");

	int i = 0, j = 1;
	while(archivo_dividido[i]) {
		instruccion *instruccion_aux =  malloc(sizeof(instruccion));

		instrucciones = string_split(archivo_dividido[i], " ");
		instruccion_aux->identificador = malloc(strlen(instrucciones[0])+1);
		strcpy(instruccion_aux->identificador, instrucciones[0]);

		instruccion_aux->parametros = list_create();

		while(instrucciones[j]) {
			void *parametro_aux = malloc(strlen(instrucciones[j])+1);

			strcpy(parametro_aux, instrucciones[j]);
			list_add(instruccion_aux->parametros, parametro_aux);
			j++;
		}

		list_add(lista_instrucciones, instruccion_aux);

		i++;
		j = 1;
	}

	list_iterate(lista_instrucciones, imprimir_lista);

	return 0;
}
