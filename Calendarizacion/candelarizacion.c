#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define QUANTUM 2 // Tiempo de quantum para el Round Robin
#define MAX_BARCOS 100
#define MAX_LINE_LENGTH 100

// Estructura para los barcos (hilos)
typedef struct {
    int id;
    char tipo[10];
    char oceano[10];
    int prioridad;
    int tiempo_cruzar;
    int tiempo_maximo;
    int velocidad; // Velocidad del barco según su tipo (longitud/tiempo ejm m/s)
    int cruzo;
} Barco;

// Variables globales para controlar el canal
char letrero[10] = "izquierda"; // Letrero inicial
int barcos_avanzando = 0; // Barcos que están avanzando
int barcos_cruzando = 0; // Barcos que están avanzando
int barcos_terminados = 0; //numero de barcos que finalizaron el recorrido
int W = 0; //variable que controla el numero de barcos en el canal equidad
int ancho_canal = 0; // Ancho del canal definido por el usuario
pthread_mutex_t canal_mutex; // Mutex para proteger el acceso al canal
pthread_cond_t canal_disponible; // Condición para indicar cuándo el canal está disponible

Barco barcos[MAX_BARCOS]; // Max de 100 barcos
int num_barcos;

int id_barco_prioridad_der = -1; //ids de los barcos con mayores prioridades a cada lado independientemente del algoritmo
int id_barco_prioridad_izq = -1;


int barcos_izq = 0;
int barcos_der = 0;

// Function to count boats in the left and right oceans
void barcos_izq_der(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    char line[MAX_LINE_LENGTH];

    while (fgets(line, sizeof(line), file)) {
        char *token = strtok(line, " ");
        token = strtok(NULL, " ");
        token = strtok(NULL, " ");

        if (token != NULL) {
            if (strstr(token, "izquierda") != NULL) {
                barcos_izq++;
            } else if (strstr(token, "derecha") != NULL) {
                barcos_der++;
            }
        }
    }

    fclose(file);
}
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
    // Formato: ID Tipo Océano Prioridad Tiempo_Maximo
    while (fgets(buffer, sizeof(buffer), file) && i < max_barcos) {
        sscanf(buffer, "%d %s %s %d %d", &barcos[i].id, barcos[i].tipo, barcos[i].oceano, &barcos[i].prioridad, &barcos[i].tiempo_maximo);

        // Asignar la velocidad según el tipo de barco
        if (strcmp(barcos[i].tipo, "Patrulla") == 0) {
            barcos[i].velocidad = 3; // Barcos patrulla son los más rápidos
        } else if (strcmp(barcos[i].tipo, "Pesquero") == 0) {
            barcos[i].velocidad = 2; // Barcos pesqueros son intermedios
        } else {
            barcos[i].velocidad = 1; // Barcos normales son los más lentos
        }

        // Calcular el tiempo restante para cruzar el canal
        barcos[i].tiempo_cruzar = (barcos[i].velocidad != 0) ? (ancho_canal / barcos[i].velocidad) : 0;
        barcos[i].cruzo = 0;

        i++;
    }

    fclose(file);
    return i; // Devolver el número de barcos leídos
}

// Función que actualiza la lista de barcos si hay nuevos barcos en el archivo
int actualizar_barcos(const char* archivo, Barco barcos[], int num_barcos_actual) {
    FILE* file = fopen(archivo, "r");
    if (file == NULL) {
        perror("No se puede abrir el archivo");
        return -1;
    }

    char buffer[256];
    Barco nuevos_barcos[MAX_BARCOS];
    int num_nuevos_barcos = 0;

    // Leer la cabecera del archivo
    fgets(buffer, sizeof(buffer), file);

    // Leer cada línea del archivo y cargar los nuevos barcos en una lista temporal
    while (fgets(buffer, sizeof(buffer), file)) {
        sscanf(buffer, "%d %s %s %d %d", &nuevos_barcos[num_nuevos_barcos].id, nuevos_barcos[num_nuevos_barcos].tipo,
               nuevos_barcos[num_nuevos_barcos].oceano, &nuevos_barcos[num_nuevos_barcos].prioridad,
               &nuevos_barcos[num_nuevos_barcos].tiempo_maximo);

        // Asignar la velocidad según el tipo de barco
        if (strcmp(nuevos_barcos[num_nuevos_barcos].tipo, "Patrulla") == 0) {
            nuevos_barcos[num_nuevos_barcos].velocidad = 3; // Barcos patrulla son los más rápidos
        } else if (strcmp(nuevos_barcos[num_nuevos_barcos].tipo, "Pesquero") == 0) {
            nuevos_barcos[num_nuevos_barcos].velocidad = 2; // Barcos pesqueros son intermedios
        } else {
            nuevos_barcos[num_nuevos_barcos].velocidad = 1; // Barcos normales son los más lentos
        }

        // Calcular el tiempo restante para cruzar el canal
        nuevos_barcos[num_nuevos_barcos].tiempo_cruzar = (nuevos_barcos[num_nuevos_barcos].velocidad != 0) ? 
                                                         (ancho_canal / nuevos_barcos[num_nuevos_barcos].velocidad) : 0;
        nuevos_barcos[num_nuevos_barcos].cruzo = 0; // Inicialmente, el barco no ha cruzado

        num_nuevos_barcos++;
    }

    fclose(file);

    // Comparar los nuevos barcos con los existentes por su ID
    for (int i = 0; i < num_nuevos_barcos; i++) {
        int encontrado = 0;
        for (int j = 0; j < num_barcos_actual; j++) {
            if (nuevos_barcos[i].id == barcos[j].id) {
                encontrado = 1;
                break;
            }
        }
        // Si no se encuentra el barco en la lista actual, se agrega
        if (!encontrado && num_barcos_actual < MAX_BARCOS) {
            barcos[num_barcos_actual] = nuevos_barcos[i];
            num_barcos_actual++;
            printf("Nuevo barco agregado: ID=%d, Tipo=%s, Océano=%s\n",
                   nuevos_barcos[i].id, nuevos_barcos[i].tipo, nuevos_barcos[i].oceano);
        }
    }

    return num_barcos_actual; // Devolver el nuevo número de barcos
}

void setear_prioridad() {
    int i;
    int max_prioridad_der = 1000;
    int max_prioridad_izq = 1000;
    
    // Inicializamos a -1 por si no hay barcos válidos (que no hayan cruzado)
    id_barco_prioridad_der = -1;
    id_barco_prioridad_izq = -1;

    for (i = 0; i < num_barcos; i++) {
        // Solo tomamos en cuenta los barcos que no hayan cruzado
        if (barcos[i].cruzo == 0) {
            // Comparar cadenas correctamente con strcmp
            if (strcmp(barcos[i].oceano, "derecha") == 0 && barcos[i].prioridad < max_prioridad_der) {
                max_prioridad_der = barcos[i].prioridad;
                id_barco_prioridad_der = barcos[i].id;
            } 
            else if (strcmp(barcos[i].oceano, "izquierda") == 0 && barcos[i].prioridad < max_prioridad_izq) {
                max_prioridad_izq = barcos[i].prioridad;
                id_barco_prioridad_izq = barcos[i].id;
            }

        }
    }
}

void setear_sjf() {
    int i;
    int max_prioridad_der = 1000;
    int max_prioridad_izq = 1000;
    
    // Inicializamos a -1 por si no hay barcos válidos (que no hayan cruzado)
    id_barco_prioridad_der = -1;
    id_barco_prioridad_izq = -1;

    for (i = 0; i < num_barcos; i++) {
        // Solo tomamos en cuenta los barcos que no hayan cruzado
        if (barcos[i].cruzo == 0) {
            // Comparar cadenas correctamente con strcmp
            if (strcmp(barcos[i].oceano, "derecha") == 0 && barcos[i].tiempo_cruzar < max_prioridad_der) {
                max_prioridad_der = barcos[i].tiempo_cruzar;
                id_barco_prioridad_der = barcos[i].id;
            } 
            else if (strcmp(barcos[i].oceano, "izquierda") == 0 && barcos[i].tiempo_cruzar < max_prioridad_izq) {
                max_prioridad_izq = barcos[i].tiempo_cruzar;
                id_barco_prioridad_izq = barcos[i].id;
            }

        }
    }
    }

void setear_round_robin() {
    static int idx_actual_der = 0;  // Índice actual para el lado derecho
    static int idx_actual_izq = 0;  // Índice actual para el lado izquierdo
    static int primera_vez = 1;     // Bandera para controlar la primera ejecución

    // Contamos los barcos que no han cruzado para cada lado
    int num_barcos_izq = 0, num_barcos_der = 0;
    for (int i = 0; i < num_barcos; i++) {
        if (barcos[i].cruzo == 0) {
            if (strcmp(barcos[i].oceano, "izquierda") == 0) {
                num_barcos_izq++;
            } else if (strcmp(barcos[i].oceano, "derecha") == 0) {
                num_barcos_der++;
            }
        }
    }

    // Ajustamos el índice para no salirnos de los barcos disponibles en cada lado
    if (num_barcos_izq > 0) {
        idx_actual_izq = idx_actual_izq % num_barcos_izq;
    }
    if (num_barcos_der > 0) {
        idx_actual_der = idx_actual_der % num_barcos_der;
    }

    // Si es la primera vez, inicializamos ambas prioridades
    if (primera_vez) {
        // Para el lado izquierdo
        //Si se quiere seguir el orden donde se anaden, se comenta el lado con el que empieza el cartel
        /*
        int barcos_procesados_izq = 0;
        for (int i = 0; i < num_barcos; i++) {
            if (barcos[i].cruzo == 0 && strcmp(barcos[i].oceano, "izquierda") == 0) {
                id_barco_prioridad_izq = barcos[i].id;  // Asignar prioridad al primer barco en la izquierda
                idx_actual_izq++;  // Avanzamos al siguiente barco en el lado izquierdo
                break;
            }
        }
        */

        // Para el lado derecho
        int barcos_procesados_der = 0;
        for (int i = 0; i < num_barcos; i++) {
            if (barcos[i].cruzo == 0 && strcmp(barcos[i].oceano, "derecha") == 0) {
                id_barco_prioridad_der = barcos[i].id;  // Asignar prioridad al primer barco en la derecha
                idx_actual_der++;  // Avanzamos al siguiente barco en el lado derecho
                break;
            }
        }

        // Después de la inicialización, cambiamos la bandera
        primera_vez = 0;
    }

    // Si el letrero apunta a la izquierda, avanzamos la prioridad en el lado izquierdo
    if (strcmp(letrero, "izquierda") == 0) {
        int barcos_procesados = 0;
        for (int i = 0; i < num_barcos; i++) {
            if (barcos[i].cruzo == 0 && strcmp(barcos[i].oceano, "izquierda") == 0) {
                if (barcos_procesados == idx_actual_izq) {
                    id_barco_prioridad_izq = barcos[i].id;  // Asignar prioridad
                    idx_actual_izq++;  // Avanzamos al siguiente barco en el lado izquierdo
                    break;
                }
                barcos_procesados++;
            }
        }
    }

    // Si el letrero apunta a la derecha, avanzamos la prioridad en el lado derecho
    if (strcmp(letrero, "derecha") == 0) {
        int barcos_procesados = 0;
        for (int i = 0; i < num_barcos; i++) {
            if (barcos[i].cruzo == 0 && strcmp(barcos[i].oceano, "derecha") == 0) {
                if (barcos_procesados == idx_actual_der) {
                    id_barco_prioridad_der = barcos[i].id;  // Asignar prioridad
                    idx_actual_der++;  // Avanzamos al siguiente barco en el lado derecho
                    break;
                }
                barcos_procesados++;
            }
        }
    }
}

void setear_fcfs() {
    int i;
    int max_prioridad_der = 1000;
    int max_prioridad_izq = 1000;
    
    // Inicializamos a -1 por si no hay barcos válidos (que no hayan cruzado)
    id_barco_prioridad_der = -1;
    id_barco_prioridad_izq = -1;

    for (i = 0; i < num_barcos; i++) {
        // Solo tomamos en cuenta los barcos que no hayan cruzado
        if (barcos[i].cruzo == 0) {
            // Comparar cadenas correctamente con strcmp
            if (strcmp(barcos[i].oceano, "derecha") == 0 && barcos[i].id < max_prioridad_der) {
                max_prioridad_der = barcos[i].id;
                id_barco_prioridad_der = barcos[i].id;
            } 
            else if (strcmp(barcos[i].oceano, "izquierda") == 0 && barcos[i].id < max_prioridad_izq) {
                max_prioridad_izq = barcos[i].id;
                id_barco_prioridad_izq = barcos[i].id;
            }

        }
    }
}

void setear_tiempo_real() {
    int i;
    int max_prioridad_der = 1000;
    int max_prioridad_izq = 1000;
    
    // Inicializamos a -1 por si no hay barcos válidos (que no hayan cruzado)
    id_barco_prioridad_der = -1;
    id_barco_prioridad_izq = -1;

    for (i = 0; i < num_barcos; i++) {
        // Solo tomamos en cuenta los barcos que no hayan cruzado
        if (barcos[i].cruzo == 0) {
            // Comparar cadenas correctamente con strcmp
            if (strcmp(barcos[i].oceano, "derecha") == 0 && barcos[i].tiempo_maximo < max_prioridad_der) {
                max_prioridad_der = barcos[i].tiempo_maximo;
                id_barco_prioridad_der = barcos[i].id;
            } 
            else if (strcmp(barcos[i].oceano, "izquierda") == 0 && barcos[i].tiempo_maximo < max_prioridad_izq) {
                max_prioridad_izq = barcos[i].tiempo_maximo;
                id_barco_prioridad_izq = barcos[i].id;
            }

        }
    }
}

void* cruzar_canal_tiempo_real(void* arg) {
    Barco* barco = (Barco*)arg;

    while (barco->tiempo_cruzar > 0) {
        pthread_mutex_lock(&canal_mutex); // Bloquear el mutex para controlar el acceso

        // Si el barco actual no tiene la prioridad más alta, esperar
    // Esperar hasta que el letrero permita que avance y sea su turno de prioridad
    while (strcmp(letrero, barco->oceano) != 0 || barcos_avanzando >= 1 ||
           (strcmp(barco->oceano, "derecha") == 0 && barco->id != id_barco_prioridad_der) ||
           (strcmp(barco->oceano, "izquierda") == 0 && barco->id != id_barco_prioridad_izq)) {

        pthread_cond_wait(&canal_disponible, &canal_mutex); // Esperar hasta que el canal esté disponible
    }
        // Iniciar el cruce del barco
        barcos_avanzando++;
        barcos_cruzando = 1;
        pthread_mutex_unlock(&canal_mutex); // Desbloquear el mutex mientras avanza

        // Simular que el barco avanza un segundo
        printf("Barco %d (Tiempo máximo: %d, Tiempo restante: %d) está cruzando el canal...\n", 
               barco->id, barco->tiempo_maximo, barco->tiempo_cruzar);
        sleep(1);
        barco->tiempo_cruzar--; // Reducir el tiempo restante

        // Volver a bloquear el mutex para actualizar el estado
        pthread_mutex_lock(&canal_mutex);
        barcos_avanzando--; // El barco libera el control tras un segundo de avance

        if (barco->tiempo_cruzar == 0) {
            printf("Barco %d ha cruzado completamente el canal.\n", barco->id);
            barco->cruzo = 1; // Marcar el barco como que ya cruzó
            barcos_terminados++;//agrega a la variable que el barco finalizó su recorrido
            barcos_cruzando = 0;
        }

        // Actualizar las prioridades dinámicamente después de cada segundo
        setear_tiempo_real();

        // Notificar a los demás barcos que el canal está disponible
        pthread_cond_broadcast(&canal_disponible);
        pthread_mutex_unlock(&canal_mutex);

        // Pausar brevemente antes de verificar si sigue teniendo la prioridad
        sleep(1);
    }

    pthread_exit(NULL);
}

// Función que simula el cruce completo de un barco con algoritmo de prioridad
void* cruzar_canal_prioridad(void* arg) {
    Barco* barco = (Barco*)arg;

    pthread_mutex_lock(&canal_mutex); // Bloquear el mutex para evitar colisiones

    // Esperar hasta que el letrero permita que avance y sea su turno de prioridad
    while (strcmp(letrero, barco->oceano) != 0 || barcos_avanzando >= 1 ||
           (strcmp(barco->oceano, "derecha") == 0 && barco->id != id_barco_prioridad_der) ||
           (strcmp(barco->oceano, "izquierda") == 0 && barco->id != id_barco_prioridad_izq)) {

        pthread_cond_wait(&canal_disponible, &canal_mutex); // Esperar hasta que el canal esté disponible
    }

    barcos_avanzando++; // Indicar que un barco está avanzando

    pthread_mutex_unlock(&canal_mutex); // Desbloquear el mutex mientras avanza

    // Simular el cruce del barco completo
    printf("Barco %d (Prioridad: %d, Océano: %s, Tiempo en cruzar: %d) está cruzando el canal...\n", 
           barco->id, barco->prioridad, barco->oceano, barco->tiempo_cruzar);
    sleep(barco->tiempo_cruzar); // Simular el tiempo que tarda en cruzar el canal completamente

    pthread_mutex_lock(&canal_mutex); // Bloquear el mutex nuevamente para actualizar el estado
    barcos_avanzando--; // El barco terminó de cruzar, liberar el turno

    printf("Barco %d ha cruzado el canal completamente.\n", barco->id);
    barcos_terminados++;//agrega a la variable que el barco finalizó su recorrido
    barco->cruzo = 1; // Actualizar el estado del barco

    // Actualizar las prioridades después de que el barco haya cruzado
    setear_prioridad();

    // Notificar a otros barcos que pueden avanzar
    pthread_cond_broadcast(&canal_disponible);
    pthread_mutex_unlock(&canal_mutex); // Desbloquear el mutex

    // Esperar un breve momento para simular el cambio de turno
    sleep(1);

    pthread_exit(NULL);
}

// Función que simula el cruce completo de un barco con algoritmo de prioridad
void* cruzar_canal_fcfs(void* arg) {
    Barco* barco = (Barco*)arg;

    pthread_mutex_lock(&canal_mutex); // Bloquear el mutex para evitar colisiones

    // Esperar hasta que el letrero permita que avance y sea su turno de prioridad
    while (strcmp(letrero, barco->oceano) != 0 || barcos_avanzando >= 1 ||
           (strcmp(barco->oceano, "derecha") == 0 && barco->id != id_barco_prioridad_der) ||
           (strcmp(barco->oceano, "izquierda") == 0 && barco->id != id_barco_prioridad_izq)) {

        pthread_cond_wait(&canal_disponible, &canal_mutex); // Esperar hasta que el canal esté disponible
    }

    barcos_avanzando++; // Indicar que un barco está avanzando

    pthread_mutex_unlock(&canal_mutex); // Desbloquear el mutex mientras avanza

    // Simular el cruce del barco completo
    printf("Barco %d (Prioridad: %d, Océano: %s, Tiempo en cruzar: %d) está cruzando el canal...\n", 
           barco->id, barco->prioridad, barco->oceano, barco->tiempo_cruzar);
    sleep(barco->tiempo_cruzar); // Simular el tiempo que tarda en cruzar el canal completamente

    pthread_mutex_lock(&canal_mutex); // Bloquear el mutex nuevamente para actualizar el estado
    barcos_avanzando--; // El barco terminó de cruzar, liberar el turno

    printf("Barco %d ha cruzado el canal completamente.\n", barco->id);
    barco->cruzo = 1; // Actualizar el estado del barco
    barcos_terminados++;//agrega a la variable que el barco finalizó su recorrido
    setear_fcfs();
    // Notificar a otros barcos que pueden avanzar
    pthread_cond_broadcast(&canal_disponible);
    pthread_mutex_unlock(&canal_mutex); // Desbloquear el mutex

    // Esperar un breve momento para simular el cambio de turno
    sleep(1);

    pthread_exit(NULL);
}

void* cruzar_canal_sjf(void* arg) {
    Barco* barco = (Barco*)arg;

    pthread_mutex_lock(&canal_mutex); // Bloquear el mutex para evitar colisiones

    // Esperar hasta que el letrero permita que avance y sea su turno de prioridad
    while (strcmp(letrero, barco->oceano) != 0 || barcos_avanzando >= 1 ||
           (strcmp(barco->oceano, "derecha") == 0 && barco->id != id_barco_prioridad_der) ||
           (strcmp(barco->oceano, "izquierda") == 0 && barco->id != id_barco_prioridad_izq)) {

        pthread_cond_wait(&canal_disponible, &canal_mutex); // Esperar hasta que el canal esté disponible
    }

    barcos_avanzando++; // Indicar que un barco está avanzando

    pthread_mutex_unlock(&canal_mutex); // Desbloquear el mutex mientras avanza

    // Simular el cruce del barco completo
    printf("Barco %d (Prioridad: %d, Océano: %s, Tiempo en cruzar: %d) está cruzando el canal...\n", 
           barco->id, barco->prioridad, barco->oceano, barco->tiempo_cruzar);
    sleep(barco->tiempo_cruzar); // Simular el tiempo que tarda en cruzar el canal completamente

    pthread_mutex_lock(&canal_mutex); // Bloquear el mutex nuevamente para actualizar el estado
    barcos_avanzando--; // El barco terminó de cruzar, liberar el turno

    printf("Barco %d ha cruzado el canal completamente.\n", barco->id);
    barco->cruzo = 1; // Actualizar el estado del barco
    barcos_terminados++;//agrega a la variable que el barco finalizó su recorrido

    // Actualizar las prioridades después de que el barco haya cruzado
    setear_sjf();

    // Notificar a otros barcos que pueden avanzar
    pthread_cond_broadcast(&canal_disponible);
    pthread_mutex_unlock(&canal_mutex); // Desbloquear el mutex

    // Esperar un breve momento para simular el cambio de turno
    sleep(1);

    pthread_exit(NULL);
}

void* cruzar_canal_round_robin(void* arg) {
    Barco* barco = (Barco*)arg;
    int conto_barco = 0;

    while (barco->tiempo_cruzar > 0) {
        pthread_mutex_lock(&canal_mutex); // Bloquear el mutex para evitar colisiones

            // Esperar hasta que el letrero permita que avance y sea su turno de prioridad
    while (strcmp(letrero, barco->oceano) != 0 || barcos_avanzando >= 1 ||
           (strcmp(barco->oceano, "derecha") == 0 && barco->id != id_barco_prioridad_der) ||
           (strcmp(barco->oceano, "izquierda") == 0 && barco->id != id_barco_prioridad_izq)) {
        pthread_cond_wait(&canal_disponible, &canal_mutex); // Esperar hasta que el canal esté disponible
    }
        

        barcos_avanzando++; // Indicar que un barco está avanzando
        if (conto_barco == 0){
        	barcos_cruzando++;
        	conto_barco = 1;
        }


        pthread_mutex_unlock(&canal_mutex); // Desbloquear el mutex mientras avanza

        // Simulamos el avance del barco por un tiempo quantum
        int tiempo_a_avanzar = (barco->tiempo_cruzar > QUANTUM) ? QUANTUM : barco->tiempo_cruzar;
        printf("Barco %d (Tipo: %s, Océano: %s) avanza por %d segundos...\n", barco->id, barco->tipo, barco->oceano, tiempo_a_avanzar);
        sleep(tiempo_a_avanzar); // Simulamos el tiempo que tarda en avanzar

        // Reducimos el tiempo restante del barco
        barco->tiempo_cruzar -= tiempo_a_avanzar;
        

        pthread_mutex_lock(&canal_mutex); // Bloquear el mutex nuevamente para actualizar el estado
        barcos_avanzando--; // El barco terminó su avance, liberar el turno
        

        if (barco->tiempo_cruzar <= 0) {
            printf("Barco %d ha cruzado el canal completamente.\n", barco->id);
            barcos_terminados++; //agrega a la variable que el barco finalizó su recorrido
            barco->cruzo = 1;
            barcos_cruzando--;
        }
        
        setear_round_robin();
        // Notificar a otro barco del mismo lado que puede avanzar
        pthread_cond_broadcast(&canal_disponible);

        pthread_mutex_unlock(&canal_mutex); // Desbloquear el mutex para permitir que otros barcos avancen

        // Esperar un breve momento para simular el cambio de turno
        sleep(1);
    }

    pthread_exit(NULL);
}

// Función que cambia el letrero de "izquierda" a "derecha" y viceversa
void cambiar_letrero() {
    pthread_mutex_lock(&canal_mutex); // Bloquear el mutex para asegurar que no haya barcos avanzando
    if (barcos_avanzando == 0 && barcos_cruzando == 0) { // Cambiar el letrero solo si ningún barco está avanzando
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
void cambiar_letrero_equidad(){
  pthread_mutex_lock(&canal_mutex);
  if (barcos_avanzando == 0 && barcos_cruzando == 0) { //revisa si no hay barcos cruzando
    if (barcos_terminados == W || barcos_izq == 0 && strcmp(letrero, "izquierda") == 0) { //revisa el letrero y si ya pasaron los barcos W o no  hay mas
          strcpy(letrero, "derecha");
          barcos_terminados = 0;
          printf("\n[LETRERO] Ahora el sentido es hacia: %s\n\n", letrero);
        } else {
          if (barcos_terminados == W || barcos_der == 0 && strcmp(letrero, "derecha") == 0) {
            strcpy(letrero, "izquierda");
            barcos_terminados = 0;
            printf("\n[LETRERO] Ahora el sentido es hacia: %s\n\n", letrero);
          }
        }
      }
   pthread_mutex_unlock(&canal_mutex);
  }
int main() {
  int flag = 1;
  pthread_t hilos[MAX_BARCOS]; // Arreglo de hilos
    int hilo_index = 0; // Índice para el arreglo de hilos

    // Definir el ancho del canal
    printf("Ingrese el ancho del canal (en unidades de longitud): ");
    scanf("%d", &ancho_canal);

    num_barcos = leer_barcos("barcos.txt", barcos, MAX_BARCOS); // Leer los barcos desde el archivo

    if (num_barcos < 0) {
        return 1; // Error al leer el archivo
    }

    int algoritmo;
    printf("Seleccione el algoritmo de calendarización:\n");
    printf("1. Round Robin (RR)\n");
    printf("2. Prioridad\n");
    printf("3. SJF \n");
    printf("4. FCFS\n");
    printf("5. Tiempo real\n");
    scanf("%d", &algoritmo);

    int intervalo_letrero;
    printf("Ingrese el intervalo de tiempo (segundos) para cambiar el letrero: ");
    scanf("%d", &intervalo_letrero);

    // Inicializar mutex y condición
    pthread_mutex_init(&canal_mutex, NULL);
    pthread_cond_init(&canal_disponible, NULL);

    // Ordenar los barcos según el algoritmo seleccionado antes de crear los hilos
    if (algoritmo == 1) { // Round Robin
        setear_round_robin();
        }else if (algoritmo == 2) { // Prioridad
        setear_prioridad();
    } else if (algoritmo == 3) { // Shortest Job First
        setear_sjf();
    } else if (algoritmo == 4) { // FCFS
        setear_fcfs();
    } else if (algoritmo == 5) { // Tiempo Maximo
        setear_tiempo_real();
    }

    // Crear los hilos (barcos) que cruzarán el canal dependiendo del algoritmo seleccionado
	for (int i = 0; i < num_barcos && hilo_index < MAX_BARCOS; i++) {
	    if (algoritmo == 1) { // Round Robin
		if (pthread_create(&hilos[hilo_index], NULL, cruzar_canal_round_robin, (void*)&barcos[i]) != 0) {
		    perror("Error al crear el hilo Round Robin");
		}
	    } else if (algoritmo == 2) { // Prioridad
		if (pthread_create(&hilos[hilo_index], NULL, cruzar_canal_prioridad, (void*)&barcos[i]) != 0) {
		    perror("Error al crear el hilo Prioridad/SJF");
		}
	    } else if (algoritmo == 3) { //SJF
		if (pthread_create(&hilos[hilo_index], NULL, cruzar_canal_sjf, (void*)&barcos[i]) != 0) {
		    perror("Error al crear el hilo Prioridad/SJF");
		}
	    } else if (algoritmo == 4) { //FCFS
		if (pthread_create(&hilos[hilo_index], NULL, cruzar_canal_fcfs, (void*)&barcos[i]) != 0) {
		    perror("Error al crear el hilo Prioridad/SJF");
		}
	    } else if (algoritmo == 5) { //Tiempo Real
		if (pthread_create(&hilos[hilo_index], NULL, cruzar_canal_tiempo_real, (void*)&barcos[i]) != 0) {
		    perror("Error al crear el hilo Prioridad/SJF");
		}
	    } else {
		printf("Algoritmo no implementado.\n");
		return 1;
	    }
	    pthread_detach(hilos[hilo_index]); // Desvincular el hilo para no necesitar pthread_join
	    hilo_index++;
	}

    // Variables para monitorear el tiempo para cambiar el letrero
    time_t ultimo_cambio = time(NULL);

    int canal;
    printf("Seleccione el tipo de canal\n");
    printf("1. Equidad\n");
    printf("2. Letrero\n");
    printf("3. Tico\n");
    scanf("%d", &canal);

    if(canal == 3) {
      printf("Tipo de canal: Tico \n");
      while (flag) {
        sleep(1); // Intervalo corto para permitir la detección oportuna de cambios

        time_t ahora = time(NULL);
        if (difftime(ahora, ultimo_cambio) >= intervalo_letrero) {
            cambiar_letrero();
            ultimo_cambio = ahora;
        }

        // Actualizar barcos cada cierto tiempo (puedes ajustar el intervalo)
        // Por ejemplo, cada 5 segundos
        static time_t ultimo_actualizacion = 0;
        if (difftime(ahora, ultimo_actualizacion) >= 5.0) {
            pthread_mutex_lock(&canal_mutex); // Bloquear el mutex para proteger el acceso a los barcos

            int nuevo_num_barcos = actualizar_barcos("barcos.txt", barcos, num_barcos);
            if (nuevo_num_barcos < 0) {
                pthread_mutex_unlock(&canal_mutex);
                continue; // Error al actualizar, intentar de nuevo en el próximo ciclo
            }

	// Crear hilos para los nuevos barcos
	for (int i = num_barcos; i < nuevo_num_barcos && hilo_index < MAX_BARCOS; i++) {
	    if (barcos[i].cruzo == 0) { // Solo crear hilos para barcos que no han cruzado
		if (algoritmo == 1) { // Round Robin
		    if (pthread_create(&hilos[hilo_index], NULL, cruzar_canal_round_robin, (void*)&barcos[i]) != 0) {
		        perror("Error al crear el hilo Round Robin para nuevo barco");
		        continue;
		    }
		} else if (algoritmo == 2) { // Prioridad
		     setear_prioridad();
		    if (pthread_create(&hilos[hilo_index], NULL, cruzar_canal_prioridad, (void*)&barcos[i]) != 0) {
		        perror("Error al crear el hilo Prioridad para nuevo barco");
		        continue;
		    }
		} else if (algoritmo == 3) { // SJF
		     setear_sjf();
		    if (pthread_create(&hilos[hilo_index], NULL, cruzar_canal_sjf, (void*)&barcos[i]) != 0) {
		        perror("Error al crear el hilo SJF para nuevo barco");
		        continue;
		    }
		} else if (algoritmo == 4) { // FCFS
		     setear_fcfs();
		    if (pthread_create(&hilos[hilo_index], NULL, cruzar_canal_fcfs, (void*)&barcos[i]) != 0) {
		        perror("Error al crear el hilo FCFS para nuevo barco");
		        continue;
		    }
		} else if (algoritmo == 5) { // Tiempo Real
		     setear_tiempo_real();
		    if (pthread_create(&hilos[hilo_index], NULL, cruzar_canal_tiempo_real, (void*)&barcos[i]) != 0) {
		        perror("Error al crear el hilo Tiempo Real para nuevo barco");
		        continue;
		    }
		}
		pthread_detach(hilos[hilo_index]); // Desvincular el hilo
		printf("Hilo creado para el nuevo barco ID=%d\n", barcos[i].id);
		hilo_index++;
	    }
	}
            num_barcos = nuevo_num_barcos; // Actualizar el número de barcos
            ultimo_actualizacion = ahora;

            pthread_mutex_unlock(&canal_mutex); // Desbloquear el mutex
        }

        int todos_cruzaron = 1;
        pthread_mutex_lock(&canal_mutex);
        for (int i = 0; i < num_barcos; i++) {
            if (barcos[i].cruzo == 0) {
                todos_cruzaron = 0;
                break;
            }
        }
        pthread_mutex_unlock(&canal_mutex);
        if (todos_cruzaron) {
            flag = 0;
        }

    }
    printf("Todos los barcos han cruzado el canal.\n");
    return 0;
    }
    if (canal == 2){
      printf("Tipo de canal: Equidad \n");
    while (flag){
	    time_t ahora = time(NULL);
	    sleep(1);
	    if(barcos_izq == 0 && barcos_der == 0){
		    flag = 0;
	    }
	    cambiar_letrero_equidad();
	    static time_t ultimo_actualizacion = 0;
            if (difftime(ahora, ultimo_actualizacion) >= 5.0) {
                pthread_mutex_lock(&canal_mutex); // Bloquear el mutex para proteger el acceso a los barcos

                int nuevo_num_barcos = actualizar_barcos("barcos.txt", barcos, num_barcos);
                if (nuevo_num_barcos < 0) {
                    pthread_mutex_unlock(&canal_mutex);
                    continue; // Error al actualizar, intentar de nuevo en el próximo ciclo
            }
	    for (int i = num_barcos; i < nuevo_num_barcos && hilo_index < MAX_BARCOS; i++) {
	        if (barcos[i].cruzo == 0) { // Solo crear hilos para barcos que no han cruzado
		    if (algoritmo == 1) { // Round Robin
		        if (pthread_create(&hilos[hilo_index], NULL, cruzar_canal_round_robin, (void*)&barcos[i]) != 0) {
		            perror("Error al crear el hilo Round Robin para nuevo barco");
		            continue;
		        }
		    } else if (algoritmo == 2) { // Prioridad
		         setear_prioridad();
		        if (pthread_create(&hilos[hilo_index], NULL, cruzar_canal_prioridad, (void*)&barcos[i]) != 0) {
		            perror("Error al crear el hilo Prioridad para nuevo barco");
		            continue;
		        }
		    } else if (algoritmo == 3) { // SJF
		         setear_sjf();
		        if (pthread_create(&hilos[hilo_index], NULL, cruzar_canal_sjf, (void*)&barcos[i]) != 0) {
		            perror("Error al crear el hilo SJF para nuevo barco");
		            continue;
		        }
		    } else if (algoritmo == 4) { // FCFS
		         setear_fcfs();
		        if (pthread_create(&hilos[hilo_index], NULL, cruzar_canal_fcfs, (void*)&barcos[i]) != 0) {
		            perror("Error al crear el hilo FCFS para nuevo barco");
		            continue;
		        }
		    } else if (algoritmo == 5) { // Tiempo Real
		         setear_tiempo_real();
		        if (pthread_create(&hilos[hilo_index], NULL, cruzar_canal_tiempo_real, (void*)&barcos[i]) != 0) {
		            perror("Error al crear el hilo Tiempo Real para nuevo barco");
		            continue;
		        }
		    }
		    pthread_detach(hilos[hilo_index]); // Desvincular el hilo
		    printf("Hilo creado para el nuevo barco ID=%d\n", barcos[i].id);
		    hilo_index++;
	        }
	    }
            num_barcos = nuevo_num_barcos; // Actualizar el número de barcos
            ultimo_actualizacion = ahora;

            pthread_mutex_unlock(&canal_mutex); // Desbloquear el mutex
        }

}
    }
    // Nota: En este diseño, el programa está en un bucle infinito.
    // Para terminarlo de manera limpia, podrías implementar señales o condiciones de terminación.

    // Destruir el mutex y la condición (Nunca se alcanzará en este diseño)
    pthread_mutex_destroy(&canal_mutex);
    pthread_cond_destroy(&canal_disponible);
}
