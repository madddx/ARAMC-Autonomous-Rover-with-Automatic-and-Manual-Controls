#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Servo.h>

ESP8266WebServer server(80);

// ================= WIFI =================
const char* ssid     = "ESP8266_CAR";
const char* password = "12345678";

// ================= MOTOR PINS =================
#define IN1 D1
#define IN2 D2
#define IN3 D3
#define IN4 D4
#define ENA D7
#define ENB D6

// ================= SERVO =================
#define PAN_SERVO  D5
#define PAN_MIN    15
#define PAN_MAX    155
#define PAN_CENTER 85
Servo panServo;

// ================= ULTRASONIC =================
#define TRIG_PIN D8
#define ECHO_PIN D0          // also GPIO16; INPUT only, no INPUT_PULLUP

#define OBSTACLE_DIST_CM  20   // auto-brake threshold
#define SAFE_DIST_CM      25   // must exceed this to un-brake

// ================= SPEED =================
#define SPEED_MIN 70
#define SPEED_MAX 255
int speedValue = 150;

// ================= STATE =================
bool obstacleDetected = false;

// ─── distance measurement ──────────────────────────────────────────
long readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  // timeout = 30 ms → ~510 cm; returns 0 if nothing heard
  long dur = pulseIn(ECHO_PIN, HIGH, 30000UL);
  if (dur == 0) return 999;           // no echo → open space
  return dur / 58L;
}

// ─── motors ────────────────────────────────────────────────────────
void applySpeed() {
  analogWrite(ENA, speedValue);
  analogWrite(ENB, speedValue);
}

void stopMotors() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}

void forward() {
  if (obstacleDetected) { stopMotors(); return; }  // ← auto-brake
  stopMotors(); delay(40);
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

void reverse() {
  stopMotors(); delay(40);
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
}

void turnLeft() {
  stopMotors(); delay(40);
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
}

void turnRight() {
  stopMotors(); delay(40);
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

// ─── HTML page ─────────────────────────────────────────────────────
String htmlPage() {
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
iframe{width:95%;max-width:400px;aspect-ratio:4/3;border:2px solid #1abc9c;border-radius:10px;margin-top:10px}
.grid{display:grid;grid-template-columns:repeat(5,60px);gap:6px;justify-content:center;margin-top:20px}
.cell{width:60px;height:60px;background:#333;border-radius:8px}
.cell.origin{background:#e67e22}
.cell.sel{background:#1abc9c}
.cell.visited{background:#2c3e50}
button{margin:6px;padding:10px 18px;font-size:16px;border:none;border-radius:10px}
.start{background:#1abc9c}
.stop{background:#e74c3c;color:white}
.reset{background:#3498db;color:white}
input[type=range]{width:90%}
.slider-label{font-size:14px;color:#aaa;margin-top:10px}
#obstacleAlert{
  display:none;background:#e74c3c;color:white;
  padding:8px;margin:8px auto;max-width:340px;
  border-radius:8px;font-weight:bold;font-size:15px
}
#obstacleAlert.show{display:block}
</style>

<script>
// ── state ──────────────────────────────────────────────────────────
const GRID   = 5;
const T_TURN = 400;   // ms for a 90° turn
const T_MOVE = 900;   // ms per grid cell

let running     = false;
let destination = null;
let pos         = {r:3, c:3};
let dir         = 0;         // 0=North 1=East 2=South 3=West
let pathStack   = [];        // remaining steps when obstacle hit
let obstacleMode= false;

// ── helpers ────────────────────────────────────────────────────────
const send = cmd => fetch('/'+cmd);
const setSpeed  = v  => fetch('/speed?val='+v);
const moveServo = v  => fetch('/servo?angle='+v);
const sleep     = ms => new Promise(r => setTimeout(r, ms));

function showAlert(on) {
  document.getElementById('obstacleAlert').classList.toggle('show', on);
}

// ── mode switch ────────────────────────────────────────────────────
function showMode(mode) {
  ['auto','manual'].forEach(m => {
    document.getElementById(m).classList.add('hidden');
    document.getElementById('btn'+m).classList.remove('active');
  });
  document.getElementById(mode).classList.remove('hidden');
  document.getElementById('btn'+mode).classList.add('active');
}

// ── grid ───────────────────────────────────────────────────────────
function clickCell(r, c) {
  if (running) return;
  destination = {r, c};
  document.querySelectorAll('.cell').forEach(el => el.classList.remove('sel'));
  document.getElementById('c'+r+c).classList.add('sel');
}

function markVisited(r, c) {
  const el = document.getElementById('c'+r+c);
  if (el && !(r===3&&c===3)) el.classList.add('visited');
}

// ── Dijkstra ───────────────────────────────────────────────────────
function dijkstra(start, end) {
  const key = (r,c) => r+','+c;
  let dist={}, prev={}, pq=[];
  for (let r=1;r<=GRID;r++) for (let c=1;c<=GRID;c++) {
    dist[key(r,c)] = Infinity; prev[key(r,c)] = null;
  }
  dist[key(start.r, start.c)] = 0;
  pq.push({r:start.r, c:start.c, d:0});
  while (pq.length) {
    pq.sort((a,b)=>a.d-b.d);
    let cur = pq.shift();
    if (cur.r===end.r && cur.c===end.c) break;
    for (let [dr,dc] of [[-1,0],[1,0],[0,-1],[0,1]]) {
      let nr=cur.r+dr, nc=cur.c+dc;
      if (nr<1||nr>GRID||nc<1||nc>GRID) continue;
      let alt = dist[key(cur.r,cur.c)] + 1;
      if (alt < dist[key(nr,nc)]) {
        dist[key(nr,nc)] = alt;
        prev[key(nr,nc)] = {r:cur.r,c:cur.c};
        pq.push({r:nr,c:nc,d:alt});
      }
    }
  }
  let path=[], cur=end;
  while (cur) { path.unshift(cur); cur = prev[key(cur.r,cur.c)]; }
  return path;
}

// ── turn helper: rotate from current dir to face a delta ──────────
// Returns array of commands needed to face (dr,dc) from dir
function turnsNeeded(dr, dc) {
  // desired direction index
  let desired;
  if      (dr===-1) desired=0;
  else if (dc===1)  desired=1;
  else if (dr===1)  desired=2;
  else              desired=3;
  let diff = (desired - dir + 4) % 4;
  if (diff===0) return [];
  if (diff===1) return ['R'];
  if (diff===3) return ['L'];
  return ['R','R'];   // 180°
}

// ── execute one step toward path[i] ───────────────────────────────
async function executeStep(target) {
  let dr = target.r - pos.r;
  let dc = target.c - pos.c;
  let turns = turnsNeeded(dr, dc);

  // apply turns
  for (let t of turns) {
    send(t); await sleep(T_TURN); send('S'); await sleep(80);
    dir = t==='R' ? (dir+1)%4 : (dir+3)%4;
  }
  // move forward
  send('F');
  await sleep(T_MOVE);
  send('S');
  pos = target;
  markVisited(pos.r, pos.c);
}

// ── obstacle polling during auto movement ─────────────────────────
async function pollObstacle(ms) {
  let start = Date.now();
  while (Date.now()-start < ms) {
    let res = await fetch('/dist');
    let d   = parseInt(await res.text());
    if (d < 20) return true;
    await sleep(80);
  }
  return false;
}

// ── detour: turn until clear, take one step, then repath ──────────
async function doDetour() {
  showAlert(true);
  obstacleMode = true;
  send('S');

  // try turning right until clear (max 3 tries)
  for (let attempt=0; attempt<3; attempt++) {
    // check left first, then right
    for (let turn of ['L','R','R']) {
      send(turn); await sleep(T_TURN); send('S'); await sleep(100);
      dir = turn==='R' ? (dir+1)%4 : (dir+3)%4;
      let res = await fetch('/dist');
      let d = parseInt(await res.text());
      if (d >= 25) {
        // gap found — take one step forward into it
        send('F'); await sleep(T_MOVE); send('S'); await sleep(100);
        // update pos based on current dir
        if      (dir===0) pos.r--;
        else if (dir===1) pos.c++;
        else if (dir===2) pos.r++;
        else              pos.c--;
        // clamp to grid
        pos.r = Math.max(1, Math.min(GRID, pos.r));
        pos.c = Math.max(1, Math.min(GRID, pos.c));
        obstacleMode = false;
        showAlert(false);
        return;
      }
    }
    // nowhere to go — back up one step and retry
    send('B'); await sleep(T_MOVE); send('S'); await sleep(100);
    if      (dir===0) pos.r++;
    else if (dir===1) pos.c--;
    else if (dir===2) pos.r--;
    else              pos.c++;
    pos.r = Math.max(1, Math.min(GRID, pos.r));
    pos.c = Math.max(1, Math.min(GRID, pos.c));
  }
  obstacleMode = false;
  showAlert(false);
}

// ── main auto runner ──────────────────────────────────────────────
async function startAuto() {
  if (!destination || running) return;
  running = true;
  pathStack = [];

  while (running) {
    // build fresh path from current position
    let path = dijkstra(pos, destination);
    if (path.length <= 1) break;   // arrived

    for (let i=1; i<path.length && running; i++) {
      // quick obstacle check before each step
      let res = await fetch('/dist');
      let d = parseInt(await res.text());

      if (d < 20) {
        // push remaining path
        pathStack = path.slice(i);
        await doDetour();
        // after detour, break inner loop → outer while re-paths
        break;
      }

      await executeStep(path[i]);

      // check we actually reached destination
      if (pos.r===destination.r && pos.c===destination.c) {
        running = false;
        return;
      }
    }
  }
  running = false;
}

function stopAuto() { running = false; send('S'); showAlert(false); }

function resetAll() {
  stopAuto();
  pos = {r:3, c:3}; dir = 0; destination = null; pathStack = [];
  document.querySelectorAll('.cell').forEach(c => {
    c.classList.remove('sel','visited');
  });
  document.getElementById('c33').classList.add('origin');
}
</script>
</head>
<body onload="showMode('manual')">

<div class="modeBar">
  <button id="btnauto"   class="modeBtn" onclick="showMode('auto')">AUTO</button>
  <button id="btnmanual" class="modeBtn" onclick="showMode('manual')">MANUAL</button>
</div>

<!-- ───── AUTO MODE ───── -->
<div id="auto" class="hidden">
  <iframe src="http://192.168.4.2/stream"></iframe>
  <div id="obstacleAlert">⚠ OBSTACLE DETECTED — rerouting…</div>
  <div class="slider-label">📷 Pan Camera:</div>
  <input type="range" min="15" max="155" value="85"
    oninput="moveServo(this.value)"
    onchange="moveServo(85); this.value=85"><br>
  <div class="grid">
    <script>
      for (let r=1;r<=5;r++) for (let c=1;c<=5;c++) {
        let cls=(r==3&&c==3)?'cell origin':'cell';
        document.write(`<div id="c${r}${c}" class="${cls}" onclick="clickCell(${r},${c})"></div>`);
      }
    </script>
  </div><br>
  <button class="start" onclick="startAuto()">START</button>
  <button class="stop"  onclick="stopAuto()">STOP</button>
  <button class="reset" onclick="resetAll()">RESET</button>
</div>

<!-- ───── MANUAL MODE ───── -->
<div id="manual" class="hidden">
  <iframe src="http://192.168.4.2/stream"></iframe>
  <div id="obstacleAlert">⚠ OBSTACLE — auto-brake engaged</div>
  <div class="slider-label">📷 Pan Camera:</div>
  <input type="range" min="15" max="155" value="85"
    oninput="moveServo(this.value)"
    onchange="moveServo(85); this.value=85"><br>
  <div class="slider-label">⚡ Speed:</div>
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

// ─── setup ─────────────────────────────────────────────────────────
void setup() {
  analogWriteRange(255);
  analogWriteFreq(1000);

  pinMode(IN1,OUTPUT); pinMode(IN2,OUTPUT);
  pinMode(IN3,OUTPUT); pinMode(IN4,OUTPUT);
  pinMode(ENA,OUTPUT); pinMode(ENB,OUTPUT);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);   // D0/GPIO16 does NOT support INPUT_PULLUP

  panServo.attach(PAN_SERVO);
  panServo.write(PAN_CENTER);

  WiFi.softAP(ssid, password);

  // ── basic movement ──
  server.on("/F", [](){
    if (obstacleDetected) { server.send(200,"text/plain","blocked"); return; }
    applySpeed(); forward(); server.send(200,"text/plain","ok");
  });
  server.on("/B", [](){
    applySpeed(); reverse();  server.send(200,"text/plain","ok");
  });
  server.on("/L", [](){
    applySpeed(); turnLeft(); server.send(200,"text/plain","ok");
  });
  server.on("/R", [](){
    applySpeed(); turnRight();server.send(200,"text/plain","ok");
  });
  server.on("/S", [](){
    stopMotors(); server.send(200,"text/plain","ok");
  });

  // ── speed ──
  server.on("/speed", [](){
    speedValue = constrain(server.arg("val").toInt(), SPEED_MIN, SPEED_MAX);
    applySpeed();
    server.send(200,"text/plain","ok");
  });

  // ── servo ──
  server.on("/servo", [](){
    int a = constrain(server.arg("angle").toInt(), PAN_MIN, PAN_MAX);
    panServo.write(a);
    server.send(200,"text/plain","ok");
  });

  // ── distance endpoint — polled by JS ──
  server.on("/dist", [](){
    long d = readDistanceCM();
    server.send(200,"text/plain", String(d));
  });

  server.on("/", [](){
    server.send(200,"text/html", htmlPage());
  });

  server.begin();
}

// ─── loop ──────────────────────────────────────────────────────────
void loop() {
  server.handleClient();

  // ── Continuous obstacle check (non-blocking style) ──
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 100) {
    lastCheck = millis();
    long d = readDistanceCM();
    if (d < OBSTACLE_DIST_CM) {
      obstacleDetected = true;
      stopMotors();         // immediate hard stop
    } else if (d > SAFE_DIST_CM) {
      obstacleDetected = false;
    }
  }
}
