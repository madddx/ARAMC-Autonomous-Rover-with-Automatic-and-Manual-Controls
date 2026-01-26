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
#define ENA D7
#define ENB D8

// ================= SERVO ===================
#define PAN_SERVO D5

#define PAN_MIN     15
#define PAN_MAX     155
#define PAN_CENTER  85

Servo panServo;

// ================= SERVO SMOOTH ============
int panCurrent = PAN_CENTER;
int panTarget  = PAN_CENTER;
unsigned long lastMove = 0;
const int servoDelay = 5;

// ================= SPEED ===================
#define SPEED_MIN 70
#define SPEED_MAX 255
int speedValue = 150;

// ================= MOTOR ===================
void applySpeed() {
  analogWrite(ENA, speedValue);
  analogWrite(ENB, speedValue);
}

void stopMotors() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}

void forward() {
  applySpeed();
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

void reverse() {
  applySpeed();
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
}

void left() {
  applySpeed();
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
}

void right() {
  applySpeed();
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
body{
  background:#111;color:white;text-align:center;
  font-family:Arial;margin:0
}

.modeBar{
  display:flex;
  justify-content:space-around;
  padding:10px;
  background:#000;
}

.modeBtn{
  flex:1;
  margin:5px;
  height:50px;
  font-size:18px;
  border-radius:12px;
  border:none;
  background:#34495e;
  color:white;
}

.modeBtn.active{
  background:#1abc9c;
}

iframe{
  width:100%;
  aspect-ratio:4/3;
  border-radius:12px;
  border:2px solid #1abc9c
}

.slider{width:90%}

button{
  width:120px;height:70px;margin:8px;
  font-size:18px;border-radius:15px;
  border:none;background:#1abc9c
}

.stop{background:#e74c3c}
.hidden{display:none}
</style>

<script>
const PAN_CENTER = 85;

function send(cmd){ fetch("/" + cmd); }
function pan(v){ fetch("/pan?val=" + v); }
function speed(v){ fetch("/speed?val=" + v); }

function panCenter(){
  document.getElementById("pan").value = PAN_CENTER;
  pan(PAN_CENTER);
}

function showMode(mode){
  document.getElementById("manual").classList.add("hidden");
  document.getElementById("auto").classList.add("hidden");
  document.getElementById("park").classList.add("hidden");

  document.getElementById("btnAuto").classList.remove("active");
  document.getElementById("btnManual").classList.remove("active");
  document.getElementById("btnPark").classList.remove("active");

  document.getElementById(mode).classList.remove("hidden");
  document.getElementById("btn"+mode.charAt(0).toUpperCase()+mode.slice(1)).classList.add("active");
}
</script>
</head>

<body onload="showMode('manual')">

<!-- MODE BUTTONS -->
<div class="modeBar">
  <button id="btnAuto" class="modeBtn" onclick="showMode('auto')">AUTO</button>
  <button id="btnManual" class="modeBtn" onclick="showMode('manual')">MANUAL</button>
  <button id="btnPark" class="modeBtn" onclick="showMode('park')">PARK</button>
</div>

<!-- AUTO PLACEHOLDER -->
<div id="auto" class="hidden">
  <h2>AUTO MODE</h2>
</div>

<!-- PARK PLACEHOLDER -->
<div id="park" class="hidden">
  <h2>PARK MODE</h2>
</div>

<!-- MANUAL MODE -->
<div id="manual">

<h2>ESP8266 FPV TANK CAR</h2>

<iframe src="http://192.168.4.2/stream"></iframe>

<h3>PAN CAMERA</h3>
<input id="pan" class="slider" type="range"
min="15" max="155" value="85"
oninput="pan(this.value)"
onmouseup="panCenter()" ontouchend="panCenter()">

<h3>SPEED</h3>
<input class="slider" type="range"
min="70" max="255" value="150"
oninput="speed(this.value)">

<hr>

<button ontouchstart="send('F')" ontouchend="send('S')">FORWARD</button><br>
<button ontouchstart="send('L')" ontouchend="send('S')">LEFT</button>
<button ontouchstart="send('R')" ontouchend="send('S')">RIGHT</button><br>
<button ontouchstart="send('B')" ontouchend="send('S')">REVERSE</button><br>
<button class="stop" onclick="send('S')">STOP</button>

</div>

</body>
</html>
)rawliteral";
}

// ================= SETUP ===================
void setup() {
  analogWriteRange(255);
  analogWriteFreq(1000);

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT); pinMode(ENB, OUTPUT);

  panServo.attach(PAN_SERVO);
  panServo.write(PAN_CENTER);

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

  server.on("/speed", [](){
    if(server.hasArg("val")){
      speedValue = constrain(server.arg("val").toInt(), SPEED_MIN, SPEED_MAX);
      applySpeed();
    }
    server.send(200,"text/plain","SPD");
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
    panServo.write(panCurrent);
  }
}
