#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Servo.h>

ESP8266WebServer server(80);

// ================= WIFI AP DETAILS =================
const char* ssid = "ESP8266_CAR";
const char* password = "12345678";

// ================= MOTOR PINS ======================
#define IN1 D1
#define IN2 D2
#define IN3 D3
#define IN4 D4

// ================= SERVO PINS ======================
#define PAN_SERVO  D5
#define TILT_SERVO D6

Servo panServo;
Servo tiltServo;

// ================= MOTOR FUNCTIONS =================
void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void forward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void reverse() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void left() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void right() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

// ================= WEB PAGE ========================
String htmlPage() {
  return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body {
  text-align:center;
  font-family:Arial;
  background:#111;
  color:white;
  margin:0;
}
iframe {
  width:95%;
  height:240px;
  border-radius:12px;
  border:2px solid #1abc9c;
}
button {
  width:120px;
  height:70px;
  font-size:18px;
  margin:10px;
  border-radius:15px;
  border:none;
  background:#1abc9c;
}
button:active { background:#16a085; }
.stop { background:#e74c3c; color:white; }

.slider {
  width:90%;
}
</style>

<script>
function send(cmd){ fetch("/" + cmd); }
function pan(val){ fetch("/pan?val=" + val); }
function tilt(val){ fetch("/tilt?val=" + val); }
</script>
</head>

<body>

<h2>ESP8266 FPV TANK CAR</h2>

<h3>Live Camera</h3>
<iframe src="http://192.168.4.2/stream"></iframe>

<h3>Camera Control</h3>
PAN<br>
<input class="slider" type="range" min="0" max="180" value="90" oninput="pan(this.value)">
<br>
TILT<br>
<input class="slider" type="range" min="30" max="150" value="90" oninput="tilt(this.value)">

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

// ================= SETUP ===========================
void setup() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  stopMotors();

  panServo.attach(PAN_SERVO);
  tiltServo.attach(TILT_SERVO);
  panServo.write(90);
  tiltServo.write(90);

  WiFi.softAP(ssid, password);

  server.on("/", []() {
    server.send(200, "text/html", htmlPage());
  });

  server.on("/F", [](){ forward(); server.send(200,"text/plain","F"); });
  server.on("/B", [](){ reverse(); server.send(200,"text/plain","B"); });
  server.on("/L", [](){ left();    server.send(200,"text/plain","L"); });
  server.on("/R", [](){ right();   server.send(200,"text/plain","R"); });
  server.on("/S", [](){ stopMotors(); server.send(200,"text/plain","S"); });

  server.on("/pan", [](){
    if(server.hasArg("val")){
      panServo.write(server.arg("val").toInt());
    }
    server.send(200,"text/plain","PAN");
  });

  server.on("/tilt", [](){
    if(server.hasArg("val")){
      tiltServo.write(server.arg("val").toInt());
    }
    server.send(200,"text/plain","TILT");
  });

  server.begin();
}

// ================= LOOP ============================
void loop() {
  server.handleClient();
}
