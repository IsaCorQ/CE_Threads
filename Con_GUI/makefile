# Variables
CC = gcc
CFLAGS = `pkg-config --cflags --libs gtk+-3.0` -lm
OBJ_FILES = CEthreads.c

# Targets
all: gui calendar

# Compilar gui
gui: gui.c calendar.c $(OBJ_FILES)
	$(CC) -o gui gui.c $(OBJ_FILES) $(CFLAGS)

# Compilar calendar
calendar: calendar.c $(OBJ_FILES)
	$(CC) -o calendar calendar.c $(OBJ_FILES)

gui-completo: gui.c CEthreads.c
	$(CC) -o gui gui.c CEthreads.c $(CFLAGS)

# Ejecutar gui
run-gui:
	./gui

# Ejecutar calendar
run-calendar:
	./calendar

# Limpiar archivos ejecutables
clean:
	rm -f gui calendar