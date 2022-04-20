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

void list_print(void * var){
	instruccion *alo = (instruccion *) var;

	printf("%s\n", alo->identificador);
	list_iterate(alo->parametros, imprimir_parametros);
}

int main (int argc, char** argv) {
	if (argc != 3) return 1; //ERROR: argumentos insuficientes
	char* path = argv[1];
	//int tamanio = atoi(argv[2]);
	FILE* archivo_instrucciones = fopen(path, "rt+");


	t_list * instruction_list = list_create();

	char* archivo = string_new();
	char** archivo_split = NULL;
	char** instrucciones = NULL;

	fseek(archivo_instrucciones, 0, SEEK_END);
	long int sizef = ftell(archivo_instrucciones);
	fseek(archivo_instrucciones, 0, SEEK_SET);

	archivo = malloc(sizef + 1);
	fread(archivo, sizef, 1, archivo_instrucciones);

	archivo_split = string_split(archivo, "\n");

	int i = 0, j = 1;
	while(archivo_split[i]) {

		instruccion *temp_val =  malloc(sizeof(instruccion));

		instrucciones = string_split(archivo_split[i], " ");
		temp_val->identificador = malloc(strlen(instrucciones[0])+1);
		strcpy(temp_val->identificador, instrucciones[0]);

		temp_val->parametros = list_create();


		while(instrucciones[j]) {

			void *temp = malloc(strlen(instrucciones[j])+1);

			strcpy(temp, instrucciones[j]);
			list_add(temp_val->parametros, temp);
			j++;
		}

		list_add(instruction_list, temp_val);

		i++;
		j = 1;
	}
	list_iterate(instruction_list, list_print);


	return 0;
}
