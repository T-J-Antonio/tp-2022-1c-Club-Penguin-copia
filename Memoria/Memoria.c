#include "utilsMemoria.h"

int cantidad_de_marcos_totales;
int tam_memoria;
int tam_pagina;
int entradas_por_tabla;
int cliente_cpu;
int cliente_kernel;
void* espacio_memoria_user;
int marcos_por_proceso;
char* path_swap = "";
char* alg_reemplazo;

t_list* lista_global_de_tablas_de_1er_nivel;
t_list* lista_global_de_tablas_de_2do_nivel;
t_dictionary* diccionario_pid;
t_dictionary* diccionario_marcos;
t_dictionary* diccionario_swap;
uint32_t* estado_de_marcos;
sem_t mutex_swap;



//cosas a tener en cuenta: 
// y kernel solo debe borrar lo administrativo al final de un proceso ya que de la forma que esta el codigo nos evitamos tener multilpes estructuras administrativas clonadas solo controlar que no pase que las mismas se generen multiples veces

// Hay que tener una estructura auxiliar, por proceso para gestionar el algoritmo de reemplazo y saber que marcos pertenecen a cada proceso HECHOOOO!!!
// Tambien tiene que haber una especie de mapa con los marcos libres para manejarlos facilmente y no recorrer mucho buscandolos HECHOOOO!!!
// Hay que armar el swap y ser capaces de escribir ahi con mmap HECHO!!!

// Y faltan los ALGOS DE remplazo Hecho!!!
// falta liberacion de memoria TODO:::!!!!

// Tenemos que ser capaces de volcar todo a swap en caso de que un proceso se suspenda y remover esas estructuras administrativas,  HECHOOOO!!!
// luego cuando se pase a ready hay que recibir ninguna noti para cargar lo necesario a memoria HECHOOOO!!!

// para la lista de marcos usados por proceso voy a hacer un dict con el pid y un struct que tenga el vector para meter marcos, el vector usa -1 si es posicion libre o el num de marco si posee y su espacio se inicializa con tam int * marcos por proceso,
// y tambien el puntero "offset" que usaremos para los algoritmos de reemplazo
// la lista total de marcos libres es mas facil le hago un vector de bitmap 
// cuando se suspende un proceso se debe vaciar la lista de marcos usados por ese proceso y pasarlos a libres en el total, en caso de eliminar aparte deberiamos destruir la estructura de marcos usados por ese proceso
// una f aux liberar marcos es util toma el pid y maneja el vaciado.

int main(){
	t_config* config = config_create("/home/utnso/Documentos/tp-2022-1c-Club-Penguin/Memoria/memoria.config");
	logger = log_create("Memoria.log", "Memoria", 1, LOG_LEVEL_DEBUG);

	sem_init(&mutex_swap, 0, 1);

	char* ip_memoria = config_get_string_value(config, "IP_MEMORIA");
	char* puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");

	tam_memoria = config_get_int_value(config, "TAM_MEMORIA");
	tam_pagina = config_get_int_value(config, "TAM_PAGINA");
	cantidad_de_marcos_totales = tam_memoria / tam_pagina;

	estado_de_marcos = malloc(sizeof(uint32_t) * cantidad_de_marcos_totales); //1 es libre, 0 es ocupado. Esto no es de suma utilidad es solo mas conceptual al principio para tomar de aca el primer marco que encuentre libre para darselo a un proceso luego los reemplazos son locales y no vamos a tener un grado de multi que sea conflictivo con esto.
	for(int i = 0; i < cantidad_de_marcos_totales; i++){
		estado_de_marcos[i] = 1;
	}
	espacio_memoria_user = malloc(tam_memoria);
	path_swap = config_get_string_value(config, "PATH_SWAP");
	marcos_por_proceso = config_get_int_value(config, "MARCOS_POR_PROCESO");

	alg_reemplazo = config_get_string_value(config, "ALGORITMO_REEMPLAZO");

	retardo_swap = config_get_int_value(config, "RETARDO_SWAP");
	retardo_mem = config_get_int_value(config, "RETARDO_MEMORIA");

	entradas_por_tabla = config_get_int_value(config, "ENTRADAS_POR_TABLA");
	diccionario_pid = dictionary_create();
	diccionario_marcos = dictionary_create();
	diccionario_swap = dictionary_create();
	lista_global_de_tablas_de_1er_nivel= list_create();
	lista_global_de_tablas_de_2do_nivel = list_create();


	int socket_memoria_escucha = iniciar_servidor(ip_memoria, puerto_escucha);

	log_info(logger, "memoria lista para recibir al request\n");
	pthread_t socket_escucha_memoria;

	void* _f_aux_escucha(void* socket_memoria) {
		escuchar(*(int*)socket_memoria);
		return NULL;
	}

	pthread_create(&socket_escucha_memoria, NULL, _f_aux_escucha, (void*) &socket_memoria_escucha);

	pthread_join(socket_escucha_memoria, NULL);
	return 0;
}

void* atender_cpu(void* nada){
	int cod_ok = 1;
	// envíos iniciales de datos necesarios para el CPU
	send(cliente_cpu, &tam_pagina, sizeof(uint32_t), 0);
	send(cliente_cpu, &entradas_por_tabla, sizeof(uint32_t), 0);

	uint32_t tabla_paginas;
	uint32_t entrada_tabla_1er_nivel;
	uint32_t index_tabla_2do_nivel;
	uint32_t entrada_tabla_2do_nivel;
	uint32_t marco;
	uint32_t dato_leido;
	uint32_t dato_a_escribir;
	uint32_t direccion_fisica;
	uint32_t pid;
	uint32_t nro_pag;

	while(1){
		int codigo_de_paquete = recibir_operacion(cliente_cpu);
		switch(codigo_de_paquete) {
		case ACCESO_A_1RA_TABLA:
			recv(cliente_cpu, &tabla_paginas, sizeof(uint32_t), MSG_WAITALL);
			recv(cliente_cpu, &entrada_tabla_1er_nivel, sizeof(uint32_t), MSG_WAITALL);
			//printf("tabla_paginas: %d\n", tabla_paginas);
			//printf("entrada_tabla_1er_nivel: %d\n", entrada_tabla_1er_nivel);
			index_tabla_2do_nivel = respuesta_a_pregunta_de_1er_acceso(tabla_paginas, entrada_tabla_1er_nivel);
			send(cliente_cpu, &index_tabla_2do_nivel, sizeof(int), 0);
			break;

		case ACCESO_A_2DA_TABLA: //voy a tener que recibir el pid tmb para facilitar el reemplazo bastante
			recv(cliente_cpu, &index_tabla_2do_nivel, sizeof(uint32_t), MSG_WAITALL);
			recv(cliente_cpu, &entrada_tabla_2do_nivel, sizeof(uint32_t), MSG_WAITALL);
			recv(cliente_cpu, &pid, sizeof(uint32_t), MSG_WAITALL);		//aca tambien tengo que ver si remplaza o no para avisar
			marco = respuesta_a_pregunta_de_2do_acceso(index_tabla_2do_nivel, entrada_tabla_2do_nivel, pid, cliente_cpu); //aca si fue necesario reemplazar un marco hay que avisarle a la tlb
			send(cliente_cpu, &marco, sizeof(uint32_t), 0);
			break;

		case LECTURA_EN_MEMORIA:
			recv(cliente_cpu, &direccion_fisica, sizeof(uint32_t), MSG_WAITALL);
			recv(cliente_cpu, &pid, sizeof(uint32_t), MSG_WAITALL);
			recv(cliente_cpu, &nro_pag, sizeof(uint32_t), MSG_WAITALL);
			dato_leido = leer_posicion(direccion_fisica, pid, nro_pag );
			send(cliente_cpu, &dato_leido, sizeof(uint32_t), 0);
			break;

		case ESCRITURA_EN_MEMORIA:
			recv(cliente_cpu, &direccion_fisica, sizeof(uint32_t), MSG_WAITALL);
			recv(cliente_cpu, &dato_a_escribir, sizeof(uint32_t), MSG_WAITALL);
			recv(cliente_cpu, &pid, sizeof(uint32_t), MSG_WAITALL);
			recv(cliente_cpu, &nro_pag, sizeof(uint32_t), MSG_WAITALL);
			escribir_en_posicion(direccion_fisica, dato_a_escribir, pid, nro_pag);
			send(cliente_cpu, &cod_ok, sizeof(uint32_t), 0);

			break;
			
		}
	}
	return NULL;
}
//tanto en escritura como lectura tengo que ser capaz de agarrar la pag del proceso para poder
//setear el bit de uso o mod en 1 en caso de que sea acceso directo por tlb o no ya que sino no se volvaria nunca a swap
uint32_t leer_posicion(uint32_t direccion_fisica, uint32_t pid, uint32_t nro_pag ){
	uint32_t dato_leido;
	usleep((useconds_t)retardo_mem * (useconds_t)1000);
	memcpy(&dato_leido, espacio_memoria_user + direccion_fisica, sizeof(uint32_t));   // lee 1 uint o lee la pagina entera?
	tabla_de_segundo_nivel* tabla = get_tabla(pid, nro_pag);
	tabla->bit_de_uso =1;
	//printf("dato leido %d\n", dato_leido);
	return dato_leido;
}


void escribir_en_posicion(uint32_t direccion_fisica, uint32_t dato_a_escribir, uint32_t pid, uint32_t nro_pag ){
	usleep((useconds_t)retardo_mem * (useconds_t)1000);
	memcpy(espacio_memoria_user + direccion_fisica, &dato_a_escribir, sizeof(uint32_t));
	tabla_de_segundo_nivel* tabla = get_tabla(pid, nro_pag);
	tabla->bit_de_uso =1;
	tabla->bit_modificado =1;
	log_info(logger, "fue escrito dato: %d\n", dato_a_escribir);
}


tabla_de_segundo_nivel* get_tabla(uint32_t pid, uint32_t nro_pag){
	uint32_t index_primer_tabla = nro_pag / entradas_por_tabla;
	uint32_t index_segunda_tabla = nro_pag % entradas_por_tabla;
	estructura_administrativa_de_marcos* admin = (estructura_administrativa_de_marcos*) dictionary_get(diccionario_marcos, string_itoa(pid));


	t_list* tabla_1er_nivel = list_get(lista_global_de_tablas_de_1er_nivel, admin->pagina_primer_nivel);
	int index_tabla_2do_nivel = *(int*) list_get(tabla_1er_nivel, index_primer_tabla);
	t_list* tabla_2do_nivel = list_get(lista_global_de_tablas_de_2do_nivel, index_tabla_2do_nivel);
	tabla_de_segundo_nivel* dato = (tabla_de_segundo_nivel*) list_get(tabla_2do_nivel, index_segunda_tabla);

	return dato;
}


void* atender_kernel(void* input){
	int cliente_fd = *(int *) input;
	// atender cualquier msg del kernel
	int rtaOk = 1;
	//printf("voy a esperar al kernel\n");
	while(1){
		int codigo_de_paquete = recibir_operacion(cliente_fd);
		int tam_mem;
		uint32_t p_aux;
		uint32_t pid;
		uint32_t tam_memoria;
		uint32_t index;

		switch(codigo_de_paquete) {
		case CREAR_PROCESO:
			pid = recibir_operacion(cliente_fd);
			tam_mem = recibir_operacion(cliente_fd); //aca hay que recibir mas cosas minimo entiendo q el pid para usarlo en el swap, y armar el swap
			p_aux = crear_proceso(tam_mem, pid);
			send(cliente_fd, &p_aux, sizeof(uint32_t), 0); //este send no pasa
			break;
		case FINALIZAR_PROCESO:
			index = recibir_operacion(cliente_fd);
			tam_memoria = recibir_operacion(cliente_fd);
			eliminar_proceso(index, tam_memoria);
			send(cliente_fd, &rtaOk, sizeof(u_int32_t), 0);

		break;

		case REANUDAR_PROCESO:
			pid = recibir_operacion(cliente_fd);
			index = recibir_operacion(cliente_fd);
			reanudar_proceso(pid, index);
			send(cliente_fd, &rtaOk, sizeof(u_int32_t), 0);
			log_info(logger, "Proceso reanudado");
		break;

		case SUSPENDER_PROCESO:
			index = recibir_operacion(cliente_fd);
			suspender_proceso(index);
			send(cliente_fd, &rtaOk, sizeof(u_int32_t), 0);
			log_info(logger, "Suspendi proceso");
		break;
		}
	}
	return NULL;
}

void* escuchar(int socket_memoria_escucha){

	pthread_t thread_type;
	cliente_cpu = esperar_cliente(socket_memoria_escucha);
	pthread_create(&thread_type, NULL, atender_cpu, (void*) &cliente_cpu );

	cliente_kernel = esperar_cliente(socket_memoria_escucha);
	pthread_t thread_kernel;
	pthread_create(&thread_kernel, NULL, atender_kernel, (void*) &cliente_kernel );
	pthread_join(thread_kernel, NULL);
 	return NULL;
}







//--------------------------------gestion de swap--------------------------------------------------------------------------------

// aca dentro de leer y escribir puedo meter los delays

void crear_swap(int pid, int cantidad_de_marcos){
	sem_wait(&mutex_swap);
	swap_struct* swp = malloc(sizeof(swap_struct));
	
	swp->path_swap = malloc(strlen(path_swap) + strlen(string_itoa(pid)) + 5);
	memcpy(swp->path_swap, path_swap, strlen(path_swap));
	memcpy(swp->path_swap + strlen(path_swap), string_itoa(pid), strlen(string_itoa(pid)));
	memcpy(swp->path_swap + strlen(path_swap) + strlen(string_itoa(pid)), ".txt", strlen(".txt")+1);	
	log_info(logger, "El path de este archivo de swap es %s\n", swp->path_swap);
	int swp_file = open (swp->path_swap, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600 );
	off_t tam = cantidad_de_marcos * tam_pagina;
	ftruncate(swp_file, tam); //esto lo uso para agrandar el archivo a esa cantidad de bits
 	swp->swap_map = mmap (0, tam, PROT_READ | PROT_WRITE, MAP_SHARED, swp_file, 0);
   	
 
	dictionary_put(diccionario_swap, string_itoa(pid), (void*)swp);
	close(swp_file);
	sem_post(&mutex_swap);
}



void volcar_pagina_en_swap(uint32_t pid, uint32_t dezplazamiento, void* dato){
	sem_wait(&mutex_swap);
	//retardo de escritura
	log_info(logger, "accesso a swap de escritura");
	usleep((useconds_t)retardo_swap * (useconds_t)1000);
	swap_struct* swap_map = (swap_struct *) dictionary_get(diccionario_swap, string_itoa(pid));
	memcpy(swap_map->swap_map + dezplazamiento, dato, tam_pagina);
	sem_post(&mutex_swap);
}

void leer_pagina_de_swap(uint32_t pid, uint32_t dezplazamiento, void* dato){
	//retardo de lectura
	sem_wait(&mutex_swap);
	usleep((useconds_t)retardo_swap * (useconds_t)1000);
	log_info(logger, "accesso a swap de lectura");
	swap_struct* swap_map = (swap_struct *) dictionary_get(diccionario_swap, string_itoa(pid));
	memcpy(dato, swap_map->swap_map + dezplazamiento, tam_pagina);
	sem_post(&mutex_swap);
}

void eliminar_swap(uint32_t pid, uint32_t memoria_total){
	sem_wait(&mutex_swap);
	swap_struct* swap_map = (swap_struct *) dictionary_remove(diccionario_swap, string_itoa(pid));
	munmap(swap_map->swap_map, memoria_total); 
	
	remove(swap_map->path_swap);
	
	free(swap_map->path_swap);
	free(swap_map);
	sem_post(&mutex_swap);

}





//---------------------------------------Gestion de procesos, ready, sus terminate----------------------------------------------------------------

void crear_tablas_de_2do_nivel(int cantidad_de_entradas_de_paginas_2do_nivel, t_list* tabla_de_1er_nivel){
	int iterador = 0;
	int contador = 0;
	uint32_t cantidad_de_entradas_de_paginas_1er_nivel = cantidad_de_entradas_de_paginas_2do_nivel / entradas_por_tabla;
	if((cantidad_de_entradas_de_paginas_2do_nivel % entradas_por_tabla) > 0){
		cantidad_de_entradas_de_paginas_1er_nivel++;
	}

	while(iterador < cantidad_de_entradas_de_paginas_1er_nivel){
		t_list* lista_de_paginas_2do_nivel = list_create();
		for(int i = 0; i < entradas_por_tabla && contador < cantidad_de_entradas_de_paginas_2do_nivel; i++){
			tabla_de_segundo_nivel* pagina = malloc(sizeof(tabla_de_segundo_nivel));
			pagina->marco = -1; // Es -1 ya que no tiene ningun marco asignado el proceso cuando recien se crea
			pagina->numero_pagina = contador;
			pagina->bit_presencia = 0;
			pagina->bit_de_uso = 0;
			pagina->bit_modificado = 0;
			list_add(lista_de_paginas_2do_nivel, pagina);
			contador++;
		}
		uint32_t index = list_size(lista_global_de_tablas_de_2do_nivel);
		int* numero = malloc(sizeof(uint32_t));
		memcpy(numero, &index, sizeof(uint32_t));
		list_add(lista_global_de_tablas_de_2do_nivel, lista_de_paginas_2do_nivel);
		list_add(tabla_de_1er_nivel, numero);
		iterador++;
	}
}


void reanudar_proceso(uint32_t pid, uint32_t primera_pag){
	estructura_administrativa_de_marcos* admin = malloc(sizeof(estructura_administrativa_de_marcos));
	admin->pagina_primer_nivel = primera_pag;
	admin->offset = 0;
	admin->marcos_asignados = malloc(sizeof(int) * marcos_por_proceso);
	for(int i = 0; i < marcos_por_proceso; i++){
		admin->marcos_asignados[i] =-1;
	}
	dictionary_put(diccionario_marcos, string_itoa(pid),(void*) admin);
}

uint32_t crear_proceso(int tamanio_en_memoria, int pid){ // tengo que iniciar las nuevas estrucutras tmb
	uint32_t cantidad_de_entradas_de_paginas_2do_nivel = tamanio_en_memoria / tam_pagina;
	if((tamanio_en_memoria % tam_pagina) > 0){
		cantidad_de_entradas_de_paginas_2do_nivel++;
	}
	crear_swap(pid, cantidad_de_entradas_de_paginas_2do_nivel);
	t_list* lista_1er_nivel_proceso = list_create();
	crear_tablas_de_2do_nivel(cantidad_de_entradas_de_paginas_2do_nivel, lista_1er_nivel_proceso);
	uint32_t tabla_pags = list_size(lista_global_de_tablas_de_1er_nivel); // retornar el index de la tabla de paginas al kernel para que se agregue a la pcb
	list_add(lista_global_de_tablas_de_1er_nivel, lista_1er_nivel_proceso);

	int* pid_perm = malloc(sizeof(uint32_t));
	memcpy(pid_perm, &pid, sizeof(uint32_t));
	dictionary_put(diccionario_pid, string_itoa(tabla_pags), pid_perm);
	estructura_administrativa_de_marcos* admin = malloc(sizeof(estructura_administrativa_de_marcos));
	admin->pagina_primer_nivel = tabla_pags;
	admin->offset = 0;
	admin->marcos_asignados = malloc(sizeof(int) * marcos_por_proceso);
	for(int i = 0; i < marcos_por_proceso; i++){
		admin->marcos_asignados[i] =-1;
	}
	dictionary_put(diccionario_marcos, string_itoa(pid), (void*)admin);
	return tabla_pags;
}

void liberar_marcos(int pid){ //aca no vuelco a swap porque sino complico mas las cosas, tambien tengo que liberar en la estructura grande
	estructura_administrativa_de_marcos* admin = (estructura_administrativa_de_marcos*) dictionary_get(diccionario_marcos, string_itoa(pid));
	int i;
	free(admin->marcos_asignados);
	dictionary_remove(diccionario_marcos, string_itoa(pid));
	free(admin);
	log_info(logger, "Liberados marcos del proceso %d", pid);
}

void suspender_proceso(int index_tabla){
	int pid = *(int*)dictionary_get(diccionario_pid, string_itoa(index_tabla));

	t_list * tabla_1er_nivel = list_get(lista_global_de_tablas_de_1er_nivel, index_tabla);
	
	int tamanio_lista = list_size(tabla_1er_nivel);
	int iterador = 0;
	
	while(iterador < tamanio_lista){
		int index_2da = *(int*) list_get(tabla_1er_nivel, iterador);
		t_list * tabla_2do_nivel = list_get(lista_global_de_tablas_de_2do_nivel, index_2da); // aca hay que corregir esto, lista 1 es una lista de ints no una lista de listas
	
		int tamanio_lista_2do_nivel = list_size(tabla_2do_nivel);
		int iterator_2do_nivel = 0;
		
		while(iterator_2do_nivel < tamanio_lista_2do_nivel){
			tabla_de_segundo_nivel * pagina = list_get(tabla_2do_nivel, iterator_2do_nivel);
			if(pagina->bit_modificado == 1 && pagina->bit_presencia == 1){
				volcar_pagina_en_swap(pid, pagina->numero_pagina * tam_pagina, espacio_memoria_user + (pagina->marco *tam_pagina));
				log_info(logger, "Se lleva la Pagina %d a swap, por suspension del proceso alojada en el marco: %d", pagina->numero_pagina, pagina->marco);
				pagina->bit_de_uso = 0;
				pagina->bit_modificado = 0;
				pagina->bit_presencia = 0;
			}
			else{
				pagina->bit_de_uso = 0;
				pagina->bit_modificado = 0;
				pagina->bit_presencia = 0;
			}
			iterator_2do_nivel++;
		}
		iterador++;
	}
	liberar_marcos(pid);
	//Responder a kernel un ok;
}

void eliminar_proceso(int index_tabla, uint32_t memoria_total){ //aca hay que destruir las tablas de paginas de memoria total recibo el dato crudo hay que meter la cuenta
	int pid = *(int*)dictionary_remove(diccionario_pid, string_itoa(index_tabla));
	liberar_marcos(pid);
	uint32_t cantidad_de_entradas_de_paginas_2do_nivel = memoria_total / tam_pagina;
	if((memoria_total % tam_pagina) > 0){
		cantidad_de_entradas_de_paginas_2do_nivel++;
	}
	eliminar_swap(pid, cantidad_de_entradas_de_paginas_2do_nivel * tam_pagina);
}


//---------------------------respuestas a accesos a memoria-----------------------------------------------------------------------

int respuesta_a_pregunta_de_1er_acceso(int index_tabla, int entrada){
	//printf("index tabla: %d\n", index_tabla);
	usleep((useconds_t)retardo_mem * (useconds_t)1000);
	t_list* tabla_1er_nivel = list_get(lista_global_de_tablas_de_1er_nivel, index_tabla);
	int index_tabla_2do_nivel = *(int*) list_get(tabla_1er_nivel, entrada);
	return index_tabla_2do_nivel;
}



int primer_marco_libre_del_proceso(estructura_administrativa_de_marcos* est){
	for(int i =0; i < marcos_por_proceso; i++){
		if(est->marcos_asignados[est->offset] == -1){
			return est->offset;
		}
		else{
			est->offset++;
			est->offset = est->offset % marcos_por_proceso; //aca seria el operador modulo para no complicarnos la vida y hacer la vuelta del puntero a la pos 0
		}
	}
	return -1; //esto ya que no hay marcos libres
}

int primer_marco_libre(){
	for(int i=0; i< cantidad_de_marcos_totales; i++){
		if(estado_de_marcos[i]==1) return i;
	}
	return -1;
}


int respuesta_a_pregunta_de_2do_acceso(int index_tabla, int entrada, uint32_t pid, int cliente_cpu){ // para esto me es mas util recibir el pid del proceso para poder agarrar sus estructuras administrativas
	usleep((useconds_t)retardo_mem * (useconds_t)1000);
	t_list* tabla_2do_nivel = list_get(lista_global_de_tablas_de_2do_nivel, index_tabla);
	estructura_administrativa_de_marcos* estructura = (estructura_administrativa_de_marcos*)dictionary_get(diccionario_marcos, string_itoa(pid));
	int marco_a_asignar;
	int marco;
	int reemp = REEMPLAZO_DE_PAGINA;
	int acceso = ACCESO_A_2DA_TABLA;
	tabla_de_segundo_nivel* dato = (tabla_de_segundo_nivel*) list_get(tabla_2do_nivel, entrada); //cambiar el nombre de el struct te la debo mepa
	if(dato->bit_presencia == 1){
		log_info(logger, "La pagina estaba cargada");
		dato->bit_de_uso = 1;
		send(cliente_cpu, &acceso, sizeof(uint32_t), 0);
		return dato->marco;
	}

	else{
		marco = primer_marco_libre_del_proceso(estructura);
		if(marco == -1){
			log_info(logger, "No tenia marco libre procedo a reemplazar");
			marco_a_asignar = reemplazar_marco(estructura, pid);
			dato->marco = marco_a_asignar;
			estructura->offset++;
			estructura->offset = estructura->offset % marcos_por_proceso; //hago que el puntero apunte al siguiente para que quede como deberia
			dato->bit_presencia = 1;
			dato->bit_modificado = 0;
			dato->bit_de_uso = 1;
			send(cliente_cpu, &reemp, sizeof(uint32_t), 0);
		}
		else{
			marco_a_asignar = primer_marco_libre();
			estructura->marcos_asignados[marco] = marco_a_asignar;
			estructura->offset++;
			estructura->offset = estructura->offset % marcos_por_proceso;
			estado_de_marcos[marco_a_asignar] = 0;
			
			
			dato->marco = marco_a_asignar;
			dato->bit_presencia = 1;
			dato->bit_modificado = 0;
			dato->bit_de_uso = 1;
			send(cliente_cpu, &acceso, sizeof(uint32_t), 0);
		} //esto es si tiene marcos disponibles.
		leer_pagina_de_swap(pid, (tam_pagina * dato->numero_pagina), espacio_memoria_user + (marco_a_asignar * tam_pagina));
	}
	return dato->marco;
}


tabla_de_segundo_nivel* retornar_pagina(int indice_pagina, int marco_asignado){
	t_list* tabla_1er_nivel = (t_list*)list_get(lista_global_de_tablas_de_1er_nivel, indice_pagina);
	int cantidad_pags = list_size(tabla_1er_nivel);
	
	for(int i = 0; i< cantidad_pags; i++){
		
		int indice_tabla_dos = *(int*)list_get(tabla_1er_nivel, i);
		t_list* tabla_dos = (t_list*)list_get(lista_global_de_tablas_de_2do_nivel, indice_tabla_dos);
		int cant_entradas = list_size(tabla_dos);
		
		for(int c = 0; c< cant_entradas; c++){
			tabla_de_segundo_nivel* dato = (tabla_de_segundo_nivel*) list_get(tabla_dos, c);
			if(dato->marco == marco_asignado){
				return dato;
			}
		}
	}
	return NULL;
}

int reemplazar_marco(estructura_administrativa_de_marcos* adm, uint32_t pid ){ //esto va a tener clock y clock-m  ESTO VA A RETORNAR EL MARCO QUE SE VA A USAR. LA ASIGNACION LE CORRESPONDE A LA SUPERIOR A ESTA
	if(strcmp(alg_reemplazo, "CLOCK") == 0 ){
		
		while(true){ //ojo con los while infinitos, deberia siempre llegarle una estructura correcta pero en caso de ser llamado de forma erronea 
			
			tabla_de_segundo_nivel* dato = retornar_pagina(adm->pagina_primer_nivel, adm->marcos_asignados[adm->offset]);
					
			if(dato->bit_de_uso ==0){
				dato->bit_presencia = 0; 
				dato->bit_de_uso = 0;
				log_info(logger, "reemplazo el marco %d", adm->marcos_asignados[adm->offset]);
				log_info(logger, "que tiene la pagina %d", dato->numero_pagina);
				if(dato->bit_modificado){
					volcar_pagina_en_swap(pid, (dato->numero_pagina * tam_pagina), espacio_memoria_user + (dato->marco * tam_pagina));
					dato->bit_modificado = 0;
				}
				return adm->offset; // ojo el puntero deberia quedar apuntando al siguiente
			}
			else{
				dato->bit_de_uso = 0;
			}

		adm->offset++;
		adm->offset = adm->offset % marcos_por_proceso;
		}
	}
	else{//aca va CLOCK-M
		int iterador = 0;
		while(true){ //ojo con los while infinitos, deberia siempre llegarle una estructura correcta pero en caso de ser llamado de forma erronea no saldria nunca
			for(int i = 0; i < marcos_por_proceso; i++){

				tabla_de_segundo_nivel* dato = retornar_pagina(adm->pagina_primer_nivel, adm->marcos_asignados[adm->offset]);
				if((iterador % 2) ==0){
					if((dato->bit_de_uso ==0) && (dato->bit_modificado == 0)){
						dato->bit_presencia = 0; 
						dato->bit_de_uso = 0;
						log_info(logger, "reemplazo el marco %d a swap", adm->marcos_asignados[adm->offset]);
						log_info(logger, "que tiene la pagina %d", dato->numero_pagina);
						if(dato->bit_modificado){
							volcar_pagina_en_swap(pid, (dato->numero_pagina * tam_pagina), espacio_memoria_user + (dato->marco * tam_pagina));
							dato->bit_modificado = 0;
						}
						return adm->offset;
					}
				}
				
				else{
					if((dato->bit_de_uso ==0) && (dato->bit_modificado == 1)){
						dato->bit_presencia = 0; 
						dato->bit_de_uso = 0;
						log_info(logger, "Se lleva la Pagina %d a swap", adm->marcos_asignados[adm->offset]);
						if(dato->bit_modificado){
							volcar_pagina_en_swap(pid, (dato->numero_pagina * tam_pagina), espacio_memoria_user + (dato->marco * tam_pagina));
							dato->bit_modificado = 0;
						}
						return adm->offset;
					}
					else{
						dato->bit_de_uso = 0;
					}
				}
			adm->offset++;
			adm->offset = adm->offset % marcos_por_proceso;
			}
			iterador++;
		}
	}
	
}
