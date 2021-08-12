#include <SoftwareSerial.h>

SoftwareSerial mySerial(2, 3);

void setup() {
  Serial.begin(9600);
  delay(300);
  mySerial.begin(9600);
  delay(300);
  mySerial.listen();

}

int cou = 0;

void loop() {
  Serial.println(cou);
  mySerial.println(cou);
  delay(1000);
  cou += 1;
}
