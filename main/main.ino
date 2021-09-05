#include <math.h>
#include <SPI.h>
#include <SoftwareSerial.h>

#include <TinyGPS++.h>
TinyGPSPlus gps;
double lat;
double lng;
unsigned int year;
unsigned char month;
unsigned char day;
unsigned char hour;
unsigned char minute;
unsigned char second;
double alt; // in meters
boolean updated;


#define XBEE_RX_PIN 2
#define XBEE_TX_PIN 3
SoftwareSerial XBeeSerial(XBEE_RX_PIN, XBEE_TX_PIN);

#define NICHROME_PIN 4

#define MOTOR_RIN1 8
#define MOTOR_RIN2 7
#define MOTOR_LIN1 6
#define MOTOR_LIN2 5
#define MOTOR_R_FORWARD() {analogWrite(MOTOR_RIN1, 255);analogWrite(MOTOR_RIN2, 0);}
#define MOTOR_R_BACKWARD() {analogWrite(MOTOR_RIN1, 0);analogWrite(MOTOR_RIN2, 255);}
#define MOTOR_R_STOP() {analogWrite(MOTOR_RIN1, 0);analogWrite(MOTOR_RIN2, 0);}
#define MOTOR_L_FORWARD() {analogWrite(MOTOR_LIN1, 255);analogWrite(MOTOR_LIN2, 0);}
#define MOTOR_L_BACKWARD() {analogWrite(MOTOR_LIN1, 0);analogWrite(MOTOR_LIN2, 255);}
#define MOTOR_L_STOP() {analogWrite(MOTOR_RIN1, 0);analogWrite(MOTOR_RIN2, 0);}

#include <SD.h>
File logFile;
String logFileName;

#define SLOGFLN(X) {Serial.println(F((X)));XBeeSerial.println(F((X)));logFile.println(F((X)));logFile.flush();}
#define SLOGF(X) {Serial.print(F((X)));XBeeSerial.print(F((X)));logFile.print(F((X)));logFile.flush();}
#define SLOGLN(X) {Serial.println((X));XBeeSerial.println((X));logFile.println((X));logFile.flush();}
#define SLOG(X) {Serial.print((X));XBeeSerial.print((X));logFile.print((X));logFile.flush();}

#define ULTRASONIC_ECHO_PIN A1
#define ULTRASONIC_TRIG_PIN A0
double duration;
double distance;

#include "nine.h"

#define CDS_PIN A6
String picName = "link";
unsigned char picCount = 0;

#include "camera.h"

unsigned char phase = 3;

// constants
#define TARGET_LAT 35.71361439331066
#define TARGET_LNG 139.76169713751702
#define M_PER_LAT 111092.7384 // https://www.wingfield.gr.jp/archives/9721 lat43
#define M_PER_LNG 1359.0081
#define MAG_OFFSET_X 0.0
#define MAG_OFFSET_Y 0.0
#define MAG_NORTH (PI * 10.0 / 180.0)
#define PI 3.14159265
#define RAD_PER_S_R 0.18
#define RAD_PER_S_L 0.15
#define M_PER_S 0.1

// control constants
#define ROTATE_REPS 3
#define ROTATE_RAD_THRESHOLD (PI * 30.0 / 180.0)

void setup() {
  pinMode(9, OUTPUT);
  digitalWrite(9, HIGH);
  
  // GPS & Serial init
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // XBee init
  XBeeSerial.begin(9600);
  delay(300);
  XBeeSerial.listen();

  // Nichrome init
  pinMode(NICHROME_PIN, OUTPUT);
  digitalWrite(NICHROME_PIN, LOW);

  // Motor init
  pinMode(MOTOR_RIN1, OUTPUT);
  pinMode(MOTOR_RIN2, OUTPUT);
  pinMode(MOTOR_LIN1, OUTPUT);
  pinMode(MOTOR_LIN2, OUTPUT);

  // SD init
  Serial.println(F("SD init"));
  if (!SD.begin(10)) {
    Serial.println(F("SD init failed"));
  }
  Serial.println(F("SD init done"));

  // ultrasonic init
  pinMode(ULTRASONIC_ECHO_PIN, INPUT);
  pinMode(ULTRASONIC_TRIG_PIN, OUTPUT);

  // NINE init
  Serial.println(F("NINE init"));
  NINE_Init();

  // CdS init
  pinMode(CDS_PIN, OUTPUT);
  
  // Camera init
  Serial.println(F("CAMERA init"));
  /*if(XBeeSerial.isListening()){
    Serial.println(F("xb is listening"));
  }else{
    Serial.println(F("xb is not listening"));
  }*/
  CAMERA_Init();

  // collect and save NINE_Mag data
  Serial.println(F("Collect and save NINE_Mag data"));
  if(SD.exists(F("nine.txt"))){
    SD.remove(F("nine.txt"));
    Serial.println(F("removed nine.txt"));
  }
  File nineFile = SD.open(F("nine.txt"), FILE_WRITE);
  if (nineFile) {
    nineFile.print(F("nine_mag = ["));
    MOTOR_R_FORWARD();
    MOTOR_L_BACKWARD();
    for(int i = 0;i < 200;i++){
      NINE_Mag();
      nineFile.print(F("["));
      nineFile.print(NINE_xMag());
      nineFile.print(F(", "));
      nineFile.print(NINE_yMag());
      nineFile.print(F(", "));
      nineFile.print(NINE_zMag());
      nineFile.println(F("],"));
      delay(100);
    }
    MOTOR_R_STOP();
    MOTOR_L_STOP();
    nineFile.println(F("]"));
    nineFile.close();
    Serial.println(F("writing to nine.txt success"));
  }
  else {
    Serial.println(F("error opening nine.txt"));
  }
  updated = false;
  while(!updated){
    while (Serial.available() > 0) {
      char c = Serial.read();
      gps.encode(c);
      if (gps.location.isUpdated()) {
        month = gps.date.month();
        day = gps.date.day();
        hour = gps.time.hour();
        minute = gps.time.minute();
        logFileName = String(month*1000000+day*10000+hour*100+minute);
        Serial.println(logFileName);
        updated = true;
        break;
      }
    }
  }

  if(SD.exists(logFileName)){
    SD.remove(logFileName);
    Serial.println(F("removed logfile"));
  }
  logFile = SD.open(logFileName, FILE_WRITE);
  if (logFile) {
    Serial.println(F("opened logfile"));
    //logFile.close();
    //SLOGFLN("SD success");
  }
  else {
    Serial.println(F("error opening log.txt"));
  }
}

void loop() {
  SLOGFLN("----------------------------------------");
  switch (phase) {
    case 0:
      // Heat nichrome wire
      SLOGFLN("Start heating nichrome");
      digitalWrite(NICHROME_PIN, HIGH);
      delay(1000);
      digitalWrite(NICHROME_PIN, LOW);
      SLOGFLN("End heating nichrome");
    case 3:
      SLOGFLN("PHASE 3");
      SLOGFLN("Get GPS data");
      updated = false;
      while(!updated){
        while (Serial.available() > 0) {
          char c = Serial.read();
          gps.encode(c);
          if (gps.location.isUpdated()) {
            lat = gps.location.lat();
            lng = gps.location.lng();
            alt = gps.altitude.meters();
            year = gps.date.year();
            month = gps.date.month();
            day = gps.date.day();
            hour = gps.time.hour();
            minute = gps.time.minute();
            second = gps.time.second();
            SLOGF("LAT=");SLOGLN(lat);
            SLOGF("LONG=");SLOGLN(lng);
            SLOGF("ALT=");SLOGLN(alt);
            SLOGF("YEAR=");SLOGLN(year);
            SLOGF("MONTH=");SLOGLN(month);
            SLOGF("DAY=");SLOGLN(day);
            SLOGF("HOUR=");SLOGLN(hour);
            SLOGF("MINUTE=");SLOGLN(minute);
            SLOGF("SECOND=");SLOGLN(second);
            updated = true;
            break;
          }
        }
      }
      float latDiff = (TARGET_LAT - lat) * M_PER_LAT;
      float lngDiff = (TARGET_LNG - lng) * M_PER_LNG;
      float distance = sqrt(latDiff * latDiff + lngDiff * lngDiff);
      float targetRad = fmod(atan2(latDiff, lngDiff) + 2.0*PI, 2.0*PI);
      SLOGF("target degree: ");SLOGLN(targetRad * 180.0 / PI);

      float cansatRad, leftRad, rightRad;
      for(int i = 0;i < ROTATE_REPS;i++){
        SLOGF("rotate rep: ");
        SLOGLN(i);
        SLOGFLN("Get Mag data");
        NINE_Mag();
        cansatRad = fmod(3.0 * PI / 2.0 + MAG_NORTH - atan2(NINE_yMag(), NINE_xMag()) + 2.0*PI, 2.0*PI);
        SLOGF("cansat degree: ");SLOGLN(cansatRad * 180.0 / PI);

        leftRad = fmod(targetRad - cansatRad + 2.0*PI, 2.0*PI);
        rightRad = fmod(cansatRad - targetRad + 2.0*PI, 2.0*PI);
        SLOGF("left degree: ");SLOGLN(leftRad * 180.0 / PI);
        SLOGF("right degree: ");SLOGLN(rightRad * 180.0 / PI);
        
        if(leftRad < rightRad){
          SLOGF("turn left ");
          SLOGLN(1000*leftRad/RAD_PER_S_L);
          MOTOR_R_FORWARD();
          MOTOR_L_BACKWARD();
          delay(1000*leftRad/RAD_PER_S_L);
          MOTOR_R_STOP();
          MOTOR_L_STOP();
        }else{
          SLOGF("turn right ");
          SLOGLN(1000*rightRad/RAD_PER_S_R);
          MOTOR_L_FORWARD();
          MOTOR_R_BACKWARD();
          delay(1000*rightRad/RAD_PER_S_R);
          MOTOR_L_STOP();
          MOTOR_R_STOP();
        }
      }
      NINE_Mag();
      cansatRad = fmod(3.0 * PI / 2.0 + MAG_NORTH - atan2(NINE_yMag(), NINE_xMag()) + 2.0*PI, 2.0*PI);
      SLOGF("final cansat degree: ");SLOGLN(cansatRad * 180.0 / PI);

      leftRad = fmod(targetRad - cansatRad + 2.0*PI, 2.0*PI);
      rightRad = fmod(cansatRad - targetRad + 2.0*PI, 2.0*PI);
      SLOGF("final left degree: ");SLOGLN(leftRad * 180.0 / PI);
      SLOGF("final right degree: ");SLOGLN(rightRad * 180.0 / PI);

      if(leftRad > ROTATE_RAD_THRESHOLD && rightRad > ROTATE_RAD_THRESHOLD){
        SLOGFLN("stucked");
      }

      SLOGF("move forward ");
      SLOGLN(1000*distance/M_PER_S);
      MOTOR_R_FORWARD();
      MOTOR_L_FORWARD();
      //delay(1000*distance/M_PER_S);
      delay(10000);
      MOTOR_R_STOP();
      MOTOR_L_STOP();
      break;
    default:
      break;
  }
}
