#include <stdio.h>
#include <stdlib.h>

// Función para agregar barco al archivo barcos.txt
void agregarBarco(int id) {
    FILE *archivo = fopen("barcos.txt", "a");
    if (archivo == NULL) {
        printf("No se pudo abrir el archivo.\n");
        exit(1);
    }

    int tipo, oceano, prioridad, tiempo_max;
    char *tipo_str, *oceano_str;

    printf("Se ha abierto el archivo correctamente. Vamos a preguntar los datos del barco.\n");

    // Preguntar tipo de barco
    printf("Seleccione el tipo de barco:\n");
    printf("1. Normal (velocidad 1)\n");
    printf("2. Pesquero (velocidad 2)\n");
    printf("3. Patrulla (velocidad 3)\n");
    printf("Ingrese una opcion (1-3): ");
    scanf("%d", &tipo);

    switch (tipo) {
        case 1: tipo_str = "Normal"; break;
        case 2: tipo_str = "Pesquero"; break;
        case 3: tipo_str = "Patrulla"; break;
        default: printf("Tipo no válido\n"); fclose(archivo); return;
    }

    // Preguntar dirección
    printf("Seleccione el océano hacia el que va el barco:\n");
    printf("1. Derecha\n");
    printf("2. Izquierda\n");
    printf("Ingrese una opcion (1-2): ");
    scanf("%d", &oceano);

    switch (oceano) {
        case 1: oceano_str = "derecha"; break;
        case 2: oceano_str = "izquierda"; break;
        default: printf("Oceano no válido\n"); fclose(archivo); return;
    }

    // Preguntar prioridad
    printf("Ingrese la prioridad del barco (1-10, donde 1 es más alta): ");
    scanf("%d", &prioridad);

    if (prioridad < 1 || prioridad > 10) {
        printf("Prioridad no válida\n");
        fclose(archivo);
        return;
    }

    // Preguntar tiempo máximo
    printf("Ingrese el tiempo máximo que puede durar en cruzar: ");
    scanf("%d", &tiempo_max);

    // Esperar que el usuario presione Enter para agregar el barco
    printf("Presione Enter para agregar el barco al archivo...");
    getchar();  // Limpiar el buffer
    getchar();  // Esperar enter

    // Escribir los datos en el archivo
    fprintf(archivo, "%d %s %s %d %d\n", id, tipo_str, oceano_str, prioridad, tiempo_max);

    fclose(archivo);
    printf("Barco añadido con éxito.\n");
}

int main() {
    int id = 1;
    char continuar = 'y';

    printf("Intentando abrir el archivo para leer el último ID...\n");
    FILE *archivo = fopen("barcos.txt", "r");
    if (archivo != NULL) {
        char encabezado[100];
        fgets(encabezado, sizeof(encabezado), archivo);  // Saltar el encabezado

        int temp_id;
        while (fscanf(archivo, "%d", &temp_id) == 1) {
            id = temp_id + 1;  // Incrementar el ID
            fscanf(archivo, "%*[^\n]");  // Saltar el resto de la línea
        }
        fclose(archivo);
        printf("El último ID leído es %d\n", id);
    } else {
        printf("No se pudo abrir el archivo barcos.txt para leer el ID. Se asignará ID 1.\n");
    }

    while (continuar == 'y') {
        agregarBarco(id);
        id++;

        printf("¿Desea agregar otro barco? (y/n): ");
        scanf(" %c", &continuar);
    }

    printf("Programa terminado.\n");
    return 0;
}

