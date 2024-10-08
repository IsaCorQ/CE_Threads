// Pin definitions for the segment pins (a-g)
const int segmentPins[7] = {2, 3, 4, 5, 6, 7, 8}; // Pins connected to segments a-g

// Digit patterns for the numbers 0-9 (common cathode display)
const byte digitPatterns[10] = {
  B1111110, // 0
  B0110000, // 1
  B1101101, // 2
  B1111001, // 3
  B0110011, // 4
  B1011011, // 5
  B1011111, // 6
  B1110000, // 7
  B1111111, // 8
  B1111011  // 9
};

void setup() {
  // Set segment pins as outputs
  for (int i = 0; i < 7; i++) {
    pinMode(segmentPins[i], OUTPUT);
  }
  
  Serial.begin(9600); // Begin serial communication at 9600 baud rate

  // Display 0 on startup
  displayDigit(0);
}

void loop() {
  // Check if data is available on the serial port
  if (Serial.available() > 0) {
    // Read the digit from the serial buffer
    char receivedDigit = Serial.read(); // Read a single character

    if (receivedDigit >= '0' && receivedDigit <= '9') {
      // Convert char to digit (e.g., '5' -> 5)
      int value = receivedDigit - '0';
      displayDigit(value); // Display the digit on the 7-segment display
    }
  }
}

// Function to display a single digit
void displayDigit(int value) {
  // Set the segments for the corresponding digit value
  byte segments = digitPatterns[value];
  for (int i = 0; i < 7; i++) {
    digitalWrite(segmentPins[i], bitRead(segments, 6 - i));
  }
}
