#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <commons/string.h>
#include <commons/collections/list.h>

typedef struct PARAMETROS{
	uint32_t parametro;
	struct PARAMETROS *sgt;
}parametros;

typedef struct INSTRUCCION {
	char* codigo_op;
	parametros *param;
} instruccion;

typedef struct INSTRUCCIONES {
	instruccion instruccion_nodo;
	struct INSTRUCCIONES *sgt;
} instrucciones;






void insertar_argument(struct PARAMETROS **nodo, uint32_t param){
	parametros *nuevo_nodo = (parametros*) malloc(sizeof(parametros));
	nuevo_nodo->sgt = NULL;
	nuevo_nodo->parametro = param;

	parametros *last = *nodo;

	if(*nodo == NULL){
		*nodo = nuevo_nodo;
	}
	else{
		while(last->sgt != NULL){
			last = last->sgt;}
		if(last->sgt == NULL){
			last->sgt = nuevo_nodo;
		}
	}

}

void insertar_instruccion(struct INSTRUCCIONES **nodo, instruccion una_instruccion){
	instrucciones *nuevo_nodo = (instrucciones*) malloc(sizeof(instrucciones));
	nuevo_nodo->sgt = NULL;
	nuevo_nodo->instruccion_nodo = una_instruccion;

	instrucciones *last = *nodo;

	if(*nodo == NULL){
		*nodo = nuevo_nodo;
	}
	else{
		while(last->sgt != NULL){
			last = last->sgt;}
		if(last->sgt == NULL){
				last->sgt = nuevo_nodo;
			}
		}

}

int main () {
	//if (argc < 2) return 1; //ERROR: argumentos insuficientes
	//char* path = argv[1];
	//int tamanio = atoi(argv[2]);
	FILE* archivo_instrucciones = fopen("/home/utnso/Documentos/tp-2022-1c-Club-Penguin/consola/ins.txt", "rt+");

	instrucciones* lista_instrucciones = NULL;
	instruccion una_instruccion;
	char* todas_las_instrucciones = string_new();
	char** lista_instrucciones_como_strings = NULL;
	char** instruccion_separada_en_strings = NULL;

	fseek(archivo_instrucciones, 0, SEEK_END);

	long int sizef = ftell(archivo_instrucciones);
	fseek(archivo_instrucciones, 0, SEEK_SET);
	todas_las_instrucciones = malloc(sizef + 1);
	fread(todas_las_instrucciones, sizef, 1, archivo_instrucciones);

	lista_instrucciones_como_strings = string_split(todas_las_instrucciones, "\n");

	int i = 0, j = 1;
	while(lista_instrucciones_como_strings[i]) {
		instruccion_separada_en_strings = string_split(lista_instrucciones_como_strings[i], " ");

		strcpy(una_instruccion.codigo_op, instruccion_separada_en_strings[0]);
		parametros* params = NULL;
		while(instruccion_separada_en_strings[j]) {
			uint32_t temp = (uint32_t)atoi(instruccion_separada_en_strings[j]);
			insertar_argument(&params, temp);
			j++;
		}
		una_instruccion.param = params;

		insertar_instruccion(&lista_instrucciones, una_instruccion);
		i++;
		j = 1;
	}

	while(lista_instrucciones){
		printf("instruccion: %s\n", lista_instrucciones->instruccion_nodo.codigo_op);
		while(lista_instrucciones->instruccion_nodo.param){
			printf("parametro: %d\n", lista_instrucciones->instruccion_nodo.param->parametro);
			lista_instrucciones->instruccion_nodo.param = lista_instrucciones->instruccion_nodo.param->sgt;
		}
		lista_instrucciones = lista_instrucciones->sgt;
	}

	return 0;
}
