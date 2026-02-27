#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Servo.h>

ESP8266WebServer server(80);

// ================= WIFI =================
const char* ssid = "ESP8266_CAR";
const char* password = "12345678";

// ================= MOTOR PINS =================
#define IN1 D1
#define IN2 D2
#define IN3 D3
#define IN4 D4
#define ENA D7
#define ENB D6

// ================= SERVO =================
#define PAN_SERVO   D5
#define PAN_MIN     15
#define PAN_MAX     155
#define PAN_CENTER  85

Servo panServo;
int currentAngle = PAN_CENTER;

// ================= SPEED =================
#define SPEED_MIN 70
#define SPEED_MAX 255

int manualSpeed = 150;
int autoSpeed   = 150;
int speedValue  = 150;

// ================= MOTOR =================
void applySpeed(){
  analogWrite(ENA, speedValue);
  analogWrite(ENB, speedValue);
}

void stopMotors(){
  digitalWrite(IN1,LOW); digitalWrite(IN2,LOW);
  digitalWrite(IN3,LOW); digitalWrite(IN4,LOW);
}

void forward(){
  stopMotors(); delay(40);
  digitalWrite(IN1,HIGH); digitalWrite(IN2,LOW);
  digitalWrite(IN3,HIGH); digitalWrite(IN4,LOW);
}

void reverse(){
  stopMotors(); delay(40);
  digitalWrite(IN1,LOW); digitalWrite(IN2,HIGH);
  digitalWrite(IN3,LOW); digitalWrite(IN4,HIGH);
}

void left(){
  stopMotors(); delay(40);
  digitalWrite(IN1,HIGH); digitalWrite(IN2,LOW);
  digitalWrite(IN3,LOW); digitalWrite(IN4,HIGH);
}

void right(){
  stopMotors(); delay(40);
  digitalWrite(IN1,LOW); digitalWrite(IN2,HIGH);
  digitalWrite(IN3,HIGH); digitalWrite(IN4,LOW);
}

// ================= HTML =================
String htmlPage(){
return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body{background:#111;color:white;font-family:Arial;text-align:center;margin:0}
.modeBar{display:flex;background:#000;padding:10px}
.modeBtn{flex:1;margin:5px;height:50px;font-size:18px;border:none;border-radius:10px;background:#333;color:white}
.modeBtn.active{background:#1abc9c}
.hidden{display:none}
iframe{
  width:95%;
  max-width:400px;
  aspect-ratio:4/3;
  border:2px solid #1abc9c;
  border-radius:10px;
  margin-top:10px;
}
.grid{
  display:grid;
  grid-template-columns:repeat(5,60px);
  gap:6px;
  justify-content:center;
  margin-top:20px;
}
.cell{width:60px;height:60px;background:#333;border-radius:8px}
.cell.origin{background:#e67e22}
.cell.sel{background:#1abc9c}
button{margin:6px;padding:10px 18px;font-size:16px;border:none;border-radius:10px}
.start{background:#1abc9c}
.stop{background:#e74c3c;color:white}
.reset{background:#3498db;color:white}
input[type=range]{width:90%}
.slider-label{font-size:14px;color:#aaa;margin-top:10px;}
</style>

<script>
let gridSize=5;
let running=false;
let destination=null;
let pos={r:3,c:3};
let dir=0;
const TURN_TIME=400;
const MOVE_TIME=900;
function send(cmd){ fetch("/"+cmd); }
function setSpeed(val){ fetch("/speed?val="+val); }
function moveServo(val){ fetch("/servo?angle="+val); }
function showMode(mode){
["auto","manual"].forEach(m=>{
document.getElementById(m).classList.add("hidden");
document.getElementById("btn"+m).classList.remove("active");
});
document.getElementById(mode).classList.remove("hidden");
document.getElementById("btn"+mode).classList.add("active");
}
function clickCell(r,c){
if(running)return;
destination={r:r,c:c};
document.querySelectorAll(".cell").forEach(c=>c.classList.remove("sel"));
document.getElementById("c"+r+c).classList.add("sel");
}
function key(r,c){return r+","+c;}
function dijkstra(start,end){
let dist={},prev={},pq=[];
for(let r=1;r<=gridSize;r++){
for(let c=1;c<=gridSize;c++){
dist[key(r,c)]=Infinity;
prev[key(r,c)]=null;
}}
dist[key(start.r,start.c)]=0;
pq.push({r:start.r,c:start.c,d:0});
while(pq.length>0){
pq.sort((a,b)=>a.d-b.d);
let cur=pq.shift();
if(cur.r===end.r&&cur.c===end.c)break;
let nbs=[
{r:cur.r-1,c:cur.c},
{r:cur.r+1,c:cur.c},
{r:cur.r,c:cur.c-1},
{r:cur.r,c:cur.c+1}
];
for(let n of nbs){
if(n.r<1||n.r>gridSize||n.c<1||n.c>gridSize)continue;
let alt=dist[key(cur.r,cur.c)]+1;
if(alt<dist[key(n.r,n.c)]){
dist[key(n.r,n.c)]=alt;
prev[key(n.r,n.c)]=cur;
pq.push({r:n.r,c:n.c,d:alt});
}}}
let path=[];
let cur=end;
while(cur){path.unshift(cur);cur=prev[key(cur.r,cur.c)];}
return path;
}
async function startAuto(){
if(!destination)return;
running=true;
let path=dijkstra(pos,destination);
for(let i=1;i<path.length&&running;i++){
let target=path[i];
let dr=target.r-pos.r;
let dc=target.c-pos.c;
let move=null;
if(dir===0){
if(dr===-1&&dc===0)move="S";
else if(dr===0&&dc===-1)move="L";
else if(dr===0&&dc===1)move="R";
else if(dr===1&&dc===0)move="B";
}
else if(dir===1){
if(dr===0&&dc===1)move="S";
else if(dr===-1&&dc===0)move="L";
else if(dr===1&&dc===0)move="R";
else if(dr===0&&dc===-1)move="B";
}
else if(dir===2){
if(dr===1&&dc===0)move="S";
else if(dr===0&&dc===1)move="L";
else if(dr===0&&dc===-1)move="R";
else if(dr===-1&&dc===0)move="B";
}
else if(dir===3){
if(dr===0&&dc===-1)move="S";
else if(dr===1&&dc===0)move="L";
else if(dr===-1&&dc===0)move="R";
else if(dr===0&&dc===1)move="B";
}
if(move==="L"){send("L");await sleep(TURN_TIME);send("S");dir=(dir+3)%4;}
else if(move==="R"){send("R");await sleep(TURN_TIME);send("S");dir=(dir+1)%4;}
else if(move==="B"){
send("R");await sleep(TURN_TIME);send("S");
await sleep(100);
send("R");await sleep(TURN_TIME);send("S");
dir=(dir+2)%4;
}
send("F");
await sleep(MOVE_TIME);
send("S");
pos=target;
}
running=false;
}
function stopAuto(){running=false;send("S");}
function resetAll(){
stopAuto();
pos={r:3,c:3};
dir=0;
destination=null;
document.querySelectorAll(".cell").forEach(c=>{
c.classList.remove("sel");
c.classList.remove("origin");
});
document.getElementById("c33").classList.add("origin");
}
function sleep(ms){return new Promise(r=>setTimeout(r,ms));}
</script>
</head>
<body onload="showMode('manual')">
<div class="modeBar">
<button id="btnauto" class="modeBtn" onclick="showMode('auto')">AUTO</button>
<button id="btnmanual" class="modeBtn" onclick="showMode('manual')">MANUAL</button>
</div>

<!-- ===== AUTO MODE ===== -->
<div id="auto" class="hidden">
<iframe src="http://192.168.4.2/stream"></iframe>
<br>
<div class="slider-label">&#128247; Pan Camera:</div>
<input type="range" min="15" max="155" value="85"
  oninput="moveServo(this.value)"
  onchange="moveServo(85); this.value=85"><br>
<div class="grid">
<script>
for(let r=1;r<=5;r++){
for(let c=1;c<=5;c++){
let cls=(r==3&&c==3)?"cell origin":"cell";
document.write(`<div id="c${r}${c}" class="${cls}" onclick="clickCell(${r},${c})"></div>`);
}}
</script>
</div>
<br>
<button class="start" onclick="startAuto()">START</button>
<button class="stop" onclick="stopAuto()">STOP</button>
<button class="reset" onclick="resetAll()">RESET</button>
</div>

<!-- ===== MANUAL MODE ===== -->
<div id="manual" class="hidden">
<iframe src="http://192.168.4.2/stream"></iframe>
<br>
<div class="slider-label">&#128247; Pan Camera:</div>
<input type="range" min="15" max="155" value="85"
  oninput="moveServo(this.value)"
  onchange="moveServo(85); this.value=85"><br>
<div class="slider-label">&#9889; Speed:</div>
<input type="range" min="70" max="255" value="150"
  oninput="setSpeed(this.value)"><br>
<button ontouchstart="send('F')" ontouchend="send('S')">FORWARD</button><br>
<button ontouchstart="send('L')" ontouchend="send('S')">LEFT</button>
<button ontouchstart="send('R')" ontouchend="send('S')">RIGHT</button><br>
<button ontouchstart="send('B')" ontouchend="send('S')">REVERSE</button><br>
<button onclick="send('S')">STOP</button>
</div>
</body>
</html>
)rawliteral";
}

// ================= SETUP =================
void setup(){
  analogWriteRange(255);
  analogWriteFreq(1000);

  pinMode(IN1,OUTPUT); pinMode(IN2,OUTPUT);
  pinMode(IN3,OUTPUT); pinMode(IN4,OUTPUT);
  pinMode(ENA,OUTPUT); pinMode(ENB,OUTPUT);

  // Servo init
  panServo.attach(PAN_SERVO);
  panServo.write(PAN_CENTER);

  WiFi.softAP(ssid,password);

  server.on("/", [](){server.send(200,"text/html",htmlPage());});

  server.on("/F", [](){ speedValue=autoSpeed; applySpeed(); forward(); server.send(200,"ok");});
  server.on("/B", [](){ speedValue=autoSpeed; applySpeed(); reverse(); server.send(200,"ok");});
  server.on("/L", [](){ speedValue=autoSpeed; applySpeed(); left(); server.send(200,"ok");});
  server.on("/R", [](){ speedValue=autoSpeed; applySpeed(); right(); server.send(200,"ok");});
  server.on("/S", [](){ stopMotors(); server.send(200,"ok");});

  server.on("/speed", [](){
    manualSpeed=constrain(server.arg("val").toInt(),SPEED_MIN,SPEED_MAX);
    speedValue=manualSpeed;
    applySpeed();
    server.send(200,"ok");
  });

  // Servo pan endpoint
  server.on("/servo", [](){
    int angle = constrain(server.arg("angle").toInt(), PAN_MIN, PAN_MAX);
    currentAngle = angle;
    panServo.write(angle);
    server.send(200,"ok");
  });

  server.begin();
}

void loop(){
  server.handleClient();
}
