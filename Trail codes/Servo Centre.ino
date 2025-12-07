#include <Servo.h>

Servo myServo;

void setup() {
  myServo.attach(9);   // Connect servo signal wire to pin 9
  myServo.attach(8);   // Connect servo signal wire to pin 8
  myServo.write(90);   // Center the servo at 90 degrees
}

void loop() {
  // Nothing needed here for centering
}
