#include "utilsConsola.h"


int main (int argc, char** argv) {
	if (argc != 3) return ERROR_ARGUMENTOS; 											//ERROR: cant. errónea de argumentos
	char* path = argv[1];
	uint32_t tamanio_en_memoria = (uint32_t) atoi(argv[2]);														// Pasar tam proceso al kernel
	uint32_t resultado;

	t_list * lista_instrucciones = list_create();										//creo la lista que va a contener todos los struct instruccion

	FILE* archivo_instrucciones = fopen(path, "rt"); 									//abro el archivo entero
	if (!archivo_instrucciones) return ERROR_ARCHIVO; 									//ERROR: el archivo no pudo abrirse
	long int tamanio_archivo = tamanio_del_archivo(archivo_instrucciones); 				//veo su tamaño

	char* archivo_string = malloc(tamanio_archivo + 1); 								//asigno memoria donde va a parar lo leído
	fread(archivo_string, tamanio_archivo, 1, archivo_instrucciones); 					//leo
	if(archivo_string == NULL) return ERROR_ARCHIVO;
	archivo_string[tamanio_archivo] = '\0';												//arreglo de una posible catástrofe
	fclose(archivo_instrucciones);														//cierro


	char** archivo_dividido = string_split(archivo_string, "\n");						//separo lo leído por línea ya que cada línea es una instrucción y prosigo a liberar la memoria de lo leído
	free(archivo_string);

	agregar_instrucciones(lista_instrucciones, archivo_dividido);						//función para ir agregando cosas a la lista de structs seguido de la liberación del archivo dividido
	free(archivo_dividido);


	t_buffer* buffer = serializar_instrucciones(lista_instrucciones, tamanio_en_memoria);
	t_config* config = config_create("/home/utnso/Documentos/tp-2022-1c-Club-Penguin/consola/consola.config");
	char* ip = config_get_string_value(config, "KERNEL_IP");
	char* port = config_get_string_value(config, "KERNEL_PORT");


	int conexion = crear_conexion(ip, port);
	empaquetar_y_enviar(buffer, conexion, OPERACION_ENVIO_INSTRUCCIONES);

	list_destroy_and_destroy_elements(lista_instrucciones, instruccion_destroyer);
	recv(conexion, &resultado, sizeof(uint32_t), MSG_WAITALL);
	liberar_conexion(conexion);

	config_destroy(config);
	return EXITO;
}
