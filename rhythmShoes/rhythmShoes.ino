
#include "I2Cdev.h"

#include "MPU9250_9Axis_MotionApps41.h"
#include <SoftwareSerial.h>

#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
#include "Wire.h"
#endif
#define rxPin 7 // lcd 7 -> 12
#define txPin 5 // lcd 5 -> 10
SoftwareSerial BTSerial(rxPin, txPin);

char INBYTE;
int  LED = 13; // LED on pin 13

  bool isStart = false;
  bool isStand = false;
MPU9250 mpu;

#define OUTPUT_READABLE_YAWPITCHROLL

#define INTERRUPT_PIN 2  // use pin 2 on Arduino Uno & most boards
#define LED_PIN 13 // (Arduino is 13, Teensy is 11, Teensy++ is 6)
bool blinkState = false;

// MPU control/status vars
bool dmpReady = false;  // set true if DMP init was successful
uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint8_t fifoBuffer[64]; // FIFO storage buffer

// orientation/motion vars
Quaternion q;           // [w, x, y, z]         quaternion container
VectorInt16 aa;         // [x, y, z]            accel sensor measurements
VectorInt16 aaReal;     // [x, y, z]            gravity-free accel sensor measurements
VectorInt16 aaWorld;    // [x, y, z]            world-frame accel sensor measurements
VectorFloat gravity;    // [x, y, z]            gravity vector
float euler[3];         // [psi, theta, phi]    Euler angle container
float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector

//LCD 코드
#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 4, 10, 6, 12);  // LCD 제어를 위한 핀 설정
// 버튼 및 키 번호를 부여합니다.
int lcd_key = 0;
int adc_key_in = 0;

int read_LCD_buttons()
{
  adc_key_in = analogRead(0);      // AO 핀으로부터 아날로그값을 읽어옴
}

//압력센서 코드
int sensorPin = A1;
int sensorValue = 0;
int cnt = 0;
int d = 0;
bool flag = false;
int TH = 970;
int MAX = 1000;

// ================================================================
// ===               INTERRUPT DETECTION ROUTINE                ===
// ================================================================

volatile bool mpuInterrupt = false;     // indicates whether MPU interrupt pin has gone high
void dmpDataReady() {
  mpuInterrupt = true;
}


// ================================================================
// ===                      INITIAL SETUP                       ===
// ================================================================

void setup() {
  // join I2C bus (I2Cdev library doesn't do this automatically)
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
  Wire.begin();
  Wire.setClock(400000); // 400kHz I2C clock. Comment this line if having compilation difficulties
#elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
  Fastwire::setup(400, true);
#endif

  Serial.begin(9600);
  BTSerial.begin(9600);
  pinMode(LED, OUTPUT);
  while (!Serial);

  // initialize device
  Serial.println(F("Initializing I2C devices..."));
  mpu.initialize();
  pinMode(INTERRUPT_PIN, INPUT);

  // verify connection
  Serial.println(F("Testing device connections..."));
  Serial.println(mpu.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));

  /*
  // wait for ready
  Serial.println(F("\nSend any character to begin DMP programming and demo: "));
  while (Serial.available() && Serial.read()); // empty buffer
  while (!Serial.available());                 // wait for data
  while (Serial.available() && Serial.read()); // empty buffer again
  */

  // load and configure the DMP
  Serial.println(F("Initializing DMP..."));
  devStatus = mpu.dmpInitialize();

  // make sure it worked (returns 0 if so)
  if (devStatus == 0) {
    // turn on the DMP, now that it's ready
    Serial.println(F("Enabling DMP..."));
    mpu.setDMPEnabled(true);

    // enable Arduino interrupt detection
    Serial.println(F("Enabling interrupt detection (Arduino external interrupt 0)..."));
    attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), dmpDataReady, RISING);
    mpuIntStatus = mpu.getIntStatus();

    // set our DMP Ready flag so the main loop() function knows it's okay to use it
    Serial.println(F("DMP ready! Waiting for first interrupt..."));
    dmpReady = true;

    // get expected DMP packet size for later comparison
    packetSize = mpu.dmpGetFIFOPacketSize();
  }
  else {
    // ERROR!
    // 1 = initial memory load failed
    // 2 = DMP configuration updates failed
    // (if it's going to break, usually the code will be 1)
    Serial.print(F("DMP Initialization failed (code "));
    Serial.print(devStatus);
    Serial.println(F(")"));
  }

  // configure LED for output
  pinMode(LED_PIN, OUTPUT);

  // LCD 셋업
  lcd.begin(16, 2);              // LCD 초기화
  lcd.setCursor(0, 0);
  
  Serial.println("Press 1 to turn Arduino pin 13 LED ON or 0 to turn it OFF:");
  BTSerial.println("Press 1 to turn Arduino pin 13 LED ON or 0 to turn it OFF:");
}



// ================================================================
// ===                    MAIN PROGRAM LOOP                     ===
// ================================================================

void loop() {  
// 
//  if (BTSerial.available()) 
//  {       
//    Serial.write(BTSerial.read());  //블루투스측 내용을 시리얼모니터에 출력W
//  }
//  if (Serial.available()) 
//  {         
//    BTSerial.write(Serial.read());  //시리얼 모니터 내용을 블루추스 측에 WRITE
//  }
//  
  // if programming failed, don't try to do anything
  if (!dmpReady) return;

  // wait for MPU interrupt or extra packet(s) available
  while (!mpuInterrupt && fifoCount < packetSize) {
    // other program behavior stuff here
    // if you are really paranoid you can frequently test in between other
    // stuff to see if mpuInterrupt is true, and if so, "break;" from the
    // while() loop to immediately process the MPU data
  }

  // reset interrupt flag and get INT_STATUS byte
  mpuInterrupt = false;
  mpuIntStatus = mpu.getIntStatus();

  // get current FIFO count
  fifoCount = mpu.getFIFOCount();

  // check for overflow (this should never happen unless our code is too inefficient)
  if ((mpuIntStatus & 0x10) || fifoCount == 1024) {
    // reset so we can continue cleanly
    mpu.resetFIFO();
    //Serial.println(F("FIFO overflow!"));
    // otherwise, check for DMP data ready interrupt (this should happen frequently)
  }
  else if (mpuIntStatus & 0x02) {
    // wait for correct available data length, should be a VERY short wait
    while (fifoCount < packetSize) fifoCount = mpu.getFIFOCount();

    // read a packet from FIFO
    mpu.getFIFOBytes(fifoBuffer, packetSize);

    // track FIFO count here in case there is > 1 packet available
    // (this lets us immediately read more without waiting for an interrupt)
    fifoCount -= packetSize;

#ifdef OUTPUT_READABLE_YAWPITCHROLL
   // display Euler angles in degrees
    mpu.dmpGetQuaternion(&q, fifoBuffer);
    mpu.dmpGetGravity(&gravity, &q);
    mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
//    Serial.print("ypr\t");
//   Serial.print(ypr[0] * 180 / M_PI);
//  Serial.print("\t");
//    Serial.print(ypr[1] * 180 / M_PI);
//    Serial.print("\t");
//    Serial.print(ypr[2] * 180 / M_PI);
//    Serial.print("\n");
//    BTSerial.print("ypr\t");
//    BTSerial.print(ypr[0] * 180 / M_PI);
//    BTSerial.print("\t");
//    BTSerial.print(ypr[1] * 180 / M_PI);
//    BTSerial.print("\t");
//    BTSerial.print(ypr[2] * 180 / M_PI);
//    BTSerial.print("\n");
#endif

    // blink LED to indicate activity
    //blinkState = !blinkState;
    //digitalWrite(LED_PIN, blinkState);
  }

    INBYTE = BTSerial.read();        // read next available byte
    if( INBYTE == '0' || INBYTE == 0 ) isStart = false;
    else if ( INBYTE == '1' || INBYTE == 1 ) isStart = true;
    if( !isStart) 
    {
      cnt = 0;
      lcd.clear();
    } 
    else if( isStart ) 
    {
    
      //LCD 루프
      //압력센서 루프
      sensorValue = analogRead(sensorPin);
    
      if (d * 50 > 2000) {
        lcd.setCursor(4, 0);
        lcd.println("STAND       ");
        BTSerial.print(ypr[0] * 180 / M_PI);
        BTSerial.print(",");
        BTSerial.print(cnt);
        BTSerial.print(",");
        BTSerial.print("1");
        BTSerial.print("\n");  
        isStand = true;
      }
    
      if (TH > sensorValue && !flag) {
        isStand = false;
//        BTSerial.print("ypr\t");
    BTSerial.print(ypr[0] * 180 / M_PI);
    BTSerial.print(",");
//    BTSerial.print(ypr[1] * 180 / M_PI);
//    BTSerial.print("\t");
//    BTSerial.print(ypr[2] * 180 / M_PI);
//    BTSerial.print(".");

        cnt++;
        flag = true;
        lcd.setCursor(4, 0);
        lcd.print("STEP ");
        lcd.print(cnt);
//        BTSerial.print("STEP "); 
        BTSerial.print(cnt);
        BTSerial.print(",");
        BTSerial.print("0");
        BTSerial.print("\n");  
        
        d = 0;
      }


      
      else if (MAX < sensorValue) {
        flag = false;
        d++;
      }
      else {
        d++;
      }
    
      lcd.setCursor(4, 1);            // 2번째 줄(1) 10번째(9) 패널에 위치하고
      if ((ypr[0] * 180 / M_PI > 20) || (ypr[0] * 180 / M_PI < -20))
        lcd.print("WRONG");
      else
        lcd.print("RIGHT");
    
      lcd.setCursor(0, 1);            // 2번째 줄(1) 1번째(0) 패널에 위치하고

    }

  
  delay(50);
}
