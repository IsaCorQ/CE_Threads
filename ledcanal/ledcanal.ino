
int leftGreenLed = 52;
int leftRedLed = 53;
int rightGreenLed = 42;
int rightRedLed = 43;

int yellowLeds[] = {46, 47, 48, 49};
int startWhiteLed = 51;
int endWhiteLed = 45;

bool signalProcessed = false;
unsigned long lastSignalTime = 0; 

void setup() {
    // Initialize LED pins
    pinMode(leftGreenLed, OUTPUT);
    pinMode(leftRedLed, OUTPUT);
    pinMode(rightGreenLed, OUTPUT);
    pinMode(rightRedLed, OUTPUT);

    for (int i = 0; i < 4; i++) {
        pinMode(yellowLeds[i], OUTPUT);
    }

    pinMode(startWhiteLed, OUTPUT);
    pinMode(endWhiteLed, OUTPUT);

    // Initialize Serial communication
    Serial.begin(9600);
    resetLEDs();
}

void loop() {
    // Check for serial input
    if (Serial.available() > 0 && !signalProcessed) {
        char receivedDigit = Serial.read();
        Serial.print(receivedDigit);
        if (receivedDigit == 'i') {
            // Left-side movement: green left, red right, move boat through the canal
            digitalWrite(leftGreenLed, HIGH);
            digitalWrite(leftRedLed, LOW);
            digitalWrite(rightGreenLed, LOW);
            digitalWrite(rightRedLed, HIGH);
            simulateBoatThroughCanalLeft();
            signalProcessed = true;
            lastSignalTime = millis();
        } else if (receivedDigit == 'd') {
            // Right-side movement: red left, green right, move boat through the canal
            digitalWrite(leftGreenLed, LOW);
            digitalWrite(leftRedLed, HIGH);
            digitalWrite(rightGreenLed, HIGH);
            digitalWrite(rightRedLed, LOW);
            simulateBoatThroughCanalRight();
            signalProcessed = true;
            lastSignalTime = millis();
        } else if (receivedDigit == 'r') {
          resetLEDs();
          signalProcessed = true;
          lastSignalTime = millis();
        } 
    }
  if (millis() - lastSignalTime > 1000) { // Example 1 second delay
      signalProcessed = false;
    }
}

void simulateBoatThroughCanalLeft() {
    // Turn on white LED to indicate the start
    digitalWrite(startWhiteLed, HIGH);
    delay(250);
    digitalWrite(startWhiteLed, LOW);
    
    // Sequentially turn on yellow LEDs to simulate movement through canal
    for (int i = 0; i < 4; i++) {
        digitalWrite(yellowLeds[i], HIGH);
        delay(250); // Simulate boat movement
        digitalWrite(yellowLeds[i], LOW);
    }
    
    // Turn on white LED to indicate the end
    digitalWrite(endWhiteLed, HIGH);
    delay(250);
    digitalWrite(endWhiteLed, LOW);
    delay(250);
    resetLEDs();
}

void simulateBoatThroughCanalRight() {
    // Turn on white LED to indicate the start
    digitalWrite(endWhiteLed, HIGH);
    delay(250);
    digitalWrite(endWhiteLed, LOW);
    
    // Sequentially turn on yellow LEDs to simulate movement through canal
    for (int i = 3; i > -1; i--) {
        digitalWrite(yellowLeds[i], HIGH);
        delay(250); // Simulate boat movement
        digitalWrite(yellowLeds[i], LOW);
    }
    
    // Turn on white LED to indicate the end
    digitalWrite(startWhiteLed, HIGH);
    delay(250);
    digitalWrite(startWhiteLed, LOW);
    delay(250);
    resetLEDs();
}

void resetLEDs() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(yellowLeds[i], LOW);
  }
  digitalWrite(leftRedLed, LOW);
  digitalWrite(leftGreenLed, LOW);
  digitalWrite(rightRedLed, LOW);
  digitalWrite(rightGreenLed, LOW);
  digitalWrite(startWhiteLed, LOW);
  digitalWrite(endWhiteLed, LOW);
}
