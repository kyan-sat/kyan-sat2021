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

#define SLOGFLN(X) {Serial.println(F((X)));logFile.println(F((X)));logFile.flush();}
#define SLOGF(X) {Serial.print(F((X)));logFile.print(F((X)));}
#define SLOGLN(X) {Serial.println((X));logFile.println((X));logFile.flush();}
#define SLOG(X) {Serial.print((X));logFile.print((X));}

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
#define RAD_PER_S_R 0.812
#define RAD_PER_S_L 0.778
#define M_PER_S 0.0625

// control constants
#define MAGCOLLECT_ROTATE_RAD (PI * 2.0 * 2.0)

#define PHASE0_WAITING_SECONDS 600

#define CDS_RELEASE_HLIM 600
#define CDS_RELEASE_REQREPS 10
#define ACCL_RELEASE_LLIM 0.00001
#define ACCL_RELEASE_HLIM 5.0
#define ACCL_RELEASE_REQREPS 10

#define PHASE2_WAITING_SECONDS 60
#define ACCL_DIFF_HLIM 0.2
#define ACCL_LANDING_REQREPS 10

#define HEATING_SECONDS 10

#define ROTATE_REPS 3
#define ROTATE_RAD_THRESHOLD (PI * 30.0 / 180.0)
#define GOAL_DISTANCE 1.0

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
  Serial.println(F("SD init..."));
  if (!SD.begin(10)) {
    Serial.println(F("failed"));
  }
  Serial.println(F("done"));

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
  Serial.println(F("collect mag"));
  String nineFileName = F("nine.txt");
  if(SD.exists(nineFileName)){
    SD.remove(nineFileName);
    Serial.print(F("removed "));
    Serial.println(nineFileName);
  }
  File nineFile = SD.open(nineFileName, FILE_WRITE);
  if (nineFile) {
    nineFile.print(F("nine_mag = ["));
    MOTOR_R_FORWARD();
    MOTOR_L_BACKWARD();
    delay(3000UL);
    int forReps = MAGCOLLECT_ROTATE_RAD / RAD_PER_S_L * 1000 / 100;
    for(int i = 0;i < forReps;i++){
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
    Serial.println(F("mag success"));
  }
  else {
    Serial.print(F("can't open"));
    Serial.println(nineFileName);
  }
  
  Serial.println(F("get log name"));
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
  }
  else {
    Serial.println(F("can't open logfile"));
  }
}

void loop() {
  switch (phase) {
    case 0: {
      SLOGFLN("PHASE 0");
      SLOGFLN("waiting");
      delay(PHASE0_WAITING_SECONDS*1000UL);
      phase = 1;
      break;
    }
    case 1: {
      SLOGFLN("PHASE 1");
      int rawCds;
      float absoluteAccl;
      char cdsStreak = 0;
      char acclStreak = 0;
      while(1){
        rawCds = analogRead(CDS_PIN);
        SLOGF("CdS: ");
        SLOGLN(rawCds);
        if(rawCds <= CDS_RELEASE_HLIM){
          cdsStreak++;
        }else{
          cdsStreak = 0;
        }
        NINE_Accl();
        absoluteAccl = sqrt(NINE_xAccl()*NINE_xAccl()+NINE_yAccl()*NINE_yAccl()+NINE_zAccl()*NINE_zAccl());
        SLOGF("accl: ");
        SLOGLN(absoluteAccl);
        if(ACCL_RELEASE_LLIM <= absoluteAccl && absoluteAccl <= ACCL_RELEASE_HLIM){
          acclStreak++;
        }else{
          acclStreak = 0;
        }
        if(cdsStreak >= CDS_RELEASE_REQREPS){
          SLOGFLN("RELEASED: cds satisfied");
          phase = 2;
          break;
        }
        if(acclStreak >= ACCL_RELEASE_REQREPS){
          SLOGFLN("RELEASED: accl satisfied");
          phase = 2;
          break;
        }
        delay(50);
      }
      break;
    }
    case 2: {
      SLOGFLN("PHASE 2");
      unsigned long startTime = millis();
      float absoluteAccl = 0, absoluteAcclPrev = 10;
      char acclStreak = 0;
      while(1){
        if(millis() - startTime >= PHASE2_WAITING_SECONDS * 1000UL){
          SLOGFLN("LANDED: time exceeded");
          phase = 3;
          break;
        }
        NINE_Accl();
        absoluteAccl = sqrt(NINE_xAccl()*NINE_xAccl()+NINE_yAccl()*NINE_yAccl()+NINE_zAccl()*NINE_zAccl());
        SLOGF("accl: ");
        SLOGLN(absoluteAccl);
        if(abs(absoluteAccl - absoluteAcclPrev) <= ACCL_DIFF_HLIM){
          acclStreak++;
        }else{
          acclStreak = 0;
        }
        if(acclStreak >= ACCL_LANDING_REQREPS){
          SLOGFLN("LANDED: accl satisfied");
          phase = 3;
          break;
        }
        delay(50);
      }
      break;
    }
    case 3: {
      SLOGFLN("PHASE 3");
      // Heat nichrome wire
      SLOGFLN("heat nichrome");
      digitalWrite(NICHROME_PIN, HIGH);
      delay(HEATING_SECONDS * 1000UL);
      digitalWrite(NICHROME_PIN, LOW);
      SLOGFLN("finished");
      phase = 4;
      break;
    }
    case 4: {
      SLOGFLN("PHASE 4");
      char moveCount = 0;
      while(1){
        SLOGF("moveCount: ");
        SLOGLN(moveCount);
        SLOGFLN("get GPS data");
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
              SLOGF("Y=");SLOGLN(year);
              SLOGF("M=");SLOGLN(month);
              SLOGF("D=");SLOGLN(day);
              SLOGF("H=");SLOGLN(hour);
              SLOGF("M=");SLOGLN(minute);
              SLOGF("S=");SLOGLN(second);
              updated = true;
              break;
            }
          }
        }
        float latDiff = (TARGET_LAT - lat) * M_PER_LAT;
        float lngDiff = (TARGET_LNG - lng) * M_PER_LNG;
        float distance = sqrt(latDiff * latDiff + lngDiff * lngDiff);
        SLOGF("dist: ");SLOGLN(distance);
        float targetRad = fmod(atan2(latDiff, lngDiff) + 2.0*PI, 2.0*PI);
        SLOGF("target deg: ");SLOGLN(targetRad * 180.0 / PI);
        
        if(distance <= GOAL_DISTANCE){
          SLOGFLN("REACHED GOAL");
          phase = 10;
          break;
        }
        
        float cansatRad, leftRad, rightRad;
        for(int i = 0;i < ROTATE_REPS;i++){
          SLOGF("rotate rep: ");
          SLOGLN(i);
          SLOGFLN("get mag data");
          NINE_Mag();
          cansatRad = fmod(3.0 * PI / 2.0 + MAG_NORTH - atan2(NINE_yMag(), NINE_xMag()) + 2.0*PI, 2.0*PI);
          SLOGF("cansat deg: ");SLOGLN(cansatRad * 180.0 / PI);

          leftRad = fmod(targetRad - cansatRad + 2.0*PI, 2.0*PI);
          rightRad = fmod(cansatRad - targetRad + 2.0*PI, 2.0*PI);
          SLOGF("left deg: ");SLOGLN(leftRad * 180.0 / PI);
          SLOGF("right deg: ");SLOGLN(rightRad * 180.0 / PI);
          
          if(leftRad < rightRad){
            SLOGF("turn L ");
            SLOGLN(1000*leftRad/RAD_PER_S_L);
            MOTOR_R_FORWARD();
            MOTOR_L_BACKWARD();
            delay(1000*leftRad/RAD_PER_S_L);
            MOTOR_R_STOP();
            MOTOR_L_STOP();
          }else{
            SLOGF("turn R ");
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
        SLOGF("final cansat deg: ");SLOGLN(cansatRad * 180.0 / PI);

        leftRad = fmod(targetRad - cansatRad + 2.0*PI, 2.0*PI);
        rightRad = fmod(cansatRad - targetRad + 2.0*PI, 2.0*PI);
        SLOGF("left deg: ");SLOGLN(leftRad * 180.0 / PI);
        SLOGF("right deg: ");SLOGLN(rightRad * 180.0 / PI);

        if(leftRad > ROTATE_RAD_THRESHOLD && rightRad > ROTATE_RAD_THRESHOLD){
          SLOGFLN("dir change failed");
        }

        SLOGF("move forward ");
        SLOGLN(1000*distance/M_PER_S);
        MOTOR_R_FORWARD();
        MOTOR_L_FORWARD();
        //delay(1000*distance/M_PER_S);
        delay(10000UL);
        MOTOR_R_STOP();
        MOTOR_L_STOP();
        moveCount++;
      }
      break;
    }
    default: {
      break;
    }
  }
}
