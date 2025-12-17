#include <Servo.h>

// ---------------- SERVOS ----------------
Servo pan;
Servo tilt;

// ---------------- ULTRASONIC ----------------
const int trigPin = 10;
const int echoPin = 11;

// ---------------- MOTORS ----------------
// RIGHT motor
const int ENA = 3;  
const int IN1 = 4;
const int IN2 = 5;
// LEFT motor
const int ENB = 6;   
const int IN3 = 7;
const int IN4 = 12;

// ---------------- SERVO MOTION ----------------
int panAngle = 90;  //start angle (Left-CENTRE-Right)
int tiltAngle = 30; //start angle (Top-CENTRE-Down)
int panDir = 3;     //this increments the angle by 3 (ex:90-93-96...)
int tiltDir = 2;    //this increments the angle by 2 (ex:30-32-34...)

// ---------------- LIMITS ----------------
const int panMin = 15;  //Left
const int panMax = 165; //Right
const int tiltMin = 0;  //Up
const int tiltMax = 60; //Down

// ---------------- SPEED ----------------
int baseSpeed = 250;         // higher torque for weight
float speedMultiplier = 1.3; // extra power without rewriting logic
int finalSpeed;

// ---- AUTO STRAIGHT TRIM ----
int rightTrim = 0;   //This cuts the extra speed on the right motor incase it turns towards right
int leftTrim  = 0;   //This cuts the extra speed on the left motor incase it turns towards left

// ---------------- DISTANCE ----------------
long duration;
int distance;
int confirmCount = 0; //This reconfirms whether the obstacle trigger is valid or not

// ---- PAN STABILITY ----
int lastPanAngle = 90; // stores previous pan angle to ignore servo jitter and prevent false obstacle detection


void setup() {
  Serial.begin(9600);
  pan.attach(8);
  tilt.attach(9);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT); pinMode(ENB, OUTPUT);

  finalSpeed = baseSpeed * speedMultiplier;
  if (finalSpeed > 255) finalSpeed = 255;//already the initialized max

  pan.write(panAngle);
  tilt.write(tiltAngle);
}

// ---------------- MOTOR CONTROL ----------------
void forward() {
// Adjust right (ENA) and left (ENB) motor speeds with trim to keep the rover moving straight, limiting values to 0–255
  analogWrite(ENA, constrain(finalSpeed + rightTrim, 0, 255));
  analogWrite(ENB, constrain(finalSpeed + leftTrim, 0, 255));

  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

void stopMotor() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}

void reverseMotor() {
  analogWrite(ENA, finalSpeed - 20);
  analogWrite(ENB, finalSpeed - 20);

  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
}

// ---- SMOOTH RIGHT TURN ----
void turnRightSmooth() {
  analogWrite(ENA, finalSpeed - 40);  // slow right
  analogWrite(ENB, finalSpeed);       // left full

  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

// ---- SMOOTH LEFT TURN ----
void turnLeftSmooth() {
  analogWrite(ENA, finalSpeed);       // right full
  analogWrite(ENB, finalSpeed - 40);  // slow left

  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, LOW);
}


// ---------------- ULTRASONIC ----------------
int getDistance() {
  digitalWrite(trigPin, LOW);               // Ensure the trigger pin is LOW before sending a pulse
  delayMicroseconds(3);                     // Short wait (3 µs) to let the sensor settle
  digitalWrite(trigPin, HIGH);              // Set trigger HIGH to start the ultrasonic pulse
  delayMicroseconds(8);                     // Keep HIGH for 8 µs (minimum trigger duration)
  digitalWrite(trigPin, LOW);               // Set trigger LOW again; sensor sends out sound wave

  duration = pulseIn(echoPin, HIGH, 20000); // Measure the time (in µs) for echo to return; 20ms timeout
  if (duration == 0) return 999;            // No echo detected → return 999 as "far away"

  int d = duration * 0.034 / 2;             // Convert round-trip time to distance in cm (speed of sound / 2)
  if (d < 2 || d > 300) return 999;         // Ignore unrealistic distances (too close or too far)
  return d;                                 // Return the valid distance in centimeters
}


// ---------------- SCAN ----------------
int scanAngle(int ang) {
  pan.write(ang);         // Move the pan servo to the desired angle 'ang'
  delay(150);             // Wait 150 ms for the servo to reach and stabilize at the position
  return getDistance();   // Measure and return the distance from the ultrasonic sensor at this angle
}


// ---------------- MAIN LOOP ----------------
void loop() {

  forward(); // Keep the rover moving forward continuously

  // -------- SERVO WAVE --------
  panAngle += panDir;   // Increment pan angle by current direction (controls horizontal sweep)
  tiltAngle += tiltDir; // Increment tilt angle by current direction (controls vertical sweep)

  // Reverse directions if limits reached
  if (panAngle >= panMax) panDir = -3;     // If pan hits right limit, sweep left faster
  if (panAngle <= panMin) panDir = 3;      // If pan hits left limit, sweep right faster
  if (tiltAngle >= tiltMax) tiltDir = -1;  // If tilt hits top limit, move down
  if (tiltAngle <= tiltMin) tiltDir = 1;   // If tilt hits bottom limit, move up

  pan.write(panAngle);   // Move pan servo to new angle
  tilt.write(tiltAngle); // Move tilt servo to new angle
  delay(10);             // Short delay to avoid overloading servo updates

  // -------- OBJECT CHECK --------
  if (panAngle > 20 && panAngle < 160) {   // Only check for obstacles when near center (ignore extreme sides)

    if (abs(panAngle - lastPanAngle) > 2) { //ignore tiny pan jitters
      lastPanAngle = panAngle;              // Update lastPanAngle to current angle

      distance = getDistance();             // Measure distance from ultrasonic sensor

      // Print current pan, tilt, and distance for debugging
      Serial.print("P:");
      Serial.print(panAngle);
      Serial.print(" T:");
      Serial.print(tiltAngle);
      Serial.print(" D:");
      Serial.println(distance);

      if (distance < 10) confirmCount++;   // Count consecutive detections if object is near
      else confirmCount = 0;                // Reset if no object

      if (confirmCount >= 2) {             // If object detected consistently twice
        confirmCount = 0;                  
        obstacleRoutine();                 // Trigger obstacle avoidance
      }
    }
  }
}

// ---------------- OBSTACLE HANDLER ----------------
void obstacleRoutine() {

  stopMotor();          // Immediately stop the rover
  delay(200);           // Short pause to ensure it fully stops

  // Reverse ~10cm
  reverseMotor();       // Move backward
  delay(600);           // Time is tuned to roughly move 10cm
  stopMotor();          // Stop after reversing
  delay(200);           // Short pause before scanning

  // Scan environment in three directions
  int center = scanAngle(90);  // Check straight ahead
  int right  = scanAngle(30);  // Check right side
  int left   = scanAngle(150); // Check left side

  // Decide best direction based on maximum free distance
  if (right > left && right > center) {
    turnRightSmooth();   // Turn smoothly right if right is best
    delay(600);          // Duration tuned for smooth turning
  }
  else if (left > right && left > center) {
    turnLeftSmooth();    // Turn smoothly left if left is best
    delay(600);          // Duration tuned for smooth turning
  }

  stopMotor();           // Stop turning
  delay(200);            // Short pause to stabilize

  // Final check: if forward path is clear, continue
  if (scanAngle(90) > 10) forward();  // Move forward if no obstacle detected
}

