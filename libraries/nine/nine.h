#ifndef NINE_h
#define NINE_h

#define Addr_Accl 0x19
#define Addr_Gyro 0x69
#define Addr_Mag 0x13

void NINE_Init(void);

void NINE_Accl(void);

void NINE_Gyro(void);

void NINE_Mag(void);

float NINE_xAccl(void);
float NINE_yAccl(void);
float NINE_zAccl(void);
float NINE_xGyro(void);
float NINE_yGyro(void);
float NINE_zGyro(void);
int NINE_xMag(void);
int NINE_yMag(void);
int NINE_zMag(void);

#endif