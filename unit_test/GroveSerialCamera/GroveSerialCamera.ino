//ほぼほぼメーカのサンプルコード
//  File SerialCamera_DemoCode_CJ-OV528.ino
//  8/8/2013 Jack Shao
//  Demo code for using seeeduino or Arduino board to cature jpg format
//  picture from seeed serial camera and save it into sd card. Push the
//  button to take the a picture .
//  For more details about the product please check http://www.seeedstudio.com/depot/

#include <SPI.h>
#include <Arduino.h>
#include <SoftwareSerial.h>
#include <SD.h>

#define PIC_PKT_LEN    128                  //data length of each read, dont set this too big because ram is limited
#define PIC_FMT_VGA    7
#define PIC_FMT_CIF    5
#define PIC_FMT_OCIF   3
#define CAM_ADDR       0 //同じメーカで使用するカメラが違うときに変更する？　
#define CAM_SERIAL     Serial

#define PIC_FMT        PIC_FMT_VGA

File myFile;

const byte cameraAddr = (CAM_ADDR << 5);  // addr 5bit 左シフト
unsigned long picTotalLen = 0;            // picture length
int picNameNum = 0;

SoftwareSerial mySerial(A2, A3);

/*********************************************************************/
void setup()
{   //SD カード初期化いじる必要なし
    Serial.begin(9600);
    delay(300);
    mySerial.begin(115200);
    delay(300);
    mySerial.listen();
    
    Serial.println("Initializing SD card....");
    pinMode(10,OUTPUT);          // CS pin of SD Card Shield

    if (!SD.begin(10)) //10pin をスレーブセレクトピンとして利用
    {
        Serial.print("sd init failed");
        return;
    }
    Serial.println("sd init done.");
    //SD カード初期化いじる必要なし
    initialize(); //カメラモジュール初期化　本スケッチ内で定義された関数
    Serial.println("Camera initialize done.");
    
}
/*********************************************************************/
void loop()
{
    /*
    int n = 0;
    while(1){
        Serial.println("Press the button to take a picture");
        while (digitalRead(buttonPin) == LOW);      //wait for buttonPin status to HIGH
        if(digitalRead(buttonPin) == HIGH){         //撮影入力ピンはプルダウンしておかないと撮影しっぱなし
            delay(50);                               //Debounce
            if (digitalRead(buttonPin) == HIGH)
            {
                Serial.println("begin to take picture");
                delay(200);
                if (n == 0) preCapture();
                Capture();
                GetData();
            }
            Serial.print("Taking pictures success ,number : ");
            Serial.println(n);
            n++ ;
        }
    }*/
    Serial.println("begin to take picture");
    delay(200);
    preCapture();
    Capture();
    GetData();
    Serial.println("Taking pictures success");
    while(1);
}
/*********************************************************************/
void clearRxBuf() //前回の残りデータを受け取って何もしない（初期化する）
{   
    while (mySerial.available())
    {
        mySerial.read();
    }
}
/*********************************************************************/
void sendCmd(char cmd[], int cmd_len) //カメラモジュールにコマンドを送る
{
    //for (char i = 0; i < cmd_len; i++) Serial.print(cmd[i]);
    //ここのprint 関数はシリアル通信でデータを送るために使っている、シリアルモニタへの表示は副次的なもの
    //Serial.print は2番目引数を与えなければ暗黙のうちにASCIIをバイナリに変換してくれる。BINは2進数変換でデータとしては異なるようです
    //この性質を利用してchar配列に保存したバイナリデータ（文字的には意味がない）を送ることが出来る
    for (int i = 0; i < cmd_len; i++) mySerial.print(cmd[i]); //Kim 書き換え,配列の参照にchar使うのは違和感 
}
/*********************************************************************/
void initialize()
{   //マニュアルで言うところの 2.Make connection with camera
    char cmd[] = {0xaa,0x0d|cameraAddr,0x00,0x00,0x00,0x00} ; //16進数をcharに格納、この時文字として呼び出すと意味のない文字（文字化け文字になっている）
    unsigned char resp[6];

    mySerial.setTimeout(500); //シリアルデータ受信最大待ち時間　ミリ秒
    while (1)
    {
        clearRxBuf();
        sendCmd(cmd,6); //Arduino からモジュールへ初期化信号を送る
        if (mySerial.readBytes((char *)resp, 6) != 6) //モジュールからの信号が6byteでないときはやり直し
        {
            continue;
        }
        if (resp[0] == 0xaa && resp[1] == (0x0e | cameraAddr) && resp[2] == 0x0d && resp[4] == 0 && resp[5] == 0) //ACK
        {
            if (mySerial.readBytes((char *)resp, 6) != 6) continue;
            if (resp[0] == 0xaa && resp[1] == (0x0d | cameraAddr) && resp[2] == 0 && resp[3] == 0 && resp[4] == 0 && resp[5] == 0) break; //SYNC
        }
    }
    cmd[1] = 0x0e | cameraAddr;
    cmd[2] = 0x0d;
    sendCmd(cmd, 6); //ACK
    Serial.println("\nCamera initialization done.");
}
/*********************************************************************/
void preCapture() //4.1 JPEG Snapshot Picture
{
    char cmd[] = { 0xaa, 0x01 | cameraAddr, 0x00, 0x07, 0x00, PIC_FMT };
    unsigned char resp[6];

    mySerial.setTimeout(100);
    while (1)
    {
        clearRxBuf();
        sendCmd(cmd, 6);
        if (mySerial.readBytes((char *)resp, 6) != 6) continue;
        if (resp[0] == 0xaa && resp[1] == (0x0e | cameraAddr) && resp[2] == 0x01 && resp[4] == 0 && resp[5] == 0) break;
    }
}
void Capture()
{
    char cmd[] = { 0xaa, 0x06 | cameraAddr, 0x08, PIC_PKT_LEN & 0xff, (PIC_PKT_LEN>>8) & 0xff ,0};
    unsigned char resp[6];

    mySerial.setTimeout(100);
    while (1)
    {
        clearRxBuf();
        sendCmd(cmd, 6);
        if (mySerial.readBytes((char *)resp, 6) != 6) continue;
        if (resp[0] == 0xaa && resp[1] == (0x0e | cameraAddr) && resp[2] == 0x06 && resp[4] == 0 && resp[5] == 0) break;
    }
    cmd[1] = 0x05 | cameraAddr;
    cmd[2] = 0;
    cmd[3] = 0;
    cmd[4] = 0;
    cmd[5] = 0;
    while (1)
    {
        clearRxBuf();
        sendCmd(cmd, 6);
        if (mySerial.readBytes((char *)resp, 6) != 6) continue;
        if (resp[0] == 0xaa && resp[1] == (0x0e | cameraAddr) && resp[2] == 0x05 && resp[4] == 0 && resp[5] == 0) break;
    }
    cmd[1] = 0x04 | cameraAddr;
    cmd[2] = 0x1;
    while (1)
    {
        clearRxBuf();
        sendCmd(cmd, 6);
        if (mySerial.readBytes((char *)resp, 6) != 6) continue;
        if (resp[0] == 0xaa && resp[1] == (0x0e | cameraAddr) && resp[2] == 0x04 && resp[4] == 0 && resp[5] == 0)
        {
            mySerial.setTimeout(1000);
            if (mySerial.readBytes((char *)resp, 6) != 6)
            {
                continue;
            }
            if (resp[0] == 0xaa && resp[1] == (0x0a | cameraAddr) && resp[2] == 0x01)
            {
                picTotalLen = (resp[3]) | (resp[4] << 8) | (resp[5] << 16);
                Serial.print("picTotalLen:");
                Serial.println(picTotalLen);
                break;
            }
        }
    }

}
/*********************************************************************/
void GetData()
{
    unsigned int pktCnt = (picTotalLen) / (PIC_PKT_LEN - 6);
    if ((picTotalLen % (PIC_PKT_LEN-6)) != 0) pktCnt += 1;

    char cmd[] = { 0xaa, 0x0e | cameraAddr, 0x00, 0x00, 0x00, 0x00 };
    unsigned char pkt[PIC_PKT_LEN];

    char picName[] = "pic00.jpg";
    picName[3] = picNameNum/10 + '0';
    picName[4] = picNameNum%10 + '0';

    if (SD.exists(picName))
    {
        SD.remove(picName);
    }

    myFile = SD.open(picName, FILE_WRITE);
    if(!myFile){
        Serial.println("myFile open fail...");
    }
    else{
        mySerial.setTimeout(1000);
        for (unsigned int i = 0; i < pktCnt; i++)
        {
            cmd[4] = i & 0xff;
            cmd[5] = (i >> 8) & 0xff;

            int retry_cnt = 0;
            retry:
            delay(10);
            clearRxBuf();
            sendCmd(cmd, 6);
            uint16_t cnt = mySerial.readBytes((char *)pkt, PIC_PKT_LEN);

            unsigned char sum = 0;
            for (int y = 0; y < cnt - 2; y++)
            {
                sum += pkt[y];
            }
            if (sum != pkt[cnt-2])
            {
                if (++retry_cnt < 100) goto retry;
                else break;
            }

            myFile.write((const uint8_t *)&pkt[4], cnt-6);
            //if (cnt != PIC_PKT_LEN) break;
        }
        cmd[4] = 0xf0;
        cmd[5] = 0xf0;
        sendCmd(cmd, 6);
    }
    myFile.close();
    picNameNum ++;
}
