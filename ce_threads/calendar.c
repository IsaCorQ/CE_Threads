#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "CEthreads.h"
#include "calendar.h"

#define QUANTUM 2 // Tiempo de quantum para el Round Robin

// Variables globales para controlar el canal
char letrero[10] = "izquierda"; // Letrero inicial
int barcos_avanzando = 0; // Barcos que están avanzando
int ancho_canal = 0; // Ancho del canal definido por el usuario
cemutex_t canal_mutex; // Mutex para proteger el acceso al canal
cecond_t canal_disponible; // Condición para indicar cuándo el canal está disponible
Barco barcos[MAX_BOATS];
int num_barcos = 0;

// Función para leer el archivo de texto
int leer_barcos(const char* archivo) {
    FILE* file = fopen(archivo, "r");
    if (file == NULL) {
        perror("No se puede abrir el archivo");
        return -1;
    }

    char buffer[256];
    num_barcos = 0;

    // Leer la cabecera del archivo
    fgets(buffer, sizeof(buffer), file);

    // Leer cada línea del archivo y cargar los datos
    while (fgets(buffer, sizeof(buffer), file) && num_barcos < MAX_BOATS) {
        Barco *barco = &barcos[num_barcos];
        sscanf(buffer, "%d %s %s %d %d %d", &barco->id, barco->tipo, barco->oceano, 
               &barco->prioridad, &barco->tiempo_sjf, &barco->tiempo_maximo);

        // Asignar la velocidad según el tipo de barco
        if (strcmp(barco->tipo, "Patrulla") == 0) {
            barco->velocidad = 3;
        } else if (strcmp(barco->tipo, "Pesquero") == 0) {
            barco->velocidad = 2;
        } else {
            barco->velocidad = 1;
        }

        // Calcular el tiempo restante para cruzar el canal
        barco->tiempo_restante = ancho_canal / barco->velocidad;
        barco->tiempo_a_avanzar = 0;

        num_barcos++;
    }

    fclose(file);
    return num_barcos;
}

void obtener_estado_barcos(Barco* barcos_gui, int* num_barcos_gui) {
    cemutex_lock(&canal_mutex);
    memcpy(barcos_gui, barcos, sizeof(Barco) * num_barcos);
    *num_barcos_gui = num_barcos;
    cemutex_unlock(&canal_mutex);
}

// Función que selecciona el barco con mayor prioridad y que puede avanzar
Barco* seleccionar_barco_prioridad(Barco barcos[], int num_barcos) {
    Barco* barco_seleccionado = NULL;
    int mayor_prioridad = 1000;

    for (int i = 0; i < num_barcos; i++) {
        // Seleccionar solo barcos que coincidan con la dirección del letrero y que aún tengan tiempo por cruzar
        if (barcos[i].tiempo_restante > 0 && strcmp(barcos[i].oceano, letrero) == 0) {
            if (barcos[i].prioridad < mayor_prioridad) {
                mayor_prioridad = barcos[i].prioridad;
                barco_seleccionado = &barcos[i];
            }
        }
    }
    return barco_seleccionado;
}

// Función que simula el cruce de un barco con algoritmo de prioridad
void* cruzar_canal_prioridad(void* arg) {
    Barco* barco = (Barco*)arg;

    while (barco->tiempo_restante > 0) {
        cemutex_lock(&canal_mutex);

        while (strcmp(letrero, barco->oceano) != 0 || barcos_avanzando >= 1) {
            cecond_wait(&canal_disponible, &canal_mutex);
        }

        barcos_avanzando++;

        cemutex_unlock(&canal_mutex);

        int tiempo_a_avanzar = (barco->tiempo_restante > QUANTUM) ? QUANTUM : barco->tiempo_restante;
        barco->tiempo_a_avanzar = tiempo_a_avanzar;
        printf("Barco %d (Prioridad: %d, Océano: %s) avanza por %d segundos...\n", 
               barco->id, barco->prioridad, barco->oceano, tiempo_a_avanzar);
        sleep(tiempo_a_avanzar);

        cemutex_lock(&canal_mutex);
        barcos_avanzando--;
        barco->tiempo_restante -= tiempo_a_avanzar;
        barco->tiempo_a_avanzar = 0;

        if (barco->tiempo_restante <= 0) {
            printf("Barco %d ha cruzado el canal completamente.\n", barco->id);
        }

        cecond_broadcast(&canal_disponible);
        cemutex_unlock(&canal_mutex);
    }

    cethread_exit(NULL);
}

void* cruzar_canal_fcfs(void* arg) {
  Barco* barco = (Barco*)arg;

  while (barco->tiempo_restante > 0) {
    cemutex_lock(&canal_mutex);
    //espera al letrero para avanzar
    while (strcmp(barco->oceano, barco->tipo) != 0) {
      cecond_wait(&canal_disponible, &canal_mutex);

    }
    //desbloquea el mutex
    cemutex_unlock(&canal_mutex);
     printf("Barco %d (Tipo: %s, Océano: %s) avanza por %d segundos...\n", barco->id, barco->tipo, barco->oceano, barco->tiempo_restante);
    sleep(barco->tiempo_restante);

    //Bloqua el mutex
    cemutex_lock(&canal_mutex);
    printf("Barco %d ha cruzado el canal completamente.\n", barco->id);

      // Notificar a otros barcos que pueden avanzar
    cecond_broadcast(&canal_disponible);
    cemutex_unlock(&canal_mutex);

  }
  cethread_exit(NULL);
}
// Función que simula el cruce de un barco con algoritmo Round Robin
void* cruzar_canal_round_robin(void* arg) {
    Barco* barco = (Barco*)arg;

    while (barco->tiempo_restante > 0) {
        cemutex_lock(&canal_mutex); // Bloquear el mutex para evitar colisiones

        // Esperar hasta que el letrero permita que avance
        while (strcmp(letrero, barco->oceano) != 0) {
            cecond_wait(&canal_disponible, &canal_mutex); // Espera hasta que el canal esté disponible para avanzar
        }

        barcos_avanzando++; // Indicar que un barco está avanzando

        cemutex_unlock(&canal_mutex); // Desbloquear el mutex mientras avanza

        // Simulamos el avance del barco por un tiempo quantum
        int tiempo_a_avanzar = (barco->tiempo_restante > QUANTUM) ? QUANTUM : barco->tiempo_restante;
        printf("Barco %d (Tipo: %s, Océano: %s) avanza por %d segundos...\n", barco->id, barco->tipo, barco->oceano, tiempo_a_avanzar);
        sleep(tiempo_a_avanzar); // Simulamos el tiempo que tarda en avanzar

        // Reducimos el tiempo restante del barco
        barco->tiempo_restante -= tiempo_a_avanzar;

        cemutex_lock(&canal_mutex); // Bloquear el mutex nuevamente para actualizar el estado
        barcos_avanzando--; // El barco terminó su avance, liberar el turno

        if (barco->tiempo_restante <= 0) {
            printf("Barco %d ha cruzado el canal completamente.\n", barco->id);
        }

        // Notificar a otros barcos que pueden avanzar
        cecond_broadcast(&canal_disponible);
        cemutex_unlock(&canal_mutex); // Desbloquear el mutex
    }

    cethread_exit(NULL);
}

// Función que cambia el letrero de "izquierda" a "derecha" y viceversa
void cambiar_letrero() {
    cemutex_lock(&canal_mutex); // Bloquear el mutex para asegurar que no haya barcos avanzando
    if (barcos_avanzando == 0) { // Cambiar el letrero solo si ningún barco está avanzando
        if (strcmp(letrero, "izquierda") == 0) {
            strcpy(letrero, "derecha");
        } else {
            strcpy(letrero, "izquierda");
        }
        printf("\n[LETRERO] Ahora el sentido es hacia: %s\n\n", letrero);
        cecond_broadcast(&canal_disponible); // Notificar a los hilos del cambio del letrero
    }
    cemutex_unlock(&canal_mutex); // Desbloquear el mutex
}

void run_calendar(int algoritmo, int intervalo_letrero) {
    // Inicializar mutex y condición
    cemutex_init(&canal_mutex, NULL);
    cecond_init(&canal_disponible, NULL);

    cethread_t hilos[num_barcos]; // Arreglo de hilos

    printf("Iniciando simulación con %d barcos...\n", num_barcos);
    printf("Ancho del canal: %d\n", ancho_canal);
    printf("Algoritmo seleccionado: %d\n", algoritmo);
    printf("Intervalo de cambio de letrero: %d segundos\n", intervalo_letrero);

    // Crear los hilos (barcos) que cruzarán el canal dependiendo del algoritmo seleccionado
    if (algoritmo == 1) { // Round Robin
        for (int i = 0; i < num_barcos; i++) {
            cethread_create(&hilos[i], NULL, cruzar_canal_round_robin, (void*)&barcos[i]);
        }
    } else if (algoritmo == 2) { // Prioridad
        for (int i = 0; i < num_barcos; i++) {
            cethread_create(&hilos[i], NULL, cruzar_canal_prioridad, (void*)&barcos[i]);
        }
    } else if (algoritmo == 4) { // FCFS
        for (int i = 0; i < num_barcos; i++) {
            cethread_create(&hilos[i], NULL, cruzar_canal_fcfs, (void*)&barcos[i]);
        }
    } else {
        printf("Algoritmo no implementado.\n");
        return;
    }

    // Controlar el cambio de letrero en intervalos definidos
    while (1) {
        sleep(intervalo_letrero); // Esperar el intervalo antes de cambiar el letrero
        cambiar_letrero(); // Cambiar el letrero después del intervalo
    }

    // Esperar a que todos los hilos terminen
    for (int i = 0; i < num_barcos; i++) {
        cethread_join(&hilos[i], NULL);
    }

    // Destruir el mutex y la condición
    cemutex_destroy(&canal_mutex);
    cecond_destroy(&canal_disponible);

    printf("Todos los barcos han cruzado el canal.\n");
}

// Función para inicializar la simulación
void inicializar_simulacion(const char* archivo_barcos, int ancho_canal_param) {
    ancho_canal = ancho_canal_param;
    num_barcos = leer_barcos(archivo_barcos);
    if (num_barcos < 0) {
        printf("Error al leer el archivo de barcos.\n");
        exit(1);
    }
}
