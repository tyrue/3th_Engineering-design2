import org.firmata.*;
import cc.arduino.*;
import processing.serial.*;

Serial myPort;
String myString = null;
float angle = 0.0;
float iniAngle = 0.0;
int cnt = 0;
boolean isIni = false, isStand = false;
int btS = 0, fnS = 0;

void setup() {
  size(600,400);
  myPort = new Serial(this, "COM8", 9600);
  
}

void draw() {
  background(255);
// RECIEVE VALUE
 try
  {
    if(myPort.available() > 0) // 
    {
      myString = myPort.readStringUntil('\n'); // 

      String[] v_list = split(myString, ',');
      angle = float(v_list[0]);
      cnt = int(v_list[1]);
      if(int(v_list[2].replace("\n", "")) == 1) isStand = true;
      else     isStand = false;
      if(myString != null)
      {
        println("yow : " + angle);
        println("cnt : " + cnt);
      }
      
    }
  }
  catch(Exception e)
  {
    
  }
  
  fnS = millis();
  println(angle);
  if(isIni == true)
  {
    text("Initiating",280,50);
    text( (fnS-btS) + "ms", 280, 70);
    if(fnS - btS >= 5000)
    {
      iniAngle = angle;
      isIni = false;
     println("Init finish"); 
    }
  }
  

      // DRAW
  // Left rotate  
  pushMatrix();
    translate(150,150);
    rotate(radians(iniAngle-angle));
    translate(-150,-150);
    LeftDraw();
  popMatrix();
  stroke(255,0,0);
  line(150,0,150,400);

  // Right rotate
  pushMatrix();
    translate(450,150);
   // rotate(PI/6);
    translate(-450,-150);
    RightDraw();
    popMatrix();
  stroke(255,0,0);
  line(450,0,450,400);
  // Button, // (startX, startY, width, height
  fill(255);
  stroke(0);
  rect(250,120,100,50);  // start
  rect(250,200,100,50);  // stop
  // printVslue
  fill(0);
  text("start", 290,150);
  text("stop", 290, 225);
  text((angle-iniAngle), 280,300);
  if(isStand == true) text("stand", 285,320);
  else text(cnt,285,320);
  
  if((angle-iniAngle) > 20 || (angle-iniAngle) < -20) text("Worng", 285, 340);
  else text("Right", 285, 340);
  
}

void mouseClicked() {
  if( (mouseX < 350 && mouseX > 250)
    && (mouseY < 170 && mouseY > 120) ) {
      myPort.write(1);
      btS = millis();
      isIni = true;
    }
    
  if( (mouseX < 350 && mouseX > 250)
    && (mouseY < 250 && mouseY > 200) ) {
      myPort.write(0);
      isIni = false;
    }
}


void LeftDraw() {
  fill(0);
  stroke(0);
  ellipse(150,150,100,200);
  ellipse(150,300,50,80);
  stroke(0,255,0);
  line(150,-100,150,1000);
 
}

void RightDraw() {
  fill(0);
  stroke(0);
  ellipse(450,150,100,200);
  ellipse(450,300,50,80);
  stroke(0,255,0);
  line(450,-100,450,1000); 
}
