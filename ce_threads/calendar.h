#ifndef CALENDAR_H
#define CALENDAR_H

#define MAX_BOATS 100

typedef struct {
    int id;
    char tipo[20];
    char oceano[20];
    int prioridad;
    int tiempo_sjf;
    int tiempo_maximo;
    int tiempo_restante;
    int velocidad;
    // Nuevas variables para la posición
    int x;
    int y;
    // Variable para indicar si el barco está en movimiento
    int en_movimiento;
    int tiempo_a_avanzar
} Barco;

extern Barco barcos[MAX_BOATS];
extern int num_barcos;

// Función para obtener el estado actual de los barcos
void obtener_estado_barcos(Barco* barcos_gui, int* num_barcos_gui);

// Función para inicializar la simulación
void inicializar_simulacion(const char* archivo_barcos, int ancho_canal);

// Función para ejecutar un paso de la simulación
void ejecutar_paso_simulacion(void);

#endif // CALENDAR_H