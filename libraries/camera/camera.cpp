#include "camera.h"

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <SoftwareSerial.h>

File myFile;

const byte cameraAddr = (CAM_ADDR << 5);
unsigned long picTotalLen = 0;

SoftwareSerial CameraSerial(CAMERA_RX_PIN, CAMERA_TX_PIN);

void CAMERA_Init(void) {
  CameraSerial.begin(115200);
  delay(300);
  CameraSerial.listen();
  initialize();
}

void clearRxBuf() {
  while (CameraSerial.available()) {
    CameraSerial.read();
  }
}

void sendCmd(char cmd[], int cmd_len) {
  for (int i = 0; i < cmd_len; i++) CameraSerial.print(cmd[i]);
}

void initialize() {/*
  if(CameraSerial.listen()){
    Serial.println("listen serial changed");
  }else{
    Serial.println("listen serial not changed");
  }*/
  char cmd[] = {0xaa, 0x0d | cameraAddr, 0x00, 0x00, 0x00, 0x00};
  unsigned char resp[6];
  /*
  if(CameraSerial.isListening()){
    Serial.println("CameraSerial is listening");
  }else{
    Serial.println("CameraSerial is not listening");
  }*/

  CameraSerial.setTimeout(500);
  while (1)
  {    
    //Serial.println(F("."));
    clearRxBuf();
    sendCmd(cmd,6);
    if (CameraSerial.readBytes((char *)resp, 6) != 6)
    {
      continue;
    }
    if (resp[0] == 0xaa && resp[1] == (0x0e | cameraAddr) && resp[2] == 0x0d && resp[4] == 0 && resp[5] == 0) //ACK
    {
      if (CameraSerial.readBytes((char *)resp, 6) != 6) continue;
      if (resp[0] == 0xaa && resp[1] == (0x0d | cameraAddr) && resp[2] == 0 && resp[3] == 0 && resp[4] == 0 && resp[5] == 0) break; //SYNC
    }
  }
  cmd[1] = 0x0e | cameraAddr;
  cmd[2] = 0x0d;
  sendCmd(cmd, 6); //ACK
  //Serial.println(F("\nCamera initialization done."));
}

void preCapture() {
  //CameraSerial.listen();
  char cmd[] = {0xaa, 0x01 | cameraAddr, 0x00, 0x07, 0x00, PIC_FMT};
  unsigned char resp[6];

  CameraSerial.setTimeout(100);
  while (1) {
    clearRxBuf();
    sendCmd(cmd, 6);
    if (CameraSerial.readBytes((char *)resp, 6) != 6) continue;
    if (resp[0] == 0xaa && resp[1] == (0x0e | cameraAddr) && resp[2] == 0x01 &&
        resp[4] == 0 && resp[5] == 0)
      break;
  }
}

void Capture() {
  //CameraSerial.listen();
  char cmd[] = {0xaa,
                0x06 | cameraAddr,
                0x08,
                PIC_PKT_LEN & 0xff,
                (PIC_PKT_LEN >> 8) & 0xff,
                0};
  unsigned char resp[6];

  while (1) {
    clearRxBuf();
    sendCmd(cmd, 6);
    if (CameraSerial.readBytes((char *)resp, 6) != 6) continue;
    if (resp[0] == 0xaa && resp[1] == (0x0e | cameraAddr) && resp[2] == 0x06 &&
        resp[4] == 0 && resp[5] == 0)
      break;
  }
  cmd[1] = 0x05 | cameraAddr;
  cmd[2] = 0;
  cmd[3] = 0;
  cmd[4] = 0;
  cmd[5] = 0;
  while (1) {
    clearRxBuf();
    sendCmd(cmd, 6);
    if (CameraSerial.readBytes((char *)resp, 6) != 6) continue;
    if (resp[0] == 0xaa && resp[1] == (0x0e | cameraAddr) && resp[2] == 0x05 &&
        resp[4] == 0 && resp[5] == 0)
      break;
  }
  cmd[1] = 0x04 | cameraAddr;
  cmd[2] = 0x1;
  CameraSerial.setTimeout(1000);
  while (1) {
    clearRxBuf();
    sendCmd(cmd, 6);
    if (CameraSerial.readBytes((char *)resp, 6) != 6) continue;
    if (resp[0] == 0xaa && resp[1] == (0x0e | cameraAddr) && resp[2] == 0x04 &&
        resp[4] == 0 && resp[5] == 0) {
      if (CameraSerial.readBytes((char *)resp, 6) != 6) {
        continue;
      }
      if (resp[0] == 0xaa && resp[1] == (0x0a | cameraAddr) &&
          resp[2] == 0x01) {
        picTotalLen = (resp[3]) | (resp[4] << 8) | (resp[5] << 16);
        //Serial.print(F("picTotalLen:"));
        //Serial.println(picTotalLen);
        break;
      }
    }
  }
}

void GetData(String picName) {
  //CameraSerial.listen();
  unsigned int pktCnt = (picTotalLen) / (PIC_PKT_LEN - 6);
  if ((picTotalLen % (PIC_PKT_LEN - 6)) != 0) pktCnt += 1;

  char cmd[] = {0xaa, 0x0e | cameraAddr, 0x00, 0x00, 0x00, 0x00};
  unsigned char pkt[PIC_PKT_LEN];

  if (SD.exists(picName)) {
    SD.remove(picName);
  }

  /*File test = SD.open("camtest.txt", FILE_WRITE);
  if (test) {
    Serial.println(F("opened camtest"));
    test.println("uoooo");
  }
  else {
    Serial.println(F("can't open camtest"));
  }
  test.close();*/
  myFile = SD.open(picName, FILE_WRITE);
  if (!myFile) {
    //Serial.println(F("myFile open failed"));
  } else {
    //Serial.println(F("myFile open success"));
    for (unsigned int i = 0; i < pktCnt; i++) {
      cmd[4] = i & 0xff;
      cmd[5] = (i >> 8) & 0xff;

      int retry_cnt = 0;
    retry:
      delay(10);
      clearRxBuf();
      sendCmd(cmd, 6);
      uint16_t cnt = CameraSerial.readBytes((char *)pkt, PIC_PKT_LEN);

      unsigned char sum = 0;
      for (int y = 0; y < cnt - 2; y++) {
        sum += pkt[y];
      }
      if (sum != pkt[cnt - 2]) {
        if (++retry_cnt < 100)
          goto retry;
        else
          break;
      }

      myFile.write((const uint8_t *)&pkt[4], cnt - 6);
      // if (cnt != PIC_PKT_LEN) break;
    }
    cmd[4] = 0xf0;
    cmd[5] = 0xf0;
    sendCmd(cmd, 6);
  }
  myFile.close();
}
