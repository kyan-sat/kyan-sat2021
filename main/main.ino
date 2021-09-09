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
unsigned char updated;


#define XBEE_RX_PIN 2
#define XBEE_TX_PIN 3
//SoftwareSerial XBeeSerial(XBEE_RX_PIN, XBEE_TX_PIN);

#define NICHROME_PIN 4

#define MOTOR_RIN1 8
#define MOTOR_RIN2 7
#define MOTOR_LIN1 6
#define MOTOR_LIN2 5
void MOTOR_R_FORWARD(){analogWrite(MOTOR_RIN1, 255);analogWrite(MOTOR_RIN2, 0);}
void MOTOR_R_BACKWARD(){analogWrite(MOTOR_RIN1, 0);analogWrite(MOTOR_RIN2, 255);}
void MOTOR_R_STOP(){analogWrite(MOTOR_RIN1, 0);analogWrite(MOTOR_RIN2, 0);}
void MOTOR_L_FORWARD(){analogWrite(MOTOR_LIN1, 255);analogWrite(MOTOR_LIN2, 0);}
void MOTOR_L_BACKWARD(){analogWrite(MOTOR_LIN1, 0);analogWrite(MOTOR_LIN2, 255);}
void MOTOR_L_STOP(){analogWrite(MOTOR_LIN1, 0);analogWrite(MOTOR_LIN2, 0);}
void MOTOR_FORWARD(){MOTOR_R_FORWARD();MOTOR_L_FORWARD();}
void MOTOR_TURN_R(){MOTOR_R_BACKWARD();MOTOR_L_FORWARD();}
void MOTOR_TURN_L(){MOTOR_R_FORWARD();MOTOR_L_BACKWARD();}
void MOTOR_STOP(){MOTOR_R_STOP();MOTOR_L_STOP();}

#include <SD.h>
File logFile;
char logFileName[13];

template <typename T> void SLOGLN(T X) {Serial.println(X);logFile.println(X);logFile.flush();}
template <typename T> void SLOG(T X) {Serial.print(X);logFile.print(X);}
#define SLOGFLN(X) SLOGLN(F(X))
#define SLOGF(X) SLOG(F(X))

#define ULTRASONIC_ECHO_PIN A1
#define ULTRASONIC_TRIG_PIN A0
double duration;
double distance;

#include "nine.h"

#define CDS_PIN A6

#include "camera.h"
void takePicture(String s){
  logFile.close();
  preCapture();
  Capture();
  GetData(s);
  logFile = SD.open(logFileName, FILE_WRITE);
  if (logFile) {
    Serial.println(F("opened log"));
  }
  else {
    Serial.println(F("can't open log"));
  }
}

unsigned char phase = 3;

// constants
#define TARGET_LAT 35.714636087650646
#define TARGET_LNG 139.7620713176752
#define M_PER_LAT 111092.7384 // https://www.wingfield.gr.jp/archives/9721 lat43
#define M_PER_LNG 81540.4864
#define MAG_NORTH (PI * 10.0 / 180.0)
#define PI 3.14159265
#define RAD_PER_S_R 0.812
#define RAD_PER_S_L 0.778
#define M_PER_S 0.0625

// control constants
#define NINE_DEBUG_FLAG 0

#define MAGCOLLECT_ROTATE_RAD (PI * 2.0 * 2.0)
long xMagOffset, yMagOffset;

#define LOGFILE_REQREPS 3

#define PHASE0_WAITING_SECONDS 600

#define CDS_RELEASE_HLIM 600
#define CDS_RELEASE_REQREPS 10
#define ACCL_RELEASE_LLIM 0.00001
#define ACCL_RELEASE_HLIM 5.0
#define ACCL_RELEASE_REQREPS 10

#define PHASE2_WAITING_SECONDS 60
#define ACCL_DIFF_HLIM 0.2
#define ACCL_LANDING_REQREPS 10

#define HEATING_SECONDS 1.5

#define FORWARD_SECONDS_AFTER_LANDING 3.0
#define GPS_DISCARD_REPS 6
#define ROTATE_REPS 3
#define ROTATE_RAD_THRESHOLD (PI * 30.0 / 180.0)
#define GOAL_DISTANCE 1.0
#define GOAL_FOWARD_RATIO 0.8
#define FIX_FOWARD_FLAG 0

void setup() {
  pinMode(9, OUTPUT);
  digitalWrite(9, HIGH);
  
  // GPS & Serial init
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // XBee init
  //XBeeSerial.begin(9600);
  delay(300);
  //XBeeSerial.listen();

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
  if(NINE_DEBUG_FLAG){
    Serial.println(F("collect mag"));
    String nineFileName = F("nine.txt");
    if(SD.exists(nineFileName)){
      SD.remove(nineFileName);
      Serial.print(F("removed "));
      Serial.println(nineFileName);
    }
    File nineFile = SD.open(nineFileName, FILE_WRITE);
    if (nineFile) {
      nineFile.print("nine_mag = [");
      MOTOR_TURN_L();
      //delay(3000UL);
      int forReps = MAGCOLLECT_ROTATE_RAD / RAD_PER_S_L * 1000 / 100;

      long xMagSum = 0, yMagSum = 0;
      for(int i = 0;i < forReps;i++){
        NINE_Mag();
        nineFile.print("[");
        nineFile.print(NINE_xMag());
        nineFile.print(", ");
        nineFile.print(NINE_yMag());
        nineFile.print(", ");
        nineFile.print(NINE_zMag());
        nineFile.println("],");
        xMagSum += NINE_xMag();
        yMagSum += NINE_yMag();
        delay(100);
      }
      MOTOR_STOP();
      nineFile.println("]");
      nineFile.close();
      Serial.println("mag success");
      xMagOffset = xMagSum / forReps;
      yMagOffset = yMagSum / forReps;
    }
    else {
      Serial.print("can't open");
      Serial.println(nineFileName);
    }
  }
  Serial.println(F("log name"));
  updated = 0;
  while(updated < LOGFILE_REQREPS){
    while (Serial.available() > 0) {
      char c = Serial.read();
      gps.encode(c);
      if (gps.location.isUpdated()) {
        updated++;
        month = gps.date.month();
        day = gps.date.day();
        hour = gps.time.hour();
        minute = gps.time.minute();
        if(updated == LOGFILE_REQREPS){
          Serial.println(month);
          Serial.println(day);
          Serial.println(hour);
          Serial.println(minute);
          sprintf(logFileName, "%02d%02d%02d%02d.txt", month, day, hour, minute);
          Serial.println(logFileName);
          Serial.println(updated);
          break;
        }
      }
    }
  }

  if(SD.exists(logFileName)){
    SD.remove(logFileName);
    Serial.println(F("removed log"));
  }
  logFile = SD.open(logFileName, FILE_WRITE);
  if (logFile) {
    Serial.println(F("opened log"));
  }
  else {
    Serial.println(F("can't open log"));
  }
}

void loop() {
  switch (phase) {
    case 0: {
      SLOGFLN("PHASE 0");
      SLOGFLN("waiting");
      takePicture("a.jpg");
      delay(PHASE0_WAITING_SECONDS*1000UL);
      phase = 1;
      break;
    }
    case 1: {
      SLOGFLN("PHASE 1");
      takePicture("b.jpg");
      int rawCds;
      float absoluteAccl;
      char cdsStreak = 0;
      char acclStreak = 0;
      while(1){
        rawCds = analogRead(CDS_PIN);
        SLOGF("CdS ");
        SLOGLN(rawCds);
        if(rawCds <= CDS_RELEASE_HLIM){
          cdsStreak++;
        }else{
          cdsStreak = 0;
        }
        NINE_Accl();
        absoluteAccl = sqrt(NINE_xAccl()*NINE_xAccl()+NINE_yAccl()*NINE_yAccl()+NINE_zAccl()*NINE_zAccl());
        SLOGF("accl ");
        SLOGLN(absoluteAccl);
        if(ACCL_RELEASE_LLIM <= absoluteAccl && absoluteAccl <= ACCL_RELEASE_HLIM){
          acclStreak++;
        }else{
          acclStreak = 0;
        }
        if(cdsStreak >= CDS_RELEASE_REQREPS){
          SLOGFLN("RELEASED cds ok");
          phase = 2;
          break;
        }
        if(acclStreak >= ACCL_RELEASE_REQREPS){
          SLOGFLN("RELEASED accl ok");
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
          SLOGFLN("LANDED time exceeded");
          phase = 3;
          break;
        }
        NINE_Accl();
        absoluteAccl = sqrt(NINE_xAccl()*NINE_xAccl()+NINE_yAccl()*NINE_yAccl()+NINE_zAccl()*NINE_zAccl());
        SLOGF("accl ");
        SLOGLN(absoluteAccl);
        if(abs(absoluteAccl - absoluteAcclPrev) <= ACCL_DIFF_HLIM){
          acclStreak++;
        }else{
          acclStreak = 0;
        }
        if(acclStreak >= ACCL_LANDING_REQREPS){
          SLOGFLN("LANDED accl ok");
          phase = 3;
          break;
        }
        delay(50);
      }
      break;
    }
    case 3: {
      SLOGFLN("PHASE 3");
      takePicture(F("p3.jpg"));
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
      takePicture(F("p4.jpg"));
      SLOGFLN("escape");
      MOTOR_FORWARD();
      delay(FORWARD_SECONDS_AFTER_LANDING * 1000UL);
      MOTOR_STOP();
      delay(1000);
      
      SLOGFLN("magOffset");
      MOTOR_TURN_L();
      delay(100);
      int forReps = MAGCOLLECT_ROTATE_RAD / RAD_PER_S_L * 1000 / 100;

      long xMagSum = 0, yMagSum = 0;
      for(int i = 0;i < forReps;i++){
        NINE_Mag();
        SLOGF("[");
        SLOG(NINE_xMag());
        SLOGF(", ");
        SLOG(NINE_yMag());
        SLOGF(", ");
        SLOG(NINE_zMag());
        SLOGFLN("],");
        xMagSum += NINE_xMag();
        yMagSum += NINE_yMag();
        delay(100);
      }
      MOTOR_STOP();
      xMagOffset = xMagSum / forReps;
      yMagOffset = yMagSum / forReps;
      SLOGFLN("mag success");

      int moveCount = 0;
      while(1){
        SLOGF("moveCount ");
        SLOGLN(moveCount);
        char moveFileName[8];
        sprintf(moveFileName, "%02d.jpg", moveCount);
        takePicture(moveFileName);
        SLOGFLN("get GPS");
        updated = 0;
        while(updated < GPS_DISCARD_REPS){
          while (Serial.available() > 0) {
            char c = Serial.read();
            gps.encode(c);
            if (gps.location.isUpdated()) {
              updated++;
              lat = gps.location.lat();
              lng = gps.location.lng();
              alt = gps.altitude.meters();
              year = gps.date.year();
              month = gps.date.month();
              day = gps.date.day();
              hour = gps.time.hour();
              minute = gps.time.minute();
              second = gps.time.second();
              Serial.println(updated);
              Serial.print(lat, 8);logFile.print(lat, 8);
              SLOGF(", ");Serial.println(lng, 8);logFile.println(lng, 8);
              if(updated == GPS_DISCARD_REPS){
                Serial.print(lat, 8);logFile.print(lat, 8);
                SLOGF(", ");Serial.println(lng, 8);logFile.println(lng, 8);
                SLOGF("ALT=");SLOGLN(alt);
                SLOGF("Y=");SLOGLN(year);
                SLOGF("M=");SLOGLN(month);
                SLOGF("D=");SLOGLN(day);
                SLOGF("H=");SLOGLN(hour);
                SLOGF("M=");SLOGLN(minute);
                SLOGF("S=");SLOGLN(second);
                break;
              }
            }
          }
        }
        float latDiff = (TARGET_LAT - lat) * M_PER_LAT;
        float lngDiff = (TARGET_LNG - lng) * M_PER_LNG;
        float distance = sqrt(latDiff * latDiff + lngDiff * lngDiff);
        SLOGF("dist ");SLOGLN(distance);
        float targetRad = fmod(atan2(latDiff, lngDiff) + 2.0*PI, 2.0*PI);
        SLOGF("T-deg ");SLOGLN(targetRad * 180.0 / PI);
        
        if(distance <= GOAL_DISTANCE){
          takePicture(F("g.jpg"));
          SLOGFLN("REACHED GOAL");
          phase = 10;
          break;
        }
        
        float cansatRad, leftRad, rightRad;
        for(int i = 0;i < ROTATE_REPS;i++){
          SLOGF("rotate rep ");
          SLOGLN(i);
          SLOGFLN("get mag");
          NINE_Mag();
          cansatRad = fmod(3.0 * PI / 2.0 + MAG_NORTH - atan2(NINE_yMag() - yMagOffset, NINE_xMag() - xMagOffset) + 2.0*PI, 2.0*PI);
          SLOGF("C-deg ");SLOGLN(cansatRad * 180.0 / PI);

          leftRad = fmod(targetRad - cansatRad + 2.0*PI, 2.0*PI);
          rightRad = fmod(cansatRad - targetRad + 2.0*PI, 2.0*PI);
          SLOGF("Ldeg ");SLOGLN(leftRad * 180.0 / PI);
          SLOGF("Rdeg ");SLOGLN(rightRad * 180.0 / PI);
          
          if(leftRad < rightRad){
            SLOGF("turn L ");
            SLOGLN(1000*leftRad/RAD_PER_S_L);
            MOTOR_TURN_L();
            delay(1000*leftRad/RAD_PER_S_L);
            MOTOR_STOP();
          }else{
            SLOGF("turn R ");
            SLOGLN(1000*rightRad/RAD_PER_S_R);
            MOTOR_TURN_R();
            delay(1000*rightRad/RAD_PER_S_R);
            MOTOR_STOP();
          }
        }
        NINE_Mag();
        cansatRad = fmod(3.0 * PI / 2.0 + MAG_NORTH - atan2(NINE_yMag() - yMagOffset, NINE_xMag() - xMagOffset) + 2.0*PI, 2.0*PI);
        SLOGF("final C-deg ");SLOGLN(cansatRad * 180.0 / PI);

        leftRad = fmod(targetRad - cansatRad + 2.0*PI, 2.0*PI);
        rightRad = fmod(cansatRad - targetRad + 2.0*PI, 2.0*PI);
        SLOGF("Ldeg ");SLOGLN(leftRad * 180.0 / PI);
        SLOGF("Rdeg ");SLOGLN(rightRad * 180.0 / PI);

        if(leftRad > ROTATE_RAD_THRESHOLD && rightRad > ROTATE_RAD_THRESHOLD){
          SLOGFLN("dir change failed");
        }else{
          SLOGFLN("dir change success");
        }

        SLOGF("move forward ");
        MOTOR_FORWARD();
        if(FIX_FOWARD_FLAG){
          SLOGLN(10000);
          delay(10000);
        }else{
          SLOGLN(1000 * distance * GOAL_FOWARD_RATIO / M_PER_S);
          delay(1000 * distance * GOAL_FOWARD_RATIO / M_PER_S);
        }
        MOTOR_STOP();
        moveCount++;
      }
      break;
    }
    default: {
      break;
    }
  }
}
