#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define QUANTUM 2 // Tiempo de quantum para el Round Robin

// Estructura para los barcos (hilos)
typedef struct {
    int id;
    char tipo[10];
    char oceano[10];
    int prioridad;
    int tiempo_sjf;
    int tiempo_maximo;
    int tiempo_restante;
    int velocidad; // Velocidad del barco según su tipo (longitud/tiempo ejm m/s)
} Barco;

// Variables globales para controlar el canal
char letrero[10] = "izquierda"; // Letrero inicial
int barcos_avanzando = 0; // Barcos que están avanzando
int ancho_canal = 0; // Ancho del canal definido por el usuario
pthread_mutex_t canal_mutex; // Mutex para proteger el acceso al canal
pthread_cond_t canal_disponible; // Condición para indicar cuándo el canal está disponible

// Función para leer el archivo de texto
int leer_barcos(const char* archivo, Barco barcos[], int max_barcos) {
    FILE* file = fopen(archivo, "r");
    if (file == NULL) {
        perror("No se puede abrir el archivo");
        return -1;
    }

    char buffer[256];
    int i = 0;

    // Leer la cabecera del archivo
    fgets(buffer, sizeof(buffer), file);

    // Leer cada línea del archivo y cargar los datos
    // Formato: ID Tipo Océano Prioridad Tiempo_SJF Tiempo_Maximo
    while (fgets(buffer, sizeof(buffer), file) && i < max_barcos) {
        sscanf(buffer, "%d %s %s %d %d %d", &barcos[i].id, barcos[i].tipo, barcos[i].oceano, &barcos[i].prioridad, &barcos[i].tiempo_sjf, &barcos[i].tiempo_maximo);

        // Asignar la velocidad según el tipo de barco
        if (strcmp(barcos[i].tipo, "Patrulla") == 0) {
            barcos[i].velocidad = 3; // Barcos patrulla son los más rápidos
        } else if (strcmp(barcos[i].tipo, "Pesquero") == 0) {
            barcos[i].velocidad = 2; // Barcos pesqueros son intermedios
        } else {
            barcos[i].velocidad = 1; // Barcos normales son los más lentos
        }

        // Calcular el tiempo restante para cruzar el canal
        barcos[i].tiempo_restante = ancho_canal / barcos[i].velocidad;

        i++;
    }

    fclose(file);
    return i; // Devolver el número de barcos leídos
}

// Función para ordenar los barcos por prioridad (menor a mayor)
void ordenar_barcos_por_prioridad(Barco barcos[], int num_barcos) {
    for (int i = 0; i < num_barcos - 1; i++) {
        for (int j = 0; j < num_barcos - i - 1; j++) {
            if (barcos[j].prioridad > barcos[j + 1].prioridad) {
                // Intercambiar barcos
                Barco temp = barcos[j];
                barcos[j] = barcos[j + 1];
                barcos[j + 1] = temp;
            }
        }
    }
}

// Función que simula el cruce completo de un barco con algoritmo de prioridad
void* cruzar_canal_prioridad(void* arg) {
    Barco* barco = (Barco*)arg;

    pthread_mutex_lock(&canal_mutex); // Bloquear el mutex para evitar colisiones

    // Esperar hasta que el letrero permita que avance
    while (strcmp(letrero, barco->oceano) != 0 || barcos_avanzando >= 1) {
        pthread_cond_wait(&canal_disponible, &canal_mutex); // Espera hasta que el canal esté disponible
    }

    barcos_avanzando++; // Indicar que un barco está avanzando

    pthread_mutex_unlock(&canal_mutex); // Desbloquear el mutex mientras avanza

    // Simular el cruce del barco completo
    printf("Barco %d (Prioridad: %d, Océano: %s) está cruzando el canal...\n", barco->id, barco->prioridad, barco->oceano);
    sleep(barco->tiempo_restante); // Simulamos el tiempo que tarda en cruzar el canal completamente

    pthread_mutex_lock(&canal_mutex); // Bloquear el mutex nuevamente para actualizar el estado
    barcos_avanzando--; // El barco terminó de cruzar, liberar el turno

    printf("Barco %d ha cruzado el canal completamente.\n", barco->id);

    // Notificar a otros barcos que pueden avanzar
    pthread_cond_broadcast(&canal_disponible);
    pthread_mutex_unlock(&canal_mutex); // Desbloquear el mutex

    pthread_exit(NULL);
}

void* cruzar_canal_fcfs(void* arg) {
  Barco* barco = (Barco*)arg;

  while (barco->tiempo_restante > 0) {
    pthread_mutex_lock(&canal_mutex);
    //espera al letrero para avanzar
    while (strcmp(barco->oceano, barco->tipo) != 0) {
      pthread_cond_wait(&canal_disponible, &canal_mutex);

    }
    //desbloquea el mutex
    pthread_mutex_unlock(&canal_mutex);
     printf("Barco %d (Tipo: %s, Océano: %s) avanza por %d segundos...\n", barco->id, barco->tipo, barco->oceano, barco->tiempo_restante);
    sleep(barco->tiempo_restante);

    //Bloqua el mutex
    pthread_mutex_lock(&canal_mutex);
    printf("Barco %d ha cruzado el canal completamente.\n", barco->id);

      // Notificar a otros barcos que pueden avanzar
    pthread_cond_broadcast(&canal_disponible);
    pthread_mutex_unlock(&canal_mutex);

  }
  pthread_exit(NULL);
}
// Función que simula el cruce de un barco con algoritmo Round Robin
void* cruzar_canal_round_robin(void* arg) {
    Barco* barco = (Barco*)arg;

    while (barco->tiempo_restante > 0) {
        pthread_mutex_lock(&canal_mutex); // Bloquear el mutex para evitar colisiones

        // Esperar hasta que el letrero permita que avance
        while (strcmp(letrero, barco->oceano) != 0) {
            pthread_cond_wait(&canal_disponible, &canal_mutex); // Espera hasta que el canal esté disponible para avanzar
        }

        barcos_avanzando++; // Indicar que un barco está avanzando

        pthread_mutex_unlock(&canal_mutex); // Desbloquear el mutex mientras avanza

        // Simulamos el avance del barco por un tiempo quantum
        int tiempo_a_avanzar = (barco->tiempo_restante > QUANTUM) ? QUANTUM : barco->tiempo_restante;
        printf("Barco %d (Tipo: %s, Océano: %s) avanza por %d segundos...\n", barco->id, barco->tipo, barco->oceano, tiempo_a_avanzar);
        sleep(tiempo_a_avanzar); // Simulamos el tiempo que tarda en avanzar

        // Reducimos el tiempo restante del barco
        barco->tiempo_restante -= tiempo_a_avanzar;

        pthread_mutex_lock(&canal_mutex); // Bloquear el mutex nuevamente para actualizar el estado
        barcos_avanzando--; // El barco terminó su avance, liberar el turno

        if (barco->tiempo_restante <= 0) {
            printf("Barco %d ha cruzado el canal completamente.\n", barco->id);
        }

        // Notificar a otros barcos que pueden avanzar
        pthread_cond_broadcast(&canal_disponible);
        pthread_mutex_unlock(&canal_mutex); // Desbloquear el mutex
    }

    pthread_exit(NULL);
}

// Función que cambia el letrero de "izquierda" a "derecha" y viceversa
void cambiar_letrero() {
    pthread_mutex_lock(&canal_mutex); // Bloquear el mutex para asegurar que no haya barcos avanzando
    if (barcos_avanzando == 0) { // Cambiar el letrero solo si ningún barco está avanzando
        if (strcmp(letrero, "izquierda") == 0) {
            strcpy(letrero, "derecha");
        } else {
            strcpy(letrero, "izquierda");
        }
        printf("\n[LETRERO] Ahora el sentido es hacia: %s\n\n", letrero);
        pthread_cond_broadcast(&canal_disponible); // Notificar a los hilos del cambio del letrero
    }
    pthread_mutex_unlock(&canal_mutex); // Desbloquear el mutex
}

int main() {
    Barco barcos[10]; // Capacidad para 10 barcos

    // Definir el ancho del canal
    printf("Ingrese el ancho del canal (en unidades de longitud): ");
    scanf("%d", &ancho_canal);

    int num_barcos = leer_barcos("barcos.txt", barcos, 10); // Leer los barcos desde el archivo

    if (num_barcos < 0) {
        return 1; // Error al leer el archivo
    }

    int algoritmo;
    printf("Seleccione el algoritmo de calendarización:\n");
    printf("1. Round Robin (RR)\n");
    printf("2. Prioridad\n");
    printf("3. SJF (No implementado)\n");
    printf("4. FCFS (No implementado)\n");
    printf("5. Tiempo real (No implementado)\n");
    scanf("%d", &algoritmo);

    int intervalo_letrero;
    printf("Ingrese el intervalo de tiempo (segundos) para cambiar el letrero: ");
    scanf("%d", &intervalo_letrero);

    // Inicializar mutex y condición
    pthread_mutex_init(&canal_mutex, NULL);
    pthread_cond_init(&canal_disponible, NULL);

    pthread_t hilos[num_barcos]; // Arreglo de hilos

    // Crear los hilos (barcos) que cruzarán el canal dependiendo del algoritmo seleccionado
    if (algoritmo == 1) { // Round Robin
        for (int i = 0; i < num_barcos; i++) {
            pthread_create(&hilos[i], NULL, cruzar_canal_round_robin, (void*)&barcos[i]);
        }
    } else if (algoritmo == 2) { // Prioridad
        // Ordenar los barcos por prioridad antes de cruzar
        ordenar_barcos_por_prioridad(barcos, num_barcos);
        
        // Los barcos cruzan en orden de prioridad
        for (int i = 0; i < num_barcos; i++) {
        	printf("Barco %d (Prioridad: %d, Océano: %s) orden\n", barcos[i].id, barcos[i].prioridad, barcos[i].oceano);
            pthread_create(&hilos[i], NULL, cruzar_canal_prioridad, (void*)&barcos[i]);
        }
    } else {
        printf("Algoritmo no implementado.\n");
        return 1;
    }

    // Controlar el cambio de letrero en intervalos definidos
    while (1) {
        sleep(intervalo_letrero); // Esperar el intervalo antes de cambiar el letrero
        cambiar_letrero(); // Cambiar el letrero después del intervalo
    }

    // Esperar a que todos los hilos terminen
    for (int i = 0; i < num_barcos; i++) {
        pthread_join(hilos[i], NULL);
    }

    // Destruir el mutex y la condición
    pthread_mutex_destroy(&canal_mutex);
    pthread_cond_destroy(&canal_disponible);

    printf("Todos los barcos han cruzado el canal.\n");
    return 0;
}

