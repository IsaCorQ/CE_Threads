#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

int num;

// Function to simulate returning a number (e.g., number of boats)
int num_barcos() {
    // Replace this with your logic to return the desired number
    return 5; // Example number
}

int main() {
    // Open the serial port (adjust path if needed)
    int serial_port = open("/dev/ttyACM0", O_RDWR);
    if (serial_port < 0) {
        perror("Error opening serial port");
        return 1;
    }

    // Configure the serial port
    struct termios tty;
    tcgetattr(serial_port, &tty);

    // Set baud rate
    cfsetispeed(&tty, B9600);
    cfsetospeed(&tty, B9600);

    // Apply settings
    tcsetattr(serial_port, TCSANOW, &tty);

    num = num_barcos();

   
    char input[2]; // Buffer to hold 1 digit and a null terminator
    snprintf(input, sizeof(input), "%d", num); // Format the number as a string

    // Send the number to Arduino
    write(serial_port, input, 1); // Send only 1 digit

    // Close the serial port
    close(serial_port);
    return 0;
}
