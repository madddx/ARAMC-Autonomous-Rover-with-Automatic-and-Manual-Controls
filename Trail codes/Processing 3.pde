import processing.serial.*;

Serial myPort;

String data = "";
int panAngle = 0;
int tiltAngle = 0;
int distance = 0;

// Store previous points for fading
class ScanPoint {
  float x, y;
  int alpha;
  ScanPoint(float x, float y) {
    this.x = x;
    this.y = y;
    this.alpha = 255;
  }
}

ArrayList<ScanPoint> scanPoints = new ArrayList<ScanPoint>();

void setup() {
  size(1200, 700);
  smooth();

  println("Available ports:");
  println(Serial.list());
  
  myPort = new Serial(this, "COM4", 9600); //CHANGE THE COMS BASED ON YOUR CONNECTIONS
  myPort.bufferUntil('\n');
}

void draw() {
  // Fade effect
  fill(0, 15);  
  noStroke();
  rect(0,0,width,height);

  drawRadar();
  drawObjects();
  drawText();
}

void serialEvent(Serial myPort) {
  data = myPort.readStringUntil('\n');
  if (data == null) return;
  data = trim(data);

  // Expect format: P:90 T:45 D:12
  String[] parts = splitTokens(data, " :");
  if (parts.length >= 6) {
    panAngle = int(parts[1]);
    tiltAngle = int(parts[3]);
    distance = int(parts[5]);
  }

  // Add point to scan points array if within range
  if(distance <= 40) {
    float pixDist = distance * ((height-height*0.1666)*0.025);
    float x = width/2 + pixDist*cos(radians(panAngle));
    float y = height - height*0.074 - pixDist*sin(radians(panAngle));
    scanPoints.add(new ScanPoint(x, y));
  }
}

// Draw radar arcs and angle lines
void drawRadar() {
  pushMatrix();
  translate(width/2, height-height*0.074);

  noFill();
  strokeWeight(2);
  stroke(98,245,31);

  arc(0,0,(width-width*0.0625),(width-width*0.0625),PI,TWO_PI);
  arc(0,0,(width-width*0.27),(width-width*0.27),PI,TWO_PI);
  arc(0,0,(width-width*0.479),(width-width*0.479),PI,TWO_PI);
  arc(0,0,(width-width*0.687),(width-width*0.687),PI,TWO_PI);

  line(-width/2,0,width/2,0);
  line(0,0,(-width/2)*cos(radians(30)),(-width/2)*sin(radians(30)));
  line(0,0,(-width/2)*cos(radians(60)),(-width/2)*sin(radians(60)));
  line(0,0,(-width/2)*cos(radians(90)),(-width/2)*sin(radians(90)));
  line(0,0,(-width/2)*cos(radians(120)),(-width/2)*sin(radians(120)));
  line(0,0,(-width/2)*cos(radians(150)),(-width/2)*sin(radians(150)));

  popMatrix();
}

// Draw all scanned points with fading effect
void drawObjects() {
  for (int i = scanPoints.size()-1; i >= 0; i--) {
    ScanPoint sp = scanPoints.get(i);
    stroke(255,0,0, sp.alpha);
    strokeWeight(8);
    point(sp.x, sp.y);

    sp.alpha -= 5; // fade effect
    if(sp.alpha <= 0) {
      scanPoints.remove(i);
    }
  }
}

// Display angle and distance info at the bottom
void drawText() {
  pushMatrix();
  fill(0);
  noStroke();
  rect(0, height-height*0.0648, width, height);

  fill(98,245,31);
  textSize(40);

  text("Pan: " + panAngle + "°", 40, height-20);
  text("Tilt: " + tiltAngle + "°", 300, height-20);
  text("Distance: " + distance + " cm", 550, height-20);

  popMatrix();
}
