#ifndef CAMERA_h
#define CAMERA_h

#include <Arduino.h>

#define CAMERA_RX_PIN  A2
#define CAMERA_TX_PIN  A3

#define PIC_PKT_LEN    128                  //data length of each read, dont set this too big because ram is limited
#define PIC_FMT_VGA    7
#define PIC_FMT_CIF    5
#define PIC_FMT_OCIF   3
#define CAM_ADDR       0

#define PIC_FMT        PIC_FMT_VGA

void CAMERA_Init(void);

void clearRxBuf(void);

void sendCmd(char cmd[], int cmd_len);

void initialize(void);

void preCapture(void);

void Capture(void);

void GetData(String);

#endif