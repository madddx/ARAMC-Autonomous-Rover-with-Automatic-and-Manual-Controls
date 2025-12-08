#include <Servo.h>

Servo pan;     // D8
Servo tilt;    // D9

const int trigPin = 10;
const int echoPin = 11;

// Pan positions for back-and-forth sweep
int panPos[] = {0, 30, 60, 90, 120, 150, 180, 150, 120, 90, 60, 30, 0};
int panCount = 13; // number of positions

long duration;
int distance;

void setup() {
  Serial.begin(9600);
  
  pan.attach(8);
  tilt.attach(9);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
}

void loop() {
  // Sweep through all pan positions
  for (int i = 0; i < panCount; i++) {
    pan.write(panPos[i]);       // Move pan to next position
    delay(20);                  // small delay to settle servo

    tiltScan(panPos[i]);        // Tilt scan up/down at this pan position
  }
}

// ---- TILT SCAN (faster) ---- //
void tiltScan(int pAngle) {

  // Up (0° → 45°)
  for (int t = 0; t <= 45; t++) {
    tilt.write(t);
    sendUltrasonic(pAngle, t);
    delay(20);   // control tilt speed
  }

  // Down (45° → 0°)
  for (int t = 45; t >= 0; t--) {
    tilt.write(t);
    sendUltrasonic(pAngle, t);
    delay(20);
  }
}

// ---- SEND ULTRASONIC DATA ---- //
void sendUltrasonic(int p, int t) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(5);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH, 20000);
  distance = duration * 0.034 / 2;

  Serial.print("P:");
  Serial.print(p);
  Serial.print(" T:");
  Serial.print(t);
  Serial.print(" D:");
  Serial.println(distance);
}
