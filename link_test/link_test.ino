#include <SPI.h>
#include <SD.h>
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
#define SLOGFLN(X) {Serial.println(F((X)));XBeeSerial.println(F((X)));}
#define SLOGF(X) {Serial.print(F((X)));XBeeSerial.print(F((X)));}
#define SLOGLN(X) {Serial.println((X));XBeeSerial.println((X));}
#define SLOG(X) {Serial.print((X));XBeeSerial.print((X));}

#define NICHROME_PIN 4

#define MOTOR_RIN1 8
#define MOTOR_RIN2 7
#define MOTOR_LIN1 6
#define MOTOR_LIN2 5

#define ULTRASONIC_ECHO_PIN A1
#define ULTRASONIC_TRIG_PIN A0
double duration;
double distance;

#include "nine.h"

#define CDS_PIN A6
String picName = "link";
unsigned char picCount = 0;

#include "camera.h"

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
  Serial.println("SD init");
  if (!SD.begin(10)) {
    Serial.println("SD init failed");
  }
  Serial.println("SD init done");

  // ultrasonic init
  pinMode(ULTRASONIC_ECHO_PIN, INPUT);
  pinMode(ULTRASONIC_TRIG_PIN, OUTPUT);

  // NINE init
  Serial.println("NINE init");
  NINE_Init();

  // CdS init
  pinMode(CDS_PIN, OUTPUT);
  
  // Camera init
  Serial.println("CAMERA init");
  /*if(XBeeSerial.isListening()){
    Serial.println("xb is listening");
  }else{
    Serial.println("xb is not listening");
  }*/
  CAMERA_Init();
}

void loop() {
  String dataString = "";
  dataString += F("-----\n");

  // Get GPS data
  SLOGFLN("GPS");/*
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
        Serial.print("LAT=");
        Serial.println(lat, 6);
        Serial.print("LONG=");
        Serial.println(lng, 6);
        Serial.print("ALT=");
        Serial.println(alt);
        Serial.print("YEAR=");
        Serial.println(year);
        Serial.print("MONTH=");
        Serial.println(month);
        Serial.print("DAY=");
        Serial.println(day);
        Serial.print("HOUR=");
        Serial.println(hour);
        Serial.print("MINUTE=");
        Serial.println(minute);
        Serial.print("SECOND=");
        Serial.println(second);
        dataString += "LAT="+String(lat);
        dataString += "LONG="+String(lng);
        dataString += "ALT="+String(alt);
        dataString += "YEAR="+String(year);
        dataString += "MONTH="+String(month);
        dataString += "DAY="+String(day);
        dataString += "HOUR="+String(hour);
        dataString += "MINUTE="+String(minute);
        dataString += "SECOND="+String(second)+"\n";
        updated = true;
        break;
      }
    }
  }*/

  // Get air pressure data

  // Get ultrasonic data
  SLOGFLN("ULTRASONIC");
  for(int i = 0;i < 5;i++){
    digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(ULTRASONIC_TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(ULTRASONIC_TRIG_PIN, LOW);

    duration = pulseIn(ULTRASONIC_ECHO_PIN, HIGH);
    if (duration > 0) {
      duration /= 2;
      distance = duration * 340 * 100 / 1000000;
      SLOGF("dis:");
      SLOG(distance);
      SLOGFLN(" cm");
      dataString += String(distance);dataString += " cm\n";
    }
    delay(500);
  }

  // Get NINE data
  SLOGFLN("NINE");
  //NINE_Accl();
  //NINE_Gyro();
  NINE_Mag();
  dataString += "NINE\n";
  /*dataString += String(NINE_xAccl());dataString += ",";
  dataString += String(NINE_yAccl());dataString += ",";
  dataString += String(NINE_zAccl());dataString += "\n";
  dataString += String(NINE_xGyro());dataString += ",";
  dataString += String(NINE_yGyro());dataString += ",";
  dataString += String(NINE_zGyro());dataString += "\n";*/
  dataString += String(NINE_xMag());dataString += ",";
  dataString += String(NINE_yMag());dataString += ",";
  dataString += String(NINE_zMag());dataString += "\n";

  // Get CdS data
  SLOGFLN("CdS");
  dataString += "CdS\n";
  for(int i = 0;i < 5;i++){
    dataString += String(analogRead(CDS_PIN));dataString += "\n";
    delay(10);
  }

  // Take a picture and save it
  SLOGFLN("Camera");
  delay(200);
  SLOGFLN("preCapture");
  preCapture();
  SLOGFLN("Capture");
  Capture();
  SLOGFLN("GetData");
  GetData(picName + String(picCount) + ".jpg");
  SLOGFLN("Camera success");
  SLOGLN(picName + String(picCount) + ".jpg");
  picCount++;

  // Write to file
  SLOGFLN("SD");
  File dataFile = SD.open("link.txt", FILE_WRITE);
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    SLOGFLN("SD success");
  }
  else {
    SLOGFLN("error opening link.txt");
  }

  // Drive motor
  SLOGFLN("R+ L+");
  analogWrite(MOTOR_RIN1, 255);
  analogWrite(MOTOR_RIN2, 0);
  analogWrite(MOTOR_LIN1, 255);
  analogWrite(MOTOR_LIN2, 0);
  delay(5000);
  SLOGFLN("R- L-");
  analogWrite(MOTOR_RIN1, 0);
  analogWrite(MOTOR_RIN2, 255);
  analogWrite(MOTOR_LIN1, 0);
  analogWrite(MOTOR_LIN2, 255);
  delay(5000);
  SLOGFLN("R+ L-");
  analogWrite(MOTOR_RIN1, 255);
  analogWrite(MOTOR_RIN2, 0);
  analogWrite(MOTOR_LIN1, 0);
  analogWrite(MOTOR_LIN2, 255);
  delay(5000);
  SLOGFLN("R- L+");
  analogWrite(MOTOR_RIN1, 0);
  analogWrite(MOTOR_RIN2, 255);
  analogWrite(MOTOR_LIN1, 255);
  analogWrite(MOTOR_LIN2, 0);
  delay(5000);

  // Heat nichrome wire
  SLOGFLN("Start heating nichrome");
  digitalWrite(NICHROME_PIN, HIGH);
  delay(1000);
  digitalWrite(NICHROME_PIN, LOW);
  SLOGFLN("End heating nichrome");

}
