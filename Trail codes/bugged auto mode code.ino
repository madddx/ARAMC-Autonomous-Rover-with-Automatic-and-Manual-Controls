#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Servo.h>

ESP8266WebServer server(80);

const char* ssid     = "ESP8266_CAR";
const char* password = "12345678";

#define IN1 D1
#define IN2 D2
#define IN3 D3
#define IN4 D4
#define ENA D7
#define ENB D6

#define PAN_SERVO  D5
#define PAN_MIN    15
#define PAN_MAX    155
#define PAN_CENTER 85
Servo panServo;

#define TRIG_PIN D8
#define ECHO_PIN D0

#define OBSTACLE_DIST_CM 20
#define SAFE_DIST_CM     25

#define SPEED_MIN 70
#define SPEED_MAX 255
int speedValue = 150;

bool obstacleDetected = false;

long readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long dur = pulseIn(ECHO_PIN, HIGH, 30000UL);
  if (dur == 0) return 999;
  return dur / 58L;
}

void applySpeed() { analogWrite(ENA, speedValue); analogWrite(ENB, speedValue); }

void stopMotors() {
  digitalWrite(IN1,LOW); digitalWrite(IN2,LOW);
  digitalWrite(IN3,LOW); digitalWrite(IN4,LOW);
}

void forward() {
  if (obstacleDetected) { stopMotors(); return; }
  stopMotors(); delay(40);
  digitalWrite(IN1,HIGH); digitalWrite(IN2,LOW);
  digitalWrite(IN3,HIGH); digitalWrite(IN4,LOW);
}

void reverse() {
  stopMotors(); delay(40);
  digitalWrite(IN1,LOW);  digitalWrite(IN2,HIGH);
  digitalWrite(IN3,LOW);  digitalWrite(IN4,HIGH);
}

void turnLeft() {
  stopMotors(); delay(40);
  digitalWrite(IN1,HIGH); digitalWrite(IN2,LOW);
  digitalWrite(IN3,LOW);  digitalWrite(IN4,HIGH);
}

void turnRight() {
  stopMotors(); delay(40);
  digitalWrite(IN1,LOW);  digitalWrite(IN2,HIGH);
  digitalWrite(IN3,HIGH); digitalWrite(IN4,LOW);
}

String htmlPage() {
  return R"rawliteral(
<!DOCTYPE html><html>
<head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<style>
body{background:#111;color:#fff;font-family:Arial;text-align:center;margin:0}
.modeBar{display:flex;background:#000;padding:10px}
.modeBtn{flex:1;margin:5px;height:50px;font-size:18px;border:none;border-radius:10px;background:#333;color:#fff}
.modeBtn.active{background:#1abc9c}
.hidden{display:none}
iframe{width:95%;max-width:400px;aspect-ratio:4/3;border:2px solid #1abc9c;border-radius:10px;margin-top:10px}
.grid{display:grid;grid-template-columns:repeat(5,64px);gap:6px;justify-content:center;margin:16px auto}
.cell{
  width:64px;height:64px;border-radius:10px;background:#2c2c2c;
  display:flex;align-items:center;justify-content:center;
  font-size:11px;color:#888;cursor:pointer;transition:background .2s
}
.cell.origin  {background:#e67e22;color:#fff}
.cell.current {background:#f39c12;color:#fff}
.cell.sel     {background:#1abc9c;color:#fff}
.cell.visited {background:#1a3a2a;color:#1abc9c}
.cell.blocked {background:#4a1010;color:#e74c3c;font-weight:bold}
.cell.path    {background:#154360;color:#5dade2}
button{margin:6px;padding:10px 18px;font-size:16px;border:none;border-radius:10px;color:#fff}
.start{background:#1abc9c}.stop{background:#e74c3c}.reset{background:#3498db}
input[type=range]{width:90%}
.lbl{font-size:13px;color:#aaa;margin-top:10px}
#alert{
  display:none;background:#c0392b;color:#fff;padding:10px;
  margin:8px auto;max-width:360px;border-radius:10px;font-size:14px;font-weight:bold
}
#alert.show{display:block}
#statusBar{font-size:12px;color:#aaa;margin:6px 0;min-height:18px}
</style>

<script>
const GRID   = 5;
const T_TURN = 400;
const T_MOVE = 900;
const T_REV  = 950;   // reverse duration when backing away from obstacle

let running     = false;
let destination = null;
let pos         = {r:3, c:3};
let dir         = 0;           // 0=N 1=E 2=S 3=W
let blockedCells= new Set();   // "r,c" strings

const send      = cmd => fetch('/'+cmd);
const setSpeed  = v   => fetch('/speed?val='+v);
const moveServo = v   => fetch('/servo?angle='+v);
const sleep     = ms  => new Promise(r => setTimeout(r, ms));
const cellKey   = (r,c) => r+','+c;

// ── UI helpers ────────────────────────────────────────────────────
function setStatus(msg) {
  document.getElementById('statusBar').textContent = msg;
}

function showAlert(on, msg='') {
  const el = document.getElementById('alert');
  el.classList.toggle('show', on);
  if (msg) el.textContent = msg;
}

function renderGrid() {
  for (let r=1;r<=GRID;r++) for (let c=1;c<=GRID;c++) {
    const el  = document.getElementById('c'+r+c);
    const key = cellKey(r,c);
    el.className = 'cell';
    el.textContent = '';
    if (blockedCells.has(key))              { el.classList.add('blocked');  el.textContent='✕'; }
    else if (r===pos.r && c===pos.c)        { el.classList.add('current');  el.textContent='🚗'; }
    else if (destination && r===destination.r && c===destination.c) { el.classList.add('sel'); el.textContent='🏁'; }
    else if (r===3 && c===3)                { el.classList.add('origin'); }
  }
}

function highlightPath(path) {
  renderGrid();
  for (let i=1; i<path.length-1; i++) {
    const key = cellKey(path[i].r, path[i].c);
    if (!blockedCells.has(key)) {
      const el = document.getElementById('c'+path[i].r+path[i].c);
      el.className = 'cell path';
      el.textContent = '·';
    }
  }
}

function showMode(mode) {
  ['auto','manual'].forEach(m => {
    document.getElementById(m).classList.add('hidden');
    document.getElementById('btn'+m).classList.remove('active');
  });
  document.getElementById(mode).classList.remove('hidden');
  document.getElementById('btn'+mode).classList.add('active');
}

function clickCell(r,c) {
  if (running) return;
  if (blockedCells.has(cellKey(r,c))) return;  // can't target a blocked cell
  destination = {r,c};
  renderGrid();
}

// ── Dijkstra — respects blockedCells ─────────────────────────────
function dijkstra(start, end) {
  const key = (r,c) => r+','+c;
  let dist={}, prev={}, pq=[];
  for (let r=1;r<=GRID;r++) for (let c=1;c<=GRID;c++) {
    dist[key(r,c)] = Infinity; prev[key(r,c)] = null;
  }
  dist[key(start.r,start.c)] = 0;
  pq.push({r:start.r,c:start.c,d:0});

  while (pq.length) {
    pq.sort((a,b)=>a.d-b.d);
    let cur = pq.shift();
    if (cur.r===end.r && cur.c===end.c) break;
    for (let [dr,dc] of [[-1,0],[1,0],[0,-1],[0,1]]) {
      let nr=cur.r+dr, nc=cur.c+dc;
      if (nr<1||nr>GRID||nc<1||nc>GRID) continue;
      if (blockedCells.has(key(nr,nc))) continue;   // ← skip blocked
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
  // validate: path must start at start
  if (path.length===0 || path[0].r!==start.r || path[0].c!==start.c) return null;
  return path;
}

// ── work out which cell is directly in front of current pos+dir ──
function cellInFront() {
  let fr = pos.r, fc = pos.c;
  if      (dir===0) fr--;
  else if (dir===1) fc++;
  else if (dir===2) fr++;
  else              fc--;
  if (fr<1||fr>GRID||fc<1||fc>GRID) return null;
  return {r:fr, c:fc};
}

// ── turn the car to face a grid delta ────────────────────────────
async function faceDirection(dr, dc) {
  let desired;
  if      (dr===-1) desired=0;
  else if (dc=== 1) desired=1;
  else if (dr=== 1) desired=2;
  else              desired=3;

  let diff = (desired - dir + 4) % 4;
  if (diff===0) return;

  if (diff===1) {
    send('R'); await sleep(T_TURN); send('S'); await sleep(80);
    dir = (dir+1)%4;
  } else if (diff===3) {
    send('L'); await sleep(T_TURN); send('S'); await sleep(80);
    dir = (dir+3)%4;
  } else {
    // 180°: two right turns
    send('R'); await sleep(T_TURN); send('S'); await sleep(80); dir=(dir+1)%4;
    send('R'); await sleep(T_TURN); send('S'); await sleep(80); dir=(dir+1)%4;
  }
}

// ── poll /dist once ───────────────────────────────────────────────
async function getDistance() {
  try {
    const r = await fetch('/dist');
    return parseInt(await r.text());
  } catch(e) { return 999; }
}

// ── obstacle handler ─────────────────────────────────────────────
//   called the moment distance < threshold during a step
async function handleObstacle() {
  send('S');
  showAlert(true, '⚠ Obstacle! Marking cell, reversing, rerouting…');
  setStatus('Obstacle detected');

  // 1. mark the cell in front as blocked
  const front = cellInFront();
  if (front) {
    blockedCells.add(cellKey(front.r, front.c));
    setStatus('Blocked: '+cellKey(front.r, front.c));
  }

  // 2. reverse away for T_REV ms
  applySpeed(); // tell ESP to set speed
  send('B');
  await sleep(T_REV);
  send('S');
  await sleep(150);

  // 3. update position estimate: we backed up half a cell
  //    (we didn't complete the forward step, so pos stays the same —
  //     we just moved backward within the current cell, still at pos)

  showAlert(false);
  renderGrid();
}

// ── single step executor (returns false if obstacle mid-move) ────
async function executeStep(target) {
  let dr = target.r - pos.r;
  let dc = target.c - pos.c;

  // turn to face target
  await faceDirection(dr, dc);

  // pre-move distance check
  let d = await getDistance();
  if (d < OBSTACLE_DIST_CM) return false;

  // send forward and monitor during movement
  applySpeed();
  send('F');

  let blocked = false;
  let elapsed = 0;
  const POLL  = 100;
  while (elapsed < T_MOVE) {
    await sleep(POLL);
    elapsed += POLL;
    d = await getDistance();
    if (d < OBSTACLE_DIST_CM) { blocked = true; break; }
  }

  send('S');
  await sleep(80);

  if (blocked) return false;

  // completed step — update position
  pos = {r: target.r, c: target.c};
  renderGrid();
  return true;
}

// ── main auto loop ────────────────────────────────────────────────
async function startAuto() {
  if (!destination || running) return;
  running = true;
  setStatus('Starting…');

  while (running) {
    if (pos.r===destination.r && pos.c===destination.c) {
      setStatus('Arrived! 🏁');
      break;
    }

    // repath from current position, skipping blocked cells
    let path = dijkstra(pos, destination);

    if (!path || path.length <= 1) {
      setStatus('No route — all paths blocked or already arrived');
      showAlert(true, '🚫 No route to destination. Try resetting blocked cells.');
      break;
    }

    highlightPath(path);
    setStatus('Route found: '+path.length+' steps');

    // execute path step by step
    let repathNeeded = false;
    for (let i=1; i<path.length && running; i++) {
      setStatus('Moving to ('+path[i].r+','+path[i].c+')…');
      let ok = await executeStep(path[i]);

      if (!ok) {
        // obstacle hit during this step → mark, reverse, repath
        await handleObstacle();
        repathNeeded = true;
        break;
      }
    }

    if (!repathNeeded && running) {
      // completed path cleanly — loop check will catch arrival
    }
  }

  running = false;
}

function stopAuto() {
  running = false;
  send('S');
  showAlert(false);
  setStatus('Stopped');
}

function resetAll() {
  stopAuto();
  pos = {r:3,c:3}; dir=0; destination=null; blockedCells.clear();
  renderGrid();
  setStatus('');
}

function clearBlocked() {
  blockedCells.clear();
  renderGrid();
  setStatus('Blocked cells cleared');
}
</script>
</head>
<body onload="showMode('manual'); renderGrid()">

<div class="modeBar">
  <button id="btnauto"   class="modeBtn" onclick="showMode('auto')">AUTO</button>
  <button id="btnmanual" class="modeBtn" onclick="showMode('manual')">MANUAL</button>
</div>

<!-- AUTO -->
<div id="auto" class="hidden">
  <iframe src="http://192.168.4.2/stream"></iframe>
  <div id="alert"></div>
  <div id="statusBar"></div>
  <div class="grid">
    <script>
      for(let r=1;r<=5;r++) for(let c=1;c<=5;c++)
        document.write(`<div id="c${r}${c}" class="cell" onclick="clickCell(${r},${c})"></div>`);
    </script>
  </div>
  <button class="start" onclick="startAuto()">START</button>
  <button class="stop"  onclick="stopAuto()">STOP</button>
  <button class="reset" onclick="resetAll()">RESET</button>
  <button style="background:#7d3c98" onclick="clearBlocked()">CLEAR BLOCKS</button>
</div>

<!-- MANUAL -->
<div id="manual" class="hidden">
  <iframe src="http://192.168.4.2/stream"></iframe>
  <div id="alert"></div>
  <div class="lbl">📷 Pan Camera:</div>
  <input type="range" min="15" max="155" value="85"
    oninput="moveServo(this.value)" onchange="moveServo(85);this.value=85"><br>
  <div class="lbl">⚡ Speed:</div>
  <input type="range" min="70" max="255" value="150" oninput="setSpeed(this.value)"><br>
  <button ontouchstart="send('F')" ontouchend="send('S')">FORWARD</button><br>
  <button ontouchstart="send('L')" ontouchend="send('S')">LEFT</button>
  <button ontouchstart="send('R')" ontouchend="send('S')">RIGHT</button><br>
  <button ontouchstart="send('B')" ontouchend="send('S')">REVERSE</button><br>
  <button onclick="send('S')">STOP</button>
</div>

</body></html>
)rawliteral";
}

void setup() {
  analogWriteRange(255); analogWriteFreq(1000);
  pinMode(IN1,OUTPUT); pinMode(IN2,OUTPUT);
  pinMode(IN3,OUTPUT); pinMode(IN4,OUTPUT);
  pinMode(ENA,OUTPUT); pinMode(ENB,OUTPUT);
  pinMode(TRIG_PIN,OUTPUT);
  pinMode(ECHO_PIN,INPUT);

  panServo.attach(PAN_SERVO);
  panServo.write(PAN_CENTER);

  WiFi.softAP(ssid, password);

  server.on("/", [](){server.send(200,"text/html",htmlPage());});

  server.on("/F", [](){
    if(obstacleDetected){server.send(200,"text/plain","blocked");return;}
    applySpeed(); forward(); server.send(200,"text/plain","ok");
  });
  server.on("/B", [](){applySpeed();reverse();server.send(200,"text/plain","ok");});
  server.on("/L", [](){applySpeed();turnLeft();server.send(200,"text/plain","ok");});
  server.on("/R", [](){applySpeed();turnRight();server.send(200,"text/plain","ok");});
  server.on("/S", [](){stopMotors();server.send(200,"text/plain","ok");});

  server.on("/speed",[](){
    speedValue=constrain(server.arg("val").toInt(),SPEED_MIN,SPEED_MAX);
    applySpeed(); server.send(200,"text/plain","ok");
  });
  server.on("/servo",[](){
    panServo.write(constrain(server.arg("angle").toInt(),PAN_MIN,PAN_MAX));
    server.send(200,"text/plain","ok");
  });
  server.on("/dist",[](){
    server.send(200,"text/plain",String(readDistanceCM()));
  });

  server.begin();
}

void loop() {
  server.handleClient();

  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 100) {
    lastCheck = millis();
    long d = readDistanceCM();
    if (d < OBSTACLE_DIST_CM) {
      obstacleDetected = true;
      stopMotors();
    } else if (d > SAFE_DIST_CM) {
      obstacleDetected = false;
    }
  }
}
