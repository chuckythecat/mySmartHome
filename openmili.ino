#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <printf.h>

#include "PL1167_nRF24.h"
#include "MiLightRadio.h"

#define CE_PIN 10
#define CSN_PIN 9

RF24 radio(CE_PIN, CSN_PIN);
PL1167_nRF24 prf(radio);
MiLightRadio mlr(prf);

uint8_t onPacket[7] =     {0xD8, 0xD7, 0xA0, 0x00, 0x01, 0x00, 0x00};
uint8_t offPacket[7] =    {0xD8, 0xD7, 0xA0, 0x00, 0x02, 0x00, 0x00};
uint8_t SminusPacket[7] = {0xD8, 0xD7, 0xA0, 0x00, 0x03, 0x00, 0x00};
uint8_t mPacket[7] =      {0xD8, 0xD7, 0xA0, 0x00, 0x04, 0x00, 0x00};
uint8_t SplusPacket[7] =  {0xD8, 0xD7, 0xA0, 0x00, 0x05, 0x00, 0x00};

void setup()
{
  Serial.begin(9600);
  printf_begin();
  delay(1000);
  Serial.println("# OpenMiLight Receiver/Transmitter starting");
  mlr.begin();
}

int lastrecv;
int receive;
void loop()
{
  while (Serial.available() == 0) {}     //wait for data available
  receive = Serial.read();

  Serial.print("I received: ");
  Serial.println(receive);
  
  if (lastrecv == receive) {
    mlr.resend();
    Serial.println("resend");
  }
  else if (receive == 49) {
    mlr.write(onPacket, sizeof(onPacket));
    Serial.println("ON");
  }
  else if (receive == 50) {
    mlr.write(offPacket, sizeof(offPacket));
    Serial.println("OFF");
  }
  else if (receive == 51) {
    mlr.write(SminusPacket, sizeof(SminusPacket));
    Serial.println("S-");
  }
  else if (receive == 52) {
    mlr.write(mPacket, sizeof(mPacket));
    Serial.println("M");
  }
  else if (receive == 53) {
    mlr.write(SplusPacket, sizeof(SplusPacket));
    Serial.println("S+");
  }
  lastrecv = receive;
}
