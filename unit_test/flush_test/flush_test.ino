#include <SPI.h>
#include <SD.h>

void setup() {
  pinMode(9, OUTPUT);
  digitalWrite(9, HIGH);
  Serial.begin(9600);
  while (!Serial) {
  }

  Serial.print("Initializing SD card...");

  if (!SD.begin(10)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");

  if(SD.exists("flush.txt")){
    SD.remove("flush.txt");
    Serial.println("removed flush.txt");
  }

  File flush = SD.open("flush.txt", FILE_WRITE);
  int count = 0;
  if (flush) {
    for(int i = 0;i < 100;i++){
      flush.println(count);
      count++;
      flush.flush();
      delay(100);
    }
    flush.close();
    Serial.println("closed");
  }
  else {
    Serial.println("error opening flush.txt");
  }

  Serial.println("done!");
}

void loop() {
}