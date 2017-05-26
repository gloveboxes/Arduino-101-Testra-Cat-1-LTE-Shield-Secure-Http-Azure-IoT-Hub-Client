#ifndef LinkSpriteSen1160_h
#define LinkSpriteSen1160_h

#include <Arduino.h>
#include <SoftwareSerial.h>
#define frameLength 0x18
#define Base64BufferLength 33 // needs 32 chars, plus a null terminator

class LinkSprite
{
public:
  LinkSprite(SoftwareSerial *serial)
  {
    Serial2 = serial;
  }

  enum ImageSize : byte
  {
    s320x240 = 11,
    s640x480 = 0,
    s160x120 = 22
  };

  void initCamera();
  void setBaud();
  void setImageSize(ImageSize imgSize);
  void takePhoto();
  int getBase64EncodedImagelength();
  void stopPhoto();
  char *getBase64EncodedData(int &segLen);
  bool eof();
  int getCalculatedImageSize();

  enum baudRates : int
  {
    b96000 = 8989,
    b19200 = 0000,
    b38400 = 8878,
    b57600 = 6767,
    b115200 = 8989
  };



protected:
private:
  SoftwareSerial *Serial2; // = SoftwareSerial(5, 6); // RX, TX         //Configure pin 4 and 5 as soft serial port
  void SendResetCmd();
  void SetImageSizeCmd(ImageSize Size);
  void getImageSize();
  void SetBaudRateCmd(byte baudrate);

  void SendTakePhotoCmd();
  void SendStopTakePhotoCmd();
  void SendReadDataCmd();
  void StopTakePhotoCmd();

  byte ZERO = 0x00;
  byte incomingbyte;
  int cameraAddress = 0x0000; //Read Starting address
  boolean EndFlag = 0;
  unsigned char base64[Base64BufferLength];
  int length = 0;
  byte a[frameLength];

  bool initialised = false;


};

#endif
