#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Servo.h>

ESP8266WebServer server(80);

// ================= WIFI AP =================
const char* ssid = "ESP8266_CAR";
const char* password = "12345678";

// ================= MOTOR PINS ==============
#define IN1 D1
#define IN2 D2
#define IN3 D3
#define IN4 D4

// ================= SERVO PINS ==============
#define PAN_SERVO  D5   // LEFT ↔ RIGHT
#define TILT_SERVO D6   // UP ↕ DOWN

// ================= SERVO LIMITS ============
#define PAN_MIN   15
#define PAN_MAX   155
#define TILT_MIN  60
#define TILT_MAX  120

#define PAN_CENTER  ((PAN_MIN + PAN_MAX) / 2)   // 85
#define TILT_CENTER ((TILT_MIN + TILT_MAX) / 2) // 90

Servo panServo, tiltServo;

// ================= SMOOTH SERVO ============
int panCurrent = PAN_CENTER, panTarget = PAN_CENTER;
int tiltCurrent = TILT_CENTER, tiltTarget = TILT_CENTER;

unsigned long lastMove = 0;
const int servoDelay = 5;   // smaller = smoother

// ================= MOTOR ===================
void stopMotors() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}
void forward() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}
void reverse() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
}
void left() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
}
void right() {
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

// ================= WEB PAGE ================
String htmlPage() {
  return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body{background:#111;color:white;text-align:center;font-family:Arial;margin:0}

iframe{
  width:100%;
  max-width:100%;
  aspect-ratio: 4 / 3;   /* change to 16 / 9 if ESP32-CAM */
  height:auto;
  border-radius:12px;
  border:2px solid #1abc9c;
}

.slider{width:90%}
button{
  width:120px;height:70px;margin:8px;
  font-size:18px;border-radius:15px;
  border:none;background:#1abc9c
}
.stop{background:#e74c3c}
</style>

<script>
const PAN_CENTER = 85;
const TILT_CENTER = 90;

function send(cmd){ fetch("/" + cmd); }
function pan(v){ fetch("/pan?val=" + v); }
function tilt(v){ fetch("/tilt?val=" + v); }

function panCenter(){
  document.getElementById("pan").value = PAN_CENTER;
  pan(PAN_CENTER);
}
function tiltCenter(){
  document.getElementById("tilt").value = TILT_CENTER;
  tilt(TILT_CENTER);
}
</script>
</head>

<body>

<h2>ESP8266 FPV TANK CAR</h2>

<h3>Live Camera</h3>
<iframe src="http://192.168.4.2/stream"></iframe>

<h3>PAN (LEFT ↔ RIGHT)</h3>
<input id="pan" class="slider" type="range"
min="15" max="155" value="85"
oninput="pan(this.value)"
onmouseup="panCenter()" ontouchend="panCenter()">

<h3>TILT (UP ↕ DOWN)</h3>
<input id="tilt" class="slider" type="range"
min="60" max="120" value="90"
oninput="tilt(this.value)"
onmouseup="tiltCenter()" ontouchend="tiltCenter()">

<hr>

<button ontouchstart="send('F')" ontouchend="send('S')"
onmousedown="send('F')" onmouseup="send('S')">FORWARD</button><br>

<button ontouchstart="send('L')" ontouchend="send('S')"
onmousedown="send('L')" onmouseup="send('S')">LEFT</button>

<button ontouchstart="send('R')" ontouchend="send('S')"
onmousedown="send('R')" onmouseup="send('S')">RIGHT</button><br>

<button ontouchstart="send('B')" ontouchend="send('S')"
onmousedown="send('B')" onmouseup="send('S')">REVERSE</button><br>

<button class="stop" onclick="send('S')">STOP</button>

</body>
</html>
)rawliteral";
}

// ================= SETUP ===================
void setup() {
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);

  panServo.attach(PAN_SERVO);
  tiltServo.attach(TILT_SERVO);

  panServo.write(PAN_CENTER);
  tiltServo.write(TILT_CENTER);

  WiFi.softAP(ssid, password);

  server.on("/", [](){ server.send(200,"text/html",htmlPage()); });

  server.on("/F", [](){ forward(); server.send(200,"text/plain","F"); });
  server.on("/B", [](){ reverse(); server.send(200,"text/plain","B"); });
  server.on("/L", [](){ left(); server.send(200,"text/plain","L"); });
  server.on("/R", [](){ right(); server.send(200,"text/plain","R"); });
  server.on("/S", [](){ stopMotors(); server.send(200,"text/plain","S"); });

  server.on("/pan", [](){
    if(server.hasArg("val")){
      int v = constrain(server.arg("val").toInt(), PAN_MIN, PAN_MAX);
      panTarget = PAN_MAX - (v - PAN_MIN);
    }
    server.send(200,"text/plain","PAN");
  });

  server.on("/tilt", [](){
    if(server.hasArg("val")){
      tiltTarget = constrain(server.arg("val").toInt(), TILT_MIN, TILT_MAX);
    }
    server.send(200,"text/plain","TILT");
  });

  server.begin();
}

// ================= LOOP ====================
void loop() {
  server.handleClient();

  if(millis() - lastMove > servoDelay){
    lastMove = millis();

    if(panCurrent < panTarget) panCurrent++;
    else if(panCurrent > panTarget) panCurrent--;

    if(tiltCurrent < tiltTarget) tiltCurrent++;
    else if(tiltCurrent > tiltTarget) tiltCurrent--;

    panServo.write(panCurrent);
    tiltServo.write(tiltCurrent);
  }
}
