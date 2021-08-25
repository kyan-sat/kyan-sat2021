#include <Arduino.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>

TinyGPSPlus gps;
//SoftwareSerial mySerial(A2, A3);  // arduinoの2とGPSのTXD, 3をRXDと接続

void setup() {
  pinMode(A2, INPUT);
  Serial.begin(9600);
  delay(300);
  //mySerial.begin(9600);
  //delay(300);
  //mySerial.listen();
}

void loop() {
  while (Serial.available() > 0) {
    char c = Serial.read();
    gps.encode(c);
    if (gps.location.isUpdated()) {
      Serial.print("LAT=");
      Serial.println(gps.location.lat(), 6);
      Serial.print("LONG=");
      Serial.println(gps.location.lng(), 6);
      Serial.print("ALT=");
      Serial.println(gps.altitude.meters());
      Serial.print("DATE=");
      Serial.println(gps.date.value());
      Serial.print("TIME=");
      Serial.println(gps.time.value());
    }
  }
}
