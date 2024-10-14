#define CRZ 51
#define RPR 3
#define GPR 4
#define BPR 5

#define LGC 48
#define LRC 49

#define LY 45
#define MY 44
#define RY 43

#define RRC 38
#define RGC 39

const int segmentPins[7] = {22, 23, 24, 25, 26, 27, 28};

bool signalProcessed = false;
unsigned long lastSignalTime = 0; 

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

  pinMode(CRZ, OUTPUT);

  pinMode(RPR, OUTPUT);
  pinMode(GPR, OUTPUT);
  pinMode(BPR, OUTPUT);

  pinMode(LGC, OUTPUT);
  pinMode(LRC, OUTPUT);

  pinMode(RGC, OUTPUT);
  pinMode(RRC, OUTPUT);

  pinMode(LY, OUTPUT);
  pinMode(MY, OUTPUT);
  pinMode(RY, OUTPUT);
  
  Serial.begin(9600);
  displayDigit(0);
}

void loop() {
    // Check for serial input
    if (Serial.available() > 0 && !signalProcessed) {
        char receivedDigit = Serial.read();
        Serial.print(receivedDigit);
        if (receivedDigit == 'i') {
            // Left-side movement: green left, red right, move boat through the canal
            digitalWrite(LGC, HIGH);
            digitalWrite(LRC, LOW);
            digitalWrite(RGC, LOW);
            digitalWrite(RRC, HIGH);
            cruzarCL();
            signalProcessed = true;
            lastSignalTime = millis();
        } else if (receivedDigit == 'd') {
            // Right-side movement: red left, green right, move boat through the canal
            digitalWrite(LGC, LOW);
            digitalWrite(LRC, HIGH);
            digitalWrite(RGC, HIGH);
            digitalWrite(RRC, LOW);
            cruzarCR();
            signalProcessed = true;
            lastSignalTime = millis();
        } else if (receivedDigit == 'r') {
          resetLEDs();
          signalProcessed = true;
          lastSignalTime = millis();
        } else if (receivedDigit >= '0' && receivedDigit <= '9') {
              int value = receivedDigit - '0';
              displayDigit(value);
        } else if (receivedDigit == 'o') {
          digitalWrite(RPR, LOW);
          digitalWrite(GPR, HIGH);
          digitalWrite(BPR, LOW); // Green for small boats
        } else if (receivedDigit == 'e') {
          digitalWrite(RPR, LOW);
          digitalWrite(GPR, LOW);
          digitalWrite(BPR, HIGH); // Blue for large boats
        } else if (receivedDigit == 'a') {
          digitalWrite(RPR, HIGH);
          digitalWrite(GPR, LOW);
          digitalWrite(BPR, LOW); // Red for priority boats
        } else if (receivedDigit == 's') {
          digitalWrite(CRZ, HIGH);
          delay(500);
          digitalWrite(CRZ, LOW);
        }
    }
  if (millis() - lastSignalTime > 1000) { // Example 1 second delay
      signalProcessed = false;
    }
}

void cruzarCL(){
  digitalWrite(LY, HIGH);
  delay(500);
  digitalWrite(LY, LOW);
  delay(500);
  digitalWrite(MY, HIGH);
  delay(500);
  digitalWrite(MY, LOW);
  delay(500);
  digitalWrite(RY, HIGH);
  delay(500);
  digitalWrite(RY, LOW);
}

void cruzarCR(){
  digitalWrite(RY, HIGH);
  delay(500);
  digitalWrite(RY, LOW);
  delay(500);
  digitalWrite(MY, HIGH);
  delay(500);
  digitalWrite(MY, LOW);
  delay(500);
  digitalWrite(LY, HIGH);
  delay(500);
  digitalWrite(LY, LOW);
}

void displayDigit(int value) {
  // Set the segments for the corresponding digit value
  byte segments = digitPatterns[value];
  for (int i = 0; i < 7; i++) {
    digitalWrite(segmentPins[i], bitRead(segments, 6 - i));
  }
}

void resetLEDs() {
  digitalWrite(RPR, LOW);
  digitalWrite(GPR, LOW);
  digitalWrite(BPR, LOW);

  digitalWrite(LGC, LOW);
  digitalWrite(LRC, LOW);

  digitalWrite(RGC, LOW);
  digitalWrite(RRC, LOW);

  digitalWrite(LY, LOW);
  digitalWrite(MY, LOW);
  digitalWrite(RY, LOW);

  digitalWrite(CRZ, LOW);

  for (int i = 0; i < 7; i++) {
    digitalWrite(segmentPins[i], LOW);
  }
}