#include <Servo.h>

Servo myServo1;
Servo myServo2;

void setup() {
  myServo1.attach(9);   // Connect servo signal wire to pin 9
  myServo2.attach(8);   // Connect servo signal wire to pin 8
  myServo1.write(0);   // Center the servo at 90 degrees
  myServo2.write(90);
}

void loop() {
  // Nothing needed here for centering
}
