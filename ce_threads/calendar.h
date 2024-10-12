#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include "CEthreads.h"

#define MAX_BOATS 100

typedef struct {
    int id;
    char tipo[10];
    char oceano[10];
    int prioridad;
    int tiempo_cruzar;
    int tiempo_maximo;
    int velocidad;
    int cruzo;
    int x;
    int y;
    int tiempo_a_avanzar;
} Barco;

typedef struct {
    Barco barcos[MAX_BOATS];
    int num_barcos;
    cemutex_t mutex_barcos;
} SharedData;

#endif // SHARED_DATA_H