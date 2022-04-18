#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <commons/string.h>
#include <commons/collections/list.h>

typedef struct INSTRUCCION {
	char* codigo_op;
	uint32_t* argumentos = list_create();
} instruccion;

int main (int argc, char** argv) {
	if (argc < 3) return 1; //ERROR: argumentos insuficientes
	char* path = argv[1];
	int tamanio = atoi(argv[2]);
	FILE* archivo_instrucciones = fopen(path, "rt");

	instruccion* lista_instrucciones = NULL;
	instruccion una_instruccion;
	char* todas_las_instrucciones = string_new();
	char** lista_instrucciones_como_strings = NULL;
	char** instruccion_separada_en_strings = NULL;

	fread(todas_las_instrucciones, sizeof(&archivo_instrucciones), 1, archivo_instrucciones);

	lista_instrucciones_como_strings = string_split(todas_las_instrucciones, "\n");

	int i = 0, j = 1;
	while(lista_instrucciones_como_strings[i]) {
		instruccion_separada_en_strings = string_split(lista_instrucciones_como_strings[i], " ");

		strcpy(una_instruccion.codigo_op, instruccion_separada_en_strings[0]);

		while(instruccion_separada_en_strings[j]) {
			una_instruccion.argumentos = uint32_t malloc
			una_instruccion.argumentos[j-1] = atoi(instruccion_separada_en_strings[j]);
		}

		i++;
		j = 1;
	}

	return 0;
}
