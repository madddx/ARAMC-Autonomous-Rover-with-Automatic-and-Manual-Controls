// --- Motor Driver Pins ---
const int ENA = 3;   // Right Motor PWM
const int IN1 = 4;   // Right Motor A
const int IN2 = 5;   // Right Motor B

const int ENB = 6;   // Left Motor PWM
const int IN3 = 7;   // Left Motor A
const int IN4 = 12;  // Left Motor B

void setup() {
  Serial.begin(9600);

  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  analogWrite(ENA, 200);  // Set speed
  analogWrite(ENB, 200);

  Serial.println("Send L, R or B");
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();

    if (cmd == 'L') {
      Serial.println("Testing LEFT motor...");
      testLeftMotor();
    }

    if (cmd == 'R') {
      Serial.println("Testing RIGHT motor...");
      testRightMotor();
    }

    if (cmd == 'B') {
      Serial.println("Testing BOTH motors...");
      testBothMotors();
    }
  }
}

// ------------ MOTOR TEST FUNCTIONS ------------

void testRightMotor() {
  // Forward
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  delay(2000);

  // Reverse
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  delay(2000);

  // Stop
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
}

void testLeftMotor() {
  // Forward
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  delay(2000);

  // Reverse
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  delay(2000);

  // Stop
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void testBothMotors() {
  // Forward
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  delay(2000);

  // Reverse
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  delay(2000);

  // Stop
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}
