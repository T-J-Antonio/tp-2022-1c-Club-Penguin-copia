#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/collections/list.h>

#define EXITO 0
#define ERROR_ARGUMENTOS 1
#define ERROR_ARCHIVO 2
#define ERROR_OP_DESCONOCIDA -2

#define OPERACION_ENVIO_INSTRUCCIONES 0

typedef struct
{
	uint32_t size;
	void* stream;						//Stream contiene dentro, el tamanio en memoria que va a ocupar el proceso de instrucciones (Que es arbitrario, no sabemos con exactitud cuanto sera), y el stream, o sea, el conjunto de instrucciones.
} t_buffer;

typedef struct
{
	uint32_t codigo_operacion_de_paquete;
	t_buffer* buffer;
} t_paquete;

enum operaciones
{
	NO_OP = 0,
	I_O = 1,
	READ = 2,
	WRITE = 3,
	COPY = 4,
	EXIT = 5
};

typedef struct
{
	uint32_t cod_op;
	uint32_t tam_param;
	uint32_t *parametros;
} instruccion;

//FUNCIONES TRAÍDAS DEL TP0

//Dados IP y puerto del servidor, realiza el handshake con él para establecer la conexión
int crear_conexion(char* ip, char* puerto);

//Cierra el socket pasado por parámetro
void liberar_conexion(int socket_cliente);

//Libera el paquete y sus contenidos
void eliminar_paquete(t_paquete* paquete);


//FUNCIONES PROPIAS

//Dada una operación en formato string, devuelve su código de operación (enum)
uint32_t convertir_instruccion(char*);

//Dada una instrucción en formato string, la traduce a struct instrucción y lo agrega a la lista
uint32_t agregar_una_instruccion(t_list *, void *, uint32_t);

//Dada la lista de instrucciones en formato string, devuelve la lista de structs instrucción
void agregar_instrucciones(t_list * , char** );

//Dado un archivo, devuelve su tamaño
long int tamanio_del_archivo(FILE *);

//Auxiliar para destruir una instruccion de la lista de instrucciones. Para destruir todas las instrucciones de la lista, se hace un iterate con list_destroy_and_destroy_elements
void instruccion_destroyer(void* elem);

//Dada una instrucción, la añade al buffer parte por parte y retorna el offset generado por esta adición
uint32_t serializar_instruccion(t_buffer*, instruccion*, uint32_t);

//Dada una lista de instrucciones, construye un buffer con ellas y lo retorna
t_buffer * serializar_instrucciones(t_list *, uint32_t);

//Dado un buffer, un socket y un código de operación, crea un paquete utilizando el buffer,
//le coloca el código de operación como header, y lo envía a través del socket
void empaquetar_y_enviar(t_buffer*, int, uint32_t);
