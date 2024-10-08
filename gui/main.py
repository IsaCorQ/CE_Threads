import tkinter as tk
from tkinter import ttk, messagebox, simpledialog
from PIL import Image, ImageTk
import os

class BoatSimulator:
    def __init__(self, root):
        self.root = root
        self.root.title("Boat Traffic Simulator")

        # Variables
        self.algoritmo = tk.StringVar()
        self.control = tk.StringVar()
        self.equidad = tk.StringVar(value="1")

        self.barcos_izquierda = {
            "Normales": tk.StringVar(value="0"),
            "Pesqueros": tk.StringVar(value="0"),
            "Patrulla": tk.StringVar(value="0")
        }

        self.barcos_derecha = {
            "Normales": tk.StringVar(value="0"),
            "Pesqueros": tk.StringVar(value="0"),
            "Patrulla": tk.StringVar(value="0")
        }

        # Load boat images
        self.boat_images = self.load_boat_images()

        # Animation variables
        self.animation_running = False
        self.displayed_boats = []  # List to keep track of displayed boats

        # Create frames
        self.frame_izquierdo = ttk.Frame(root, padding="10")
        self.frame_izquierdo.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))

        self.frame_derecho = ttk.Frame(root, padding="10")
        self.frame_derecho.grid(row=0, column=1, sticky=(tk.W, tk.E, tk.N, tk.S))

        # Create UI elements
        self.create_ui_elements()

        # Bind events
        self.control.trace('w', self.on_control_change)

    def create_ui_elements(self):
        # Selección de Barcos
        ttk.Label(self.frame_izquierdo, text="Selección de Barcos:").pack(pady=(10, 0))

        # Frame for left and right boat selections
        boats_frame = ttk.Frame(self.frame_izquierdo)
        boats_frame.pack(fill=tk.X, pady=5)

        # Left boats
        left_frame = ttk.Frame(boats_frame)
        left_frame.pack(side=tk.LEFT, padx=10)
        ttk.Label(left_frame, text="Izquierda").pack()

        for boat_type in self.barcos_izquierda:
            ttk.Label(left_frame, text=boat_type).pack()
            ttk.Entry(left_frame, textvariable=self.barcos_izquierda[boat_type]).pack()

        # Right boats
        right_frame = ttk.Frame(boats_frame)
        right_frame.pack(side=tk.LEFT, padx=10)
        ttk.Label(right_frame, text="Derecha").pack()

        for boat_type in self.barcos_derecha:
            ttk.Label(right_frame, text=boat_type).pack()
            ttk.Entry(right_frame, textvariable=self.barcos_derecha[boat_type]).pack()

        # Algoritmo
        ttk.Label(self.frame_izquierdo, text="Algoritmo").pack(pady=(10, 0))
        algoritmo_options = ["Round Robin", "Prioridad", "Shortest Job First",
                             "First Come First Serve", "Tiempo Real"]
        self.algoritmo.set(algoritmo_options[0])
        ttk.OptionMenu(self.frame_izquierdo, self.algoritmo,
                       self.algoritmo.get(), *algoritmo_options).pack()

        # Control
        ttk.Label(self.frame_izquierdo, text="Control").pack(pady=(10, 0))
        control_options = ["Tico", "Letrero", "Equidad"]
        self.control.set(control_options[0])
        ttk.OptionMenu(self.frame_izquierdo, self.control,
                       self.control.get(), *control_options).pack()

        # Equidad frame
        self.equidad_frame = ttk.Frame(self.frame_izquierdo)
        ttk.Label(self.equidad_frame, text="Valor de Equidad").pack()
        ttk.Entry(self.equidad_frame, textvariable=self.equidad).pack()

        # Buttons
        ttk.Button(self.frame_izquierdo, text="Guardar",
                   command=self.guardar_datos).pack(pady=5)
        ttk.Button(self.frame_izquierdo, text="Iniciar",
                   command=self.start_animation).pack(pady=5)

        # Canvas setup
        self.canvas = tk.Canvas(self.frame_derecho, width=900, height=700, bg='white')
        self.canvas.pack(expand=True, fill=tk.BOTH)

        # Load ocean image
        self.ocean_image = Image.open(os.path.join("assets", "oceano.jpg"))
        self.ocean_image = self.ocean_image.resize((700, 600), Image.Resampling.LANCZOS)
        self.ocean_photo = ImageTk.PhotoImage(self.ocean_image)

        # Water channel with ocean image
        self.canvas.create_image(450, 350, image=self.ocean_photo, tags="channel")

        # Green and orange zones (longer rectangles)
        self.canvas.create_rectangle(10, 10, 440, 90, outline='green', width=2, tags="left_zone")
        self.canvas.create_rectangle(460, 10, 890, 90, outline='orange', width=2, tags="right_zone")

        # Parking zones
        self.canvas.create_rectangle(10, 610, 440, 690, outline='orange', width=2, tags="left_parking")
        self.canvas.create_rectangle(460, 610, 890, 690, outline='green', width=2, tags="right_parking")

    def on_control_change(self, *args):
        # Hide all frames first
        self.equidad_frame.pack_forget()

        # Show appropriate frame based on selection
        if self.control.get() == "Equidad":
            self.equidad_frame.pack()

    def load_boat_images(self):
        images = {}
        image_path = os.path.join("assets")
        image_files = {
            "Normales": "barco_normal.png",
            "Pesqueros": "barco_pesquero.png",
            "Patrulla": "barco_patrulla.png"
        }

        for boat_type, filename in image_files.items():
            try:
                path = os.path.join(image_path, filename)
                image = Image.open(path)
                image = image.resize((30, 30), Image.Resampling.LANCZOS)
                images[boat_type] = ImageTk.PhotoImage(image)
            except Exception as e:
                print(f"Error loading image {filename}: {e}")
                fallback = tk.PhotoImage(width=30, height=30)
                images[boat_type] = fallback

        return images

    def display_boats(self):
        # Clear previous boats
        for boat in self.displayed_boats:
            self.canvas.delete(boat)
        self.displayed_boats.clear()

        # Display left boats
        x_offset = 20
        y_offset = 50
        for boat_type, var in self.barcos_izquierda.items():
            try:
                count = int(var.get())
                for i in range(count):
                    if x_offset > 360:
                        x_offset = 20
                        y_offset += 40
                    boat = self.canvas.create_image(x_offset, y_offset,
                                                    image=self.boat_images[boat_type],
                                                    tags=f"boat_left_{boat_type}")
                    self.displayed_boats.append(boat)
                    x_offset += 40
            except ValueError:
                continue

        # Display right boats
        x_offset = 470
        y_offset = 50
        for boat_type, var in self.barcos_derecha.items():
            try:
                count = int(var.get())
                for i in range(count):
                    if x_offset > 810:
                        x_offset = 470
                        y_offset += 40
                    boat = self.canvas.create_image(x_offset, y_offset,
                                                    image=self.boat_images[boat_type],
                                                    tags=f"boat_right_{boat_type}")
                    self.displayed_boats.append(boat)
                    x_offset += 40
            except ValueError:
                continue

    def start_animation(self):
        # Check if there's at least one boat in barcos.txt
        try:
            with open('barcos.txt', 'r') as f:
                lines = f.readlines()
                if len(lines) <= 1:  # Only header or empty file
                    messagebox.showerror("Error", "Debe haber al menos un barco en barcos.txt para iniciar la simulación.")
                    return
        except FileNotFoundError:
            messagebox.showerror("Error", "El archivo barcos.txt no existe. Guarde los datos primero.")
            return

        if self.animation_running:
            return

        self.animation_running = True
        self.animate_boats()

    def animate_boats(self):
        if not self.animation_running:
            return

        all_boats = self.displayed_boats.copy()
        moved = False

        for boat in all_boats:
            tags = self.canvas.gettags(boat)
            x, y = self.canvas.coords(boat)

            if 'boat_left' in tags:
                if y < 350:
                    self.canvas.move(boat, 0, 5)
                    moved = True
                elif x < 450:
                    self.canvas.move(boat, 5, 0)
                    moved = True
                elif y < 650:
                    self.canvas.move(boat, 0, 5)
                    moved = True
                else:
                    new_x = 460 + (x - 10)
                    self.canvas.moveto(boat, new_x, 650)
                    new_tags = ('boat_right_' + tags[0].split('_')[-1],)
                    self.canvas.itemconfig(boat, tags=new_tags)
                    moved = True
            elif 'boat_right' in tags:
                if y < 350:
                    self.canvas.move(boat, 0, 5)
                    moved = True
                elif x > 450:
                    self.canvas.move(boat, -5, 0)
                    moved = True
                elif y < 650:
                    self.canvas.move(boat, 0, 5)
                    moved = True
                else:
                    new_x = 10 + (x - 460)
                    self.canvas.moveto(boat, new_x, 650)
                    new_tags = ('boat_left_' + tags[0].split('_')[-1],)
                    self.canvas.itemconfig(boat, tags=new_tags)
                    moved = True

        if moved:
            self.root.after(50, self.animate_boats)
        else:
            self.animation_running = False

    def guardar_datos(self):
        try:
            barcos_data = []

            # Process boats
            for direction, barcos in [("Izquierda", self.barcos_izquierda),
                                      ("Derecha", self.barcos_derecha)]:
                for tipo, var in barcos.items():
                    try:
                        cantidad = self.validate_numeric_input(var.get(),
                                                               f"Barcos {tipo} ({direction})")
                        if cantidad > 0:
                            barcos_data.append((direction, tipo, cantidad))
                    except ValueError as e:
                        messagebox.showerror("Error", str(e))
                        return

            if not barcos_data:
                messagebox.showwarning("Advertencia", "No hay barcos para guardar")
                return

            # Save data to barcos.txt
            with open('barcos.txt', 'w') as f:
                f.write("ID Tipo Oceano Prioridad Tiempo_SJF Tiempo_Maximo\n")
                id_counter = 1
                for direccion, tipo, cantidad in barcos_data:
                    for _ in range(cantidad):
                        prioridad = self.get_validated_user_input(
                            f"Ingrese la prioridad para el barco {tipo} {id_counter}:")
                        tiempo_sjf = self.get_validated_user_input(
                            f"Ingrese el Tiempo SJF para el barco {tipo} {id_counter}:")
                        tiempo_maximo = self.get_validated_user_input(
                            f"Ingrese el Tiempo Máximo para el barco {tipo} {id_counter}:")

                        if prioridad is None or tiempo_sjf is None or tiempo_maximo is None:
                            messagebox.showerror("Error", "Operación cancelada. No se guardaron los datos.")
                            return

                        f.write(f"{id_counter} {tipo} {direccion} {prioridad} {tiempo_sjf} {tiempo_maximo}\n")
                        id_counter += 1

            with open('config.txt', 'w') as f:
                f.write("Algoritmo Control ValoControl\n")
                valor_control = "0"
                if self.control.get() == "Equidad":
                    valor_control = self.equidad.get()

                # Convert algorithm name to initials
                algoritmo_initials = {
                    "Round Robin": "RR",
                    "Prioridad": "P",
                    "Shortest Job First": "SJF",
                    "First Come First Serve": "FCFS",
                    "Tiempo Real": "TR"
                }
                algoritmo_saved = algoritmo_initials.get(self.algoritmo.get(), self.algoritmo.get())

                f.write(f"{algoritmo_saved} {self.control.get()} {valor_control}\n")

            messagebox.showinfo("Éxito", "Datos guardados correctamente en barcos.txt y config.txt")

            # Update boat display
            self.display_boats()

        except Exception as e:
            messagebox.showerror("Error", f"Error al guardar datos: {str(e)}")

    def get_validated_user_input(self, prompt):
        while True:
            value = simpledialog.askstring("Input", prompt, parent=self.root)
            if value is None:  # User cancelled
                return None
            try:
                num = int(value)
                if num < 0:
                    raise ValueError
                return num
            except ValueError:
                messagebox.showerror("Error", "Por favor, ingrese un número entero no negativo.")

    def validate_numeric_input(self, value, field_name):
        try:
            num = int(value)
            if num < 0:
                raise ValueError(f"{field_name} no puede ser negativo")
            return num
        except ValueError:
            raise ValueError(f"{field_name} debe ser un número entero válido")


def main():
    root = tk.Tk()
    app = BoatSimulator(root)
    root.mainloop()


if __name__ == "__main__":
    main()