#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include "CEthreads.h"

#define QUANTUM 5 // Tiempo de quantum para el Round Robin
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
    int tiempo_total; // Add the missing semicolon here
} Barco;

// Variables globales para controlar el canal
char letrero[10] = "izquierda"; // Letrero inicial
int barcos_avanzando = 0; // Barcos que están avanzando
int barcos_cruzando = 0; // Barcos que están avanzando
int ancho_canal = 0; // Ancho del canal definido por el usuario
cemutex_t canal_mutex; // Mutex para proteger el acceso al canal
cecond_t canal_disponible; // Condición para indicar cuándo el canal está disponible

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

int leer_config(const char* archivo_config, int* algoritmo, int* valor_control, int* ancho_canal, int* intervalo_letrero) {
    FILE* file = fopen(archivo_config, "r");
    if (file == NULL) {
        perror("No se puede abrir el archivo config.txt");
        return -1;
    }

    char buffer[256];

    // Leer la cabecera del archivo (ignorar la primera línea)
    fgets(buffer, sizeof(buffer), file);

    // Leer la segunda línea que contiene los datos
    char algoritmo_str[10], control[10];
    if (fgets(buffer, sizeof(buffer), file)) {
        sscanf(buffer, "%s %s %d %d %d", algoritmo_str, control, valor_control, ancho_canal, intervalo_letrero);

        // Convertir el algoritmo a un número para manejarlo en el código
        if (strcmp(algoritmo_str, "RR") == 0) {
            *algoritmo = 1;
        } else if (strcmp(algoritmo_str, "Prioridad") == 0) {
            *algoritmo = 2;
        } else if (strcmp(algoritmo_str, "SJF") == 0) {
            *algoritmo = 3;
        } else if (strcmp(algoritmo_str, "FCFS") == 0) {
            *algoritmo = 4;
        } else if (strcmp(algoritmo_str, "Tiempo") == 0) {
            *algoritmo = 5;
        } else {
            printf("Algoritmo no reconocido en config.txt.\n");
            fclose(file);
            return -1;
        }

        // Imprimir los valores de control para debug
        printf("Algoritmo: %s\n", algoritmo_str);
        printf("Control: %s\n", control);
        printf("Valor del Control: %d\n", *valor_control);
        printf("Ancho del Canal: %d\n", *ancho_canal);
        printf("Intervalo del Letrero: %d\n", *intervalo_letrero);
    }

    fclose(file);
    return 0;
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

void registrar_movimiento_barco(int id_barco, int tiempo_avanzado) {
    static int first_time = 1;
    FILE *file;

    if (first_time) {
        file = fopen("output.txt", "w");
        if (file == NULL) {
            perror("Error al abrir output.txt");
            return;
        }
        fclose(file);
        first_time = 0;
    }

    file = fopen("output.txt", "a");
    if (file == NULL) {
        perror("Error al abrir output.txt");
        return;
    }

    // Escribir la información en el archivo
    fprintf(file, "%d,%d\n", id_barco, tiempo_avanzado);

    fclose(file);
}

void* cruzar_canal_tiempo_real(void* arg) {
    
}

// Función que simula el cruce completo de un barco con algoritmo de prioridad
void* cruzar_canal_prioridad(void* arg) {
    
}

// Función que simula el cruce completo de un barco con algoritmo de prioridad
void* cruzar_canal_fcfs(void* arg) {
    
}

void* cruzar_canal_sjf(void* arg) {
   
}

void* cruzar_canal_round_robin(void* arg) {
    Barco* barco = (Barco*)arg;
    int conto_barco = 0;

    while (barco->tiempo_cruzar > 0) {
        cemutex_lock(&canal_mutex);

        while (strcmp(letrero, barco->oceano) != 0 || barcos_avanzando >= 1 ||
               (strcmp(barco->oceano, "derecha") == 0 && barco->id != id_barco_prioridad_der) ||
               (strcmp(barco->oceano, "izquierda") == 0 && barco->id != id_barco_prioridad_izq)) {
            cecond_wait(&canal_disponible, &canal_mutex);
        }

        barcos_avanzando++;
        if (conto_barco == 0) {
            barcos_cruzando++;
            conto_barco = 1;
        }

        cemutex_unlock(&canal_mutex);

        int tiempo_a_avanzar = (barco->tiempo_cruzar > QUANTUM) ? QUANTUM : barco->tiempo_cruzar;
        printf("Barco %d (Tipo: %s, Océano: %s) avanza por %d segundos...\n", barco->id, barco->tipo, barco->oceano, tiempo_a_avanzar);
        
        // Registrar el movimiento del barco
        registrar_movimiento_barco(barco->id, tiempo_a_avanzar);
        
        //sleep(tiempo_a_avanzar);

        barco->tiempo_cruzar -= tiempo_a_avanzar;

        cemutex_lock(&canal_mutex);
        barcos_avanzando--;

        if (barco->tiempo_cruzar <= 0) {
            printf("Barco %d ha cruzado el canal completamente.\n", barco->id);
            barco->cruzo = 1;
            barcos_cruzando--;
            
            // Registrar que el barco ha completado el cruce
            registrar_movimiento_barco(barco->id, 0);
        }

        setear_round_robin();
        cecond_broadcast(&canal_disponible);

        cemutex_unlock(&canal_mutex);

        sleep(1);
    }

    cethread_exit(NULL);
}

// Función que cambia el letrero de "izquierda" a "derecha" y viceversa
void cambiar_letrero() {
    cemutex_lock(&canal_mutex); // Bloquear el mutex para asegurar que no haya barcos avanzando
    if (barcos_avanzando == 0 && barcos_cruzando == 0) { // Cambiar el letrero solo si ningún barco está avanzando
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

int main() {
    cethread_t hilos[MAX_BARCOS]; // Arreglo de hilos
    int hilo_index = 0; // Índice para el arreglo de hilos

    barcos_izq_der("barcos.txt");
    printf("Number of boats in the left ocean: %d\n", barcos_izq);
    printf("Number of boats in the right ocean: %d\n", barcos_der);

    // Variables para almacenar los valores de config.txt
    int algoritmo, valor_control, intervalo_letrero;

    // Leer el archivo config.txt
    if (leer_config("config.txt", &algoritmo, &valor_control, &ancho_canal, &intervalo_letrero) < 0) {
        return 1; // Error al leer config.txt
    }

    // Leer los barcos desde el archivo barcos.txt
    num_barcos = leer_barcos("barcos.txt", barcos, MAX_BARCOS);
    if (num_barcos < 0) {
        return 1; // Error al leer el archivo de barcos
    }

    // Imprimir información de los barcos
    printf("Barcos cargados:\n");
    for (int i = 0; i < num_barcos; i++) {
        printf("Barco ID=%d, Tipo=%s, Océano=%s, Prioridad=%d, Tiempo Cruzar=%d\n",
               barcos[i].id, barcos[i].tipo, barcos[i].oceano, barcos[i].prioridad, barcos[i].tiempo_cruzar);
    }

    // Imprimir el algoritmo seleccionado
    printf("Algoritmo seleccionado: ");
    switch (algoritmo) {
        case 1: printf("Round Robin\n"); break;
        case 2: printf("Prioridad\n"); break;
        case 3: printf("Shortest Job First (SJF)\n"); break;
        case 4: printf("First Come First Serve (FCFS)\n"); break;
        case 5: printf("Tiempo Real\n"); break;
        default: printf("Algoritmo no válido\n"); return 1;
    }

    // Inicializar mutex y condición
    cemutex_init(&canal_mutex);
    cecond_init(&canal_disponible);

    // Configurar el algoritmo
    if (algoritmo == 1) {
        setear_round_robin();
    } else if (algoritmo == 2) {
        setear_prioridad();
    } else if (algoritmo == 3) {
        setear_sjf();
    } else if (algoritmo == 4) {
        setear_fcfs();
    } else if (algoritmo == 5) {
        setear_tiempo_real();
    }

    // Crear los hilos para los barcos
    for (int i = 0; i < num_barcos && hilo_index < MAX_BARCOS; i++) {
        if (algoritmo == 1) {
            if (cethread_create(&hilos[hilo_index], NULL, cruzar_canal_round_robin, (void*)&barcos[i]) != 0) {
                perror("Error al crear el hilo Round Robin");
            }
        } else if (algoritmo == 2) {
            if (cethread_create(&hilos[hilo_index], NULL, cruzar_canal_prioridad, (void*)&barcos[i]) != 0) {
                perror("Error al crear el hilo Prioridad");
            }
        } else if (algoritmo == 3) {
            if (cethread_create(&hilos[hilo_index], NULL, cruzar_canal_sjf, (void*)&barcos[i]) != 0) {
                perror("Error al crear el hilo SJF");
            }
        } else if (algoritmo == 4) {
            if (cethread_create(&hilos[hilo_index], NULL, cruzar_canal_fcfs, (void*)&barcos[i]) != 0) {
                perror("Error al crear el hilo FCFS");
            }
        } else if (algoritmo == 5) {
            if (cethread_create(&hilos[hilo_index], NULL, cruzar_canal_tiempo_real, (void*)&barcos[i]) != 0) {
                perror("Error al crear el hilo Tiempo Real");
            }
        }
        cethread_detach(&hilos[hilo_index]);
        hilo_index++;
    }

    // Variables para monitorear el tiempo para cambiar el letrero
    time_t ultimo_cambio = time(NULL);

    while (1) {
        sleep(1); // Intervalo corto para permitir la detección oportuna de cambios

        time_t ahora = time(NULL);
        if (difftime(ahora, ultimo_cambio) >= intervalo_letrero) {
            cambiar_letrero();
            ultimo_cambio = ahora;
        }

        // Actualizar barcos cada 5 segundos
        static time_t ultimo_actualizacion = 0;
        if (difftime(ahora, ultimo_actualizacion) >= 5.0) {
            cemutex_lock(&canal_mutex);

            int nuevo_num_barcos = actualizar_barcos("barcos.txt", barcos, num_barcos);
            if (nuevo_num_barcos >= 0) {
                num_barcos = nuevo_num_barcos;
            }
            cemutex_unlock(&canal_mutex);

            ultimo_actualizacion = ahora;
        }

        // Verificar si todos los barcos han cruzado
        int todos_cruzaron = 1;
        cemutex_lock(&canal_mutex);
        for (int i = 0; i < num_barcos; i++) {
            if (barcos[i].cruzo == 0) {
                todos_cruzaron = 0;
                break;
            }
        }
        cemutex_unlock(&canal_mutex);
        if (todos_cruzaron) {
            break;
        }
    }

    cemutex_destroy(&canal_mutex);
    cecond_destroy(&canal_disponible);

    printf("Todos los barcos han cruzado el canal.\n");

    return 0;
}
