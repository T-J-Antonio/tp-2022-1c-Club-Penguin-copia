#include "utilsKernel.h"

void* recibiendo(void* , t_config*);
void* funcion_pasar_a_ready(void*);
void pasar_a_running(pcb*);
void* escuchar_consola(int , t_config*);
void* realizar_io(void* );
void* dispositivo_io(void*);
void* recibir_pcb_de_cpu( void* );
void* planificador_de_corto_plazo(void*);
pcb* algoritmo_srt();
void remover_de_cola_ready(pcb*);
void hacer_cuenta_srt(pcb* );
