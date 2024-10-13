#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <math.h>
#include "CEthreads.h"

#define MAX_BOATS 100

typedef struct {
    int id;
    char type[20];
    char ocean[20];
    int priority;
    int sjf_time;
    int max_time;
    int x;
    int y;
    GdkPixbuf *pixbuf;
    gboolean finished;
    int total_distance;
    int distance_moved;
} Boat;

typedef struct {
    GtkWidget *window;
    GtkWidget *canvas;
    GtkWidget *algorithm_combo;
    GtkWidget *control_combo;
    GtkWidget *equity_frame;
    GtkWidget *equity_entry;
    GtkWidget *left_normal_entry;
    GtkWidget *left_fishing_entry;
    GtkWidget *left_patrol_entry;
    GtkWidget *right_normal_entry;
    GtkWidget *right_fishing_entry;
    GtkWidget *right_patrol_entry;
    gboolean animation_running;
    guint animation_timeout_id;
    GdkPixbuf *normal_boat_pixbuf;
    GdkPixbuf *fishing_boat_pixbuf;
    GdkPixbuf *patrol_boat_pixbuf;
    GdkPixbuf *ocean_pixbuf;
    GtkWidget *width_entry;
    GtkWidget *interval_entry;
    Boat boats[MAX_BOATS];
    int boat_count;
    int canal_width;
} BoatSimulator;

/// Function prototypes
static void load_boat_images(BoatSimulator *sim);
static void create_ui(BoatSimulator *sim);
static void draw_canvas(GtkWidget *widget, cairo_t *cr, gpointer data);
static gboolean animate_boats(gpointer data);
static void start_animation(GtkWidget *widget, gpointer data);
static void control_changed(GtkComboBox *widget, gpointer data);
static int get_validated_user_input(GtkWindow *parent, const char *prompt);
static void save_data(GtkWidget *widget, gpointer data);


static Boat displayed_boats[MAX_BOATS];
static int boat_count = 0;

void *run_calendar(void *arg) {
    system("./calendar");
    return NULL;
}

static void load_boat_images(BoatSimulator *sim) {
    // Load boat images
    GError *error = NULL;
    sim->normal_boat_pixbuf = gdk_pixbuf_new_from_file_at_scale(
        "assets/barco_normal.png", 30, 30, TRUE, &error);
    sim->fishing_boat_pixbuf = gdk_pixbuf_new_from_file_at_scale(
        "assets/barco_pesquero.png", 30, 30, TRUE, &error);
    sim->patrol_boat_pixbuf = gdk_pixbuf_new_from_file_at_scale(
        "assets/barco_patrulla.png", 30, 30, TRUE, &error);
    sim->ocean_pixbuf = gdk_pixbuf_new_from_file_at_scale(
        "assets/oceano.jpg", 700, 600, TRUE, &error);
    
    if (error) {
        g_print("Error loading images: %s\n", error->message);
        g_error_free(error);
    }
}

static void draw_canvas(GtkWidget *widget, cairo_t *cr, gpointer data) {
    BoatSimulator *sim = (BoatSimulator *)data;
    
    // Draw ocean background
    if (sim->ocean_pixbuf) {
        gdk_cairo_set_source_pixbuf(cr, sim->ocean_pixbuf, 75, 75);
        cairo_paint(cr);
    } 
    
    // Draw zones
    cairo_set_line_width(cr, 2);
    
    // Left green zone
    cairo_set_source_rgb(cr, 0, 1, 0);
    cairo_rectangle(cr, 10, 10, 430, 80);
    cairo_stroke(cr);
    
    // Right orange zone
    cairo_set_source_rgb(cr, 1, 0.5, 0);
    cairo_rectangle(cr, 460, 10, 430, 80);
    cairo_stroke(cr);
    
    // Left parking (orange)
    cairo_set_source_rgb(cr, 1, 0.5, 0);
    cairo_rectangle(cr, 10, 610, 430, 80);
    cairo_stroke(cr);
    
    // Right parking (green)
    cairo_set_source_rgb(cr, 0, 1, 0);
    cairo_rectangle(cr, 460, 610, 430, 80);
    cairo_stroke(cr);
    
    // Draw boats
    for (int i = 0; i < sim->boat_count; i++) {
        Boat *boat = &sim->boats[i];
        if (boat->pixbuf) {
            gdk_cairo_set_source_pixbuf(cr, boat->pixbuf, boat->x, boat->y);
            cairo_paint(cr);
        }
    }
}

static gboolean animate_boats(gpointer data) {
    BoatSimulator *sim = (BoatSimulator *)data;
    static FILE *output_file = NULL;
    static int current_boat_id = -1;
    static int moves_left = 0;

    if (!output_file) {
        output_file = fopen("output.txt", "r");
        if (!output_file) {
            g_print("Error opening output.txt\n");
            sim->animation_running = FALSE;
            return G_SOURCE_REMOVE;
        }
    }

    Boat *boat = NULL;
    if (current_boat_id != -1) {
        for (int i = 0; i < sim->boat_count; i++) {
            if (sim->boats[i].id == current_boat_id) {
                boat = &sim->boats[i];
                break;
            }
        }
    }

    // If we don't have a current boat or the current boat has finished its moves, get the next instruction
    if (!boat || moves_left == 0) {
        sleep(1);  // Wait for the boat to park
        int id, moves;
        if (fscanf(output_file, "%d,%d", &id, &moves) == 2) {
            current_boat_id = id;
            moves_left = moves;
            printf("New instruction: Boat ID %d, Moves: %d\n", id, moves);
            
            // Find the current boat
            for (int i = 0; i < sim->boat_count; i++) {
                if (sim->boats[i].id == current_boat_id) {
                    boat = &sim->boats[i];
                    break;
                }
            }

            // If moves is 0, park the boat immediately
            if (moves == 0) {
                printf("Boat %d is parking.\n", boat->id);
                if (strcmp(boat->ocean, "izquierda") == 0) {
                    boat->x = 460 + (rand() % 400);  // Random x position in right parking
                } else {
                    boat->x = 10 + (rand() % 400);  // Random x position in left parking
                }
                boat->y = 610 + (rand() % 70);  // Random y position in parking
                boat->finished = TRUE;
                printf("Boat %d parked at x=%d, y=%d\n", boat->id, boat->x, boat->y);
                moves_left = 0;  // Ensure we move to the next instruction on the next iteration
                return G_SOURCE_CONTINUE;
            }
        } else {
            fclose(output_file);
            output_file = NULL;
            sim->animation_running = FALSE;
            return G_SOURCE_REMOVE;
        }
    }

    if (boat && !boat->finished) {
        printf("Moving Boat ID %d (%s) from %s side\n", boat->id, boat->type, boat->ocean);
        
        int speed_multiplier;
        if (strcmp(boat->type, "Normal") == 0) speed_multiplier = 10;
        else if (strcmp(boat->type, "Pesquero") == 0) speed_multiplier = 20;
        else speed_multiplier = 50; // Patrulla

        // Calculate move distance based on speed
        int move_distance = 5 * speed_multiplier;

        // Calculate target position
        int target_x, target_y;
        if (strcmp(boat->ocean, "izquierda") == 0) {
            target_x = 450;
            target_y = 350;
        } else {
            target_x = 450;
            target_y = 350;
        }

        // Calculate direction vector
        float dx = target_x - boat->x;
        float dy = target_y - boat->y;
        float length = sqrt(dx*dx + dy*dy);
        if (length > 0) {
            dx /= length;
            dy /= length;
        }

        // Move the boat diagonally
        boat->x += (int)(dx * move_distance);
        boat->y += (int)(dy * move_distance);

        // Ensure the boat doesn't overshoot the target
        if ((strcmp(boat->ocean, "izquierda") == 0 && boat->x > target_x) ||
            (strcmp(boat->ocean, "derecha") == 0 && boat->x < target_x)) {
            boat->x = target_x;
        }
        if (boat->y > target_y) {
            boat->y = target_y;
        }

        moves_left--;
        printf("Boat %d moved. New position: x=%d, y=%d. Moves left: %d\n", 
               boat->id, boat->x, boat->y, moves_left);
    }

    // Force an immediate redraw of the canvas
    gtk_widget_queue_draw(sim->canvas);
    
    // Process all pending GTK events to update the GUI
    while (gtk_events_pending()) {
        gtk_main_iteration();
    }
    
    // Add a small delay to control animation speed
    usleep(100000);  // Sleep for 100ms (0.1 seconds)

    return G_SOURCE_CONTINUE;
}   

static void start_animation(GtkWidget *widget, gpointer data) {
    BoatSimulator *sim = (BoatSimulator *)data;
    
    if (sim->animation_running) {
        return;
    }
    
    FILE *f = fopen("barcos.txt", "r");
    if (!f) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(sim->window),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            "El archivo barcos.txt no existe. Guarde los datos primero.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    fclose(f);

    // Crear e iniciar el hilo de calendar usando CEthreads
    cethread_t calendar_thread;
    if (cethread_create(&calendar_thread, NULL, run_calendar, NULL) != 0) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(sim->window),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            "Error al iniciar el hilo de calendar.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }

    sim->animation_running = TRUE;
    sim->animation_timeout_id = g_timeout_add(50, animate_boats, sim);
}


static void control_changed(GtkComboBox *widget, gpointer data) {
    BoatSimulator *sim = (BoatSimulator *)data;
    gchar *selected = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget));
    
    if (g_strcmp0(selected, "Equidad") == 0) {
        gtk_widget_show(sim->equity_frame);
    } else {
        gtk_widget_hide(sim->equity_frame);
    }
    
    g_free(selected);
}

static void create_ui(BoatSimulator *sim) {
    // Window setup
    sim->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(sim->window), "Boat Traffic Simulator");
    gtk_window_set_default_size(GTK_WINDOW(sim->window), 900, 700);
    g_signal_connect(G_OBJECT(sim->window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
    
    // Main layout
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_add(GTK_CONTAINER(sim->window), hbox);
    
    // Left control panel
    GtkWidget *left_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(hbox), left_panel, FALSE, FALSE, 0);
    
    // Boat selection
    GtkWidget *boat_frame = gtk_frame_new("Selección de Barcos");
    gtk_box_pack_start(GTK_BOX(left_panel), boat_frame, FALSE, FALSE, 0);
    
    GtkWidget *boat_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_add(GTK_CONTAINER(boat_frame), boat_box);
    
    // Left boats
    GtkWidget *left_boat_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_box_pack_start(GTK_BOX(boat_box), left_boat_box, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(left_boat_box), 
        gtk_label_new("izquierda"), FALSE, FALSE, 0);
    
    sim->left_normal_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(sim->left_normal_entry), "0");
    gtk_box_pack_start(GTK_BOX(left_boat_box), 
        gtk_label_new("Normal"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(left_boat_box), 
        sim->left_normal_entry, FALSE, FALSE, 0);
    
    sim->left_fishing_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(sim->left_fishing_entry), "0");
    gtk_box_pack_start(GTK_BOX(left_boat_box), 
        gtk_label_new("Pesquero"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(left_boat_box), 
        sim->left_fishing_entry, FALSE, FALSE, 0);
    
    sim->left_patrol_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(sim->left_patrol_entry), "0");
    gtk_box_pack_start(GTK_BOX(left_boat_box), 
        gtk_label_new("Patrulla"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(left_boat_box), 
        sim->left_patrol_entry, FALSE, FALSE, 0);
    
    // Right boats
    GtkWidget *right_boat_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_box_pack_start(GTK_BOX(boat_box), right_boat_box, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(right_boat_box), 
        gtk_label_new("derecha"), FALSE, FALSE, 0);
    
    sim->right_normal_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(sim->right_normal_entry), "0");
    gtk_box_pack_start(GTK_BOX(right_boat_box), 
        gtk_label_new("Normal"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(right_boat_box), 
        sim->right_normal_entry, FALSE, FALSE, 0);
    
    sim->right_fishing_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(sim->right_fishing_entry), "0");
    gtk_box_pack_start(GTK_BOX(right_boat_box), 
        gtk_label_new("Pesquero"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(right_boat_box), 
        sim->right_fishing_entry, FALSE, FALSE, 0);
    
    sim->right_patrol_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(sim->right_patrol_entry), "0");
    gtk_box_pack_start(GTK_BOX(right_boat_box), 
        gtk_label_new("Patrulla"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(right_boat_box), 
        sim->right_patrol_entry, FALSE, FALSE, 0);


    // Use the BoatSimulator structure fields
    sim->width_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(sim->width_entry), "10");

    sim->interval_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(sim->interval_entry), "5");

    // Declare the labels for "Ancho del canal" and "Intervalo de tiempo"
    GtkWidget *width_label = gtk_label_new("Ancho del canal");
    GtkWidget *interval_label = gtk_label_new("Intervalo de tiempo (segundos)");

    gtk_box_pack_start(GTK_BOX(left_panel), width_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(left_panel), sim->width_entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(left_panel), interval_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(left_panel), sim->interval_entry, FALSE, FALSE, 0);


    
    // Algorithm selection
    gtk_box_pack_start(GTK_BOX(left_panel), 
        gtk_label_new("Algoritmo"), FALSE, FALSE, 0);
    sim->algorithm_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sim->algorithm_combo), "Round Robin");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sim->algorithm_combo), "Prioridad");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sim->algorithm_combo), "Shortest Job First");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sim->algorithm_combo), "First Come First Serve");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sim->algorithm_combo), "Tiempo Real");
    gtk_combo_box_set_active(GTK_COMBO_BOX(sim->algorithm_combo), 0);
    gtk_box_pack_start(GTK_BOX(left_panel), sim->algorithm_combo, FALSE, FALSE, 0);
    
    // Control selection
    gtk_box_pack_start(GTK_BOX(left_panel), 
        gtk_label_new("Control"), FALSE, FALSE, 0);
    sim->control_combo = gtk_combo_box_text_new();
    // Control selection (continued)
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sim->control_combo), "Equidad");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sim->control_combo), "Letrero");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sim->control_combo), "Tico");
    gtk_combo_box_set_active(GTK_COMBO_BOX(sim->control_combo), 0);
    gtk_box_pack_start(GTK_BOX(left_panel), sim->control_combo, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(sim->control_combo), "changed", G_CALLBACK(control_changed), sim);
    
    // Equity frame
    sim->equity_frame = gtk_frame_new("Equidad");
    gtk_box_pack_start(GTK_BOX(left_panel), sim->equity_frame, FALSE, FALSE, 0);
    sim->equity_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(sim->equity_entry), "1");
    gtk_container_add(GTK_CONTAINER(sim->equity_frame), sim->equity_entry);
    gtk_widget_hide(sim->equity_frame);
    
    // Buttons
    GtkWidget *save_button = gtk_button_new_with_label("Guardar");
    g_signal_connect(G_OBJECT(save_button), "clicked", G_CALLBACK(save_data), sim);
    gtk_box_pack_start(GTK_BOX(left_panel), save_button, FALSE, FALSE, 0);
    
    GtkWidget *start_button = gtk_button_new_with_label("Iniciar");
    g_signal_connect(G_OBJECT(start_button), "clicked", G_CALLBACK(start_animation), sim);
    gtk_box_pack_start(GTK_BOX(left_panel), start_button, FALSE, FALSE, 0);
    
    // Canvas setup
    sim->canvas = gtk_drawing_area_new();
    gtk_widget_set_size_request(sim->canvas, 900, 700);
    g_signal_connect(G_OBJECT(sim->canvas), "draw", G_CALLBACK(draw_canvas), sim);
    gtk_box_pack_start(GTK_BOX(hbox), sim->canvas, TRUE, TRUE, 0);
}

static int get_validated_user_input(GtkWindow *parent, const char *prompt) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Input",
        parent,
        GTK_DIALOG_MODAL,
        "_OK",
        GTK_RESPONSE_ACCEPT,
        "_Cancel",
        GTK_RESPONSE_CANCEL,
        NULL);
    
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *label = gtk_label_new(prompt);
    GtkWidget *entry = gtk_entry_new();
    
    gtk_container_add(GTK_CONTAINER(content_area), label);
    gtk_container_add(GTK_CONTAINER(content_area), entry);
    gtk_widget_show_all(dialog);
    
    int result = -1;
    while (result == -1) {
        int response = gtk_dialog_run(GTK_DIALOG(dialog));
        if (response == GTK_RESPONSE_ACCEPT) {
            const char *text = gtk_entry_get_text(GTK_ENTRY(entry));
            char *endptr;
            long val = strtol(text, &endptr, 10);
            if (*endptr == '\0' && val >= 0 && val <= INT_MAX) {
                result = (int)val;
            } else {
                GtkWidget *error_dialog = gtk_message_dialog_new(GTK_WINDOW(dialog),
                    GTK_DIALOG_MODAL,
                    GTK_MESSAGE_ERROR,
                    GTK_BUTTONS_OK,
                    "Por favor, ingrese un número entero no negativo.");
                gtk_dialog_run(GTK_DIALOG(error_dialog));
                gtk_widget_destroy(error_dialog);
            }
        } else {
            break;
        }
    }
    
    gtk_widget_destroy(dialog);
    return result;
}

static void save_data(GtkWidget *widget, gpointer data) {
    BoatSimulator *sim = (BoatSimulator *)data;
    
    // Clear existing boats
    boat_count = 0;
    
    FILE *boats_file = fopen("barcos.txt", "w");
    FILE *config_file = fopen("config.txt", "w");
    
    if (!boats_file || !config_file) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(sim->window),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            "Error al abrir archivos para guardar.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        if (boats_file) fclose(boats_file);
        if (config_file) fclose(config_file);
        return;
    }
    
    // Write headers
    fprintf(boats_file, "ID Tipo Océano Prioridad Tiempo_SJF Tiempo_Maximo\n");
    fprintf(config_file, "Algoritmo Control ValoControl AnchoCanal IntervaloLetrero\n");
    
    // Process and save boat data
    int id_counter = 1;
    const char *directions[] = {"izquierda", "derecha"};
    GtkWidget *entries[][3] = {
        {sim->left_normal_entry, sim->left_fishing_entry, sim->left_patrol_entry},
        {sim->right_normal_entry, sim->right_fishing_entry, sim->right_patrol_entry}
    };
    const char *types[] = {"Normal", "Pesquero", "Patrulla"};
    
    // Calculate initial positions and offsets
    int left_x = 20;
    int right_x = 470;
    int y_offset = 20;
    int max_boats_per_row = 10;
    int left_boats_count = 0;
    int right_boats_count = 0;
    
    for (int dir = 0; dir < 2; dir++) {
        for (int type = 0; type < 3; type++) {
            const char *count_text = gtk_entry_get_text(GTK_ENTRY(entries[dir][type]));
            int count = atoi(count_text);
            
            for (int i = 0; i < count && sim->boat_count < MAX_BOATS; i++) {
                char prompt[100];
                snprintf(prompt, sizeof(prompt), 
                    "Ingrese la prioridad para el barco %s %d:", types[type], id_counter);
                int priority = get_validated_user_input(GTK_WINDOW(sim->window), prompt);
                
                snprintf(prompt, sizeof(prompt), 
                    "Ingrese el Tiempo SJF para el barco %s %d:", types[type], id_counter);
                int sjf_time = get_validated_user_input(GTK_WINDOW(sim->window), prompt);
                
                snprintf(prompt, sizeof(prompt), 
                    "Ingrese el Tiempo Máximo para el barco %s %d:", types[type], id_counter);
                int max_time = get_validated_user_input(GTK_WINDOW(sim->window), prompt);
                
                if (priority == -1 || sjf_time == -1 || max_time == -1) {
                    fclose(boats_file);
                    fclose(config_file);
                    return;
                }
                
                fprintf(boats_file, "%d %s %s %d %d %d\n",
                    id_counter, types[type], directions[dir], priority, sjf_time, max_time);
                
                // Add boat to the simulator's boat array
                Boat *boat = &sim->boats[sim->boat_count];
                boat->id = id_counter;
                strcpy(boat->type, types[type]);
                strcpy(boat->ocean, directions[dir]);
                boat->priority = priority;
                boat->sjf_time = sjf_time;
                boat->max_time = max_time;
                boat->finished = FALSE;

                int *boats_count = (dir == 0) ? &left_boats_count : &right_boats_count;
                int row = *boats_count / max_boats_per_row;
                int col = *boats_count % max_boats_per_row;
                
                if (dir == 0) {  // Left side
                    boat->x = left_x + (col * 40);
                    boat->y = y_offset + (row * 40);
                } else {  // Right side
                    boat->x = right_x + (col * 40);
                    boat->y = y_offset + (row * 40);
                }
                
                if (strcmp(types[type], "Normal") == 0)
                    boat->pixbuf = sim->normal_boat_pixbuf;
                else if (strcmp(types[type], "Pesquero") == 0)
                    boat->pixbuf = sim->fishing_boat_pixbuf;
                else
                    boat->pixbuf = sim->patrol_boat_pixbuf;
                
                (*boats_count)++;
                sim->boat_count++;
                id_counter++;
            }
        }
    }
    
    // Save config data
    const char *algorithm = gtk_combo_box_text_get_active_text(
        GTK_COMBO_BOX_TEXT(sim->algorithm_combo));
    const char *control = gtk_combo_box_text_get_active_text(
        GTK_COMBO_BOX_TEXT(sim->control_combo));
    const char *equity = gtk_entry_get_text(GTK_ENTRY(sim->equity_entry));

    // Get the values for Canal Width and Time Interval
    const char *canal_width = gtk_entry_get_text(GTK_ENTRY(sim->width_entry));
    const char *time_interval = gtk_entry_get_text(GTK_ENTRY(sim->interval_entry));


    // Save canal width
    const char *canal_width_str = gtk_entry_get_text(GTK_ENTRY(sim->width_entry));
    sim->canal_width = atoi(canal_width_str);


    
    // Convert algorithm to initials
    const char *algorithm_initial;
    if (strcmp(algorithm, "Round Robin") == 0) algorithm_initial = "RR";
    else if (strcmp(algorithm, "Prioridad") == 0) algorithm_initial = "P";
    else if (strcmp(algorithm, "Shortest Job First") == 0) algorithm_initial = "SJF";
    else if (strcmp(algorithm, "First Come First Serve") == 0) algorithm_initial = "FCFS";
    else if (strcmp(algorithm, "Tiempo Real") == 0) algorithm_initial = "TR";
    else algorithm_initial = algorithm;
    
    fprintf(config_file, "%s %s %s %s %s\n", algorithm_initial, control,
        strcmp(control, "Equidad") == 0 ? equity : "0", canal_width, time_interval);

    fclose(boats_file);
    fclose(config_file);
    
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(sim->window),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK,
        "Datos guardados correctamente en barcos.txt y config.txt");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    
    gtk_widget_queue_draw(sim->canvas);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    
    BoatSimulator sim = {0};
    sim.animation_running = FALSE;
    
    load_boat_images(&sim);
    create_ui(&sim);
    
    gtk_widget_show_all(sim.window);
    gtk_main();
    
    return 0;
}    