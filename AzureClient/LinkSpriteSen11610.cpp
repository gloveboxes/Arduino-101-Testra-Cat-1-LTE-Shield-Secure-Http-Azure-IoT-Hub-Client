#include "LinkSpriteSen11610.h"
#include <base64.hpp>

void LinkSprite::SendResetCmd()
{
    Serial2->write(0x56);
    Serial2->write(ZERO);
    Serial2->write(0x26);
    Serial2->write(ZERO);
}

void LinkSprite::SetImageSizeCmd(byte Size)
{
    Serial2->write(0x56);
    Serial2->write(ZERO);
    Serial2->write(0x54);
    Serial2->write(0x01);
    Serial2->write(Size);
}

void LinkSprite::getImageSize()
{
    //56 00 34 01 00
    Serial2->write(0x56);
    Serial2->write(ZERO);
    Serial2->write(0x34);
    Serial2->write(0x01);
    Serial2->write(ZERO);
}

void LinkSprite::SetBaudRateCmd(byte baudrate)
{
    Serial2->write(0x56);
    Serial2->write(ZERO);
    Serial2->write(0x24);
    Serial2->write(0x03);
    Serial2->write(0x01);

    Serial2->write(0x1C);
    Serial2->write(0x4C);
    //0D A6
    //1C 4C
    //    Serial2->write(baudrate);
}

void LinkSprite::SendTakePhotoCmd()
{
    Serial2->write(0x56);
    Serial2->write(ZERO);
    Serial2->write(0x36);
    Serial2->write(0x01);
    Serial2->write(ZERO);
}

void LinkSprite::SendStopTakePhotoCmd()
{
    //56 00 36 01 03
    Serial2->write(0x56);
    Serial2->write(ZERO);
    Serial2->write(0x36);
    Serial2->write(0x01);
    Serial2->write(0x03);
}

void LinkSprite::SendReadDataCmd()
{
    Serial2->write(0x56);
    Serial2->write(ZERO);
    Serial2->write(0x32);
    Serial2->write(0x0c);
    Serial2->write(ZERO);
    Serial2->write(0x0a);
    Serial2->write(ZERO);
    Serial2->write(ZERO);

    Serial2->write(highByte(cameraAddress));
    Serial2->write(lowByte(cameraAddress));

    Serial2->write(ZERO);
    Serial2->write(ZERO);

    Serial2->write(highByte(frameLength));
    Serial2->write(lowByte(frameLength));

    Serial2->write(ZERO);
    Serial2->write(0x0A);
    cameraAddress += frameLength;
}

void LinkSprite::StopTakePhotoCmd()
{
    Serial2->write(0x56);
    Serial2->write(ZERO);
    Serial2->write(0x36);
    Serial2->write(0x01);
    Serial2->write(0x03);
}

void LinkSprite::initCamera()
{
    EndFlag = false;
    cameraAddress = 0x0000;
    length = 0;

    if (!initialised)
    {
        Serial2->begin(38400);
        initialised = true;
    }

    SendResetCmd();
    delay(100);
    Serial2->begin(38400);
    delay(1000);
    Serial.println("reset");
    while (Serial2->available())
    {
        incomingbyte = Serial2->read();
    }
}

void LinkSprite::setBaud()
{
    SetBaudRateCmd(0x0D);
    delay(500);
    Serial2->end();

    Serial2->begin(57600);
    delay(200);
    Serial.println("baud");
    while (Serial2->available())
    {
        incomingbyte = Serial2->read();
    }
}

void LinkSprite::takePhoto()
{
    SendTakePhotoCmd();
    delay(500);
    Serial.println("take photo");
    while (Serial2->available())
    {
        incomingbyte = Serial2->read();
        //    Serial.println(String(incomingbyte));
    }
    Serial.println("photo taken");
}

int LinkSprite::getImagelength()
{
    byte buffer[9];
    int index = 0, length = 0;

    getImageSize();
    delay(500);

    while (Serial2->available() && index < 9)
    {
        incomingbyte = Serial2->read();
        buffer[index] = incomingbyte;
        index++;
    }

    if (index == 9)
    {
        length = buffer[7] << 8;
        length += buffer[8];
    }
    else
    {
        length = -1;
        EndFlag = true;
    }
    return length;
}

void LinkSprite::stopPhoto()
{
    Serial.println();
    Serial.println("stop photo");
    SendStopTakePhotoCmd();
    delay(1000);
    while (Serial2->available())
    {
        incomingbyte = Serial2->read();
    }
}

bool LinkSprite::eof()
{
    return EndFlag;
}

int LinkSprite::getCalculatedImageSize()
{
    return length;
}

char *LinkSprite::getCamaraData()
{
    int j = 0, k = 0, count = 0;

    SendReadDataCmd();
    delay(10);

    while (Serial2->available() > 0)
    {
        incomingbyte = Serial2->read();
        k++;
        //            delay(1); //250 for regular
        if ((k > 5) && (j < 24) && (!EndFlag))
        {
            a[j] = incomingbyte;
            if ((a[j - 1] == 0xFF) && (a[j] == 0xD9)) //tell if the picture is finished
            {
                EndFlag = true;
            }
            j++;
            count++;
        }
    }
    for (int i = 0; i < Base64BufferLength; i++)
    {
        base64[i] = 0;
    }
    
    encode_base64(a, count, base64);
    length += count;

    return (char *)base64;
}
