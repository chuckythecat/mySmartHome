#include <GyverHub.h>
#include <builder.h>
#include <canvas.h>
#include <config.hpp>
#include <macro.hpp>
#include <stream.h>

#include <RF24.h>
#include <RF24_config.h>
#include <nRF24L01.h>
#include <printf.h>

#include "PL1167_nRF24.h"
#include "MiLightRadio.h"

GyverHub hub("MyDevices", "Roller Blinds", "🪟");  // префикс, имя, иконка

WiFiEventHandler gotIpEventHandler, disconnectedEventHandler;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ROLLER BLINDS~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define TRANSMIT_PIN             D8      // We'll use digital 13 for transmitting
#define REPEAT_COMMAND           8       // How many times to repeat the same command: original remotes repeat 8 (multi) or 10 (single) times by default

// If you wish to use PORTB commands instead of digitalWrite, these are for Arduino Uno digital 13:
#define D13high | 0x20;
#define D13low  & 0xDF;

// Timings in microseconds (us). Get sample count by zooming all the way in to the waveform with Audacity.
// Calculate microseconds with: (samples / sample rate, usually 44100 or 48000) - ~15-20 to compensate for delayMicroseconds overhead.
// Sample counts listed below with a sample rate of 44100 Hz:
#define AOK_AGC1_PULSE                   5300  // 234 samples
#define AOK_AGC2_PULSE                   530   // 24 samples after the actual AGC bit
#define AOK_RADIO_SILENCE                5030  // 222 samples

#define AOK_PULSE_SHORT                  270   // 12 samples
#define AOK_PULSE_LONG                   565   // 25 samples, approx. 2 * AOK_PULSE_SHORT

#define AOK_COMMAND_BIT_ARRAY_SIZE       65    // Command bit count

void sendAOKCommand(char* command) {

  if (command == NULL) {
    return;
  }

  // Prepare for transmitting and check for validity
  // Prepare the digital pin for output

  if (strlen(command) < AOK_COMMAND_BIT_ARRAY_SIZE) {
    return;
  }
  if (strlen(command) > AOK_COMMAND_BIT_ARRAY_SIZE) {
    return;
  }

  // Repeat the command:
  for (int i = 0; i < REPEAT_COMMAND; i++) {
    doAOKTribitSend(command);
  }

  // Disable output to transmitter to prevent interference with
  // other devices. Otherwise the transmitter will keep on transmitting,
  // disrupting most appliances operating on the 433.92MHz band:
  digitalWrite(TRANSMIT_PIN, LOW);
}

void doAOKTribitSend(char* command) {

  // Starting (AGC) bits:
  transmitHigh(AOK_AGC1_PULSE);
  transmitLow(AOK_AGC2_PULSE);

  // Transmit command:
  for (int i = 0; i < AOK_COMMAND_BIT_ARRAY_SIZE; i++) {

    // If current bit is 0, transmit HIGH-LOW-LOW (100):
    if (command[i] == '0') {
      transmitHigh(AOK_PULSE_SHORT);
      transmitLow(AOK_PULSE_LONG);
    }

    // If current bit is 1, transmit HIGH-HIGH-LOW (110):
    if (command[i] == '1') {
      transmitHigh(AOK_PULSE_LONG);
      transmitLow(AOK_PULSE_SHORT);
    }
  }

  // Radio silence at the end.
  // It's better to go a bit over than under minimum required length:
  transmitLow(AOK_RADIO_SILENCE);
}

void transmitHigh(int delay_microseconds) {
  digitalWrite(TRANSMIT_PIN, HIGH);
  //PORTB = PORTB D13high; // If you wish to use faster PORTB calls instead
  delayMicroseconds(delay_microseconds);
}

void transmitLow(int delay_microseconds) {
  digitalWrite(TRANSMIT_PIN, LOW);
  //PORTB = PORTB D13low; // If you wish to use faster PORTB calls instead
  delayMicroseconds(delay_microseconds);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~MILIGHT~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define CE_PIN  D0
#define CSN_PIN D1

RF24 radio(CE_PIN, CSN_PIN);
PL1167_nRF24 prf(radio);
MiLightRadio mlr(prf);

uint8_t onPacket[7] =     {0xD8, 0xD7, 0xA0, 0x00, 0x01, 0x00, 0x00};
uint8_t offPacket[7] =    {0xD8, 0xD7, 0xA0, 0x00, 0x02, 0x00, 0x00};
uint8_t SminusPacket[7] = {0xD8, 0xD7, 0xA0, 0x00, 0x03, 0x00, 0x00};
uint8_t mPacket[7] =      {0xD8, 0xD7, 0xA0, 0x00, 0x04, 0x00, 0x00};
uint8_t SplusPacket[7] =  {0xD8, 0xD7, 0xA0, 0x00, 0x05, 0x00, 0x00};
uint8_t maxbrightPacket[7] =  {0xD8, 0xD7, 0x2F, 0x00, 0x0F, 0x00, 0x00};
uint8_t minbrightPacket[7] =  {0xD8, 0xD7, 0x20, 0x00, 0x0F, 0x00, 0x00};

void build() {
  hub.BeginWidgets(120);
  hub.WidgetSize(100);
  if (hub.Button(0, "UP")) {
    sendAOKCommand("10100011000000001101100000101100000001000000000000001011000100111");
    sendAOKCommand("10100011000000001101100000101100000001000000000000100100001011001");
    Serial.println("UP");
  }

  hub.BeginWidgets(120);
  hub.WidgetSize(100);
  if (hub.Button(0, "STOP")) {
    sendAOKCommand("10100011000000001101100000101100000001000000000000100011001010111");
    Serial.println("STOP");
  }

  hub.BeginWidgets(120);
  hub.WidgetSize(100);
  if (hub.Button(0, "DOWN")) {
    sendAOKCommand("10100011000000001101100000101100000001000000000001000011010010111");
    sendAOKCommand("10100011000000001101100000101100000001000000000000100100001011001");
    Serial.println("DOWN");
  }

  hub.BeginWidgets(120);
  hub.WidgetSize(50);
  if (hub.Button(0, "Lights ON")) {
    mlr.write(onPacket, sizeof(onPacket));
    Serial.println("Lights ON");
  }

  hub.WidgetSize(50);
  if (hub.Button(0, "Lights OFF")) {
    mlr.write(offPacket, sizeof(offPacket));
    Serial.println("Lights OFF");
  }

  hub.BeginWidgets(120);

  hub.WidgetSize(33);
  if (hub.Button(0, "Warmer")) {
    mlr.write(SminusPacket, sizeof(SminusPacket));
    Serial.println("Lights S-");
  }
  
  hub.WidgetSize(33);
  if (hub.Button(0, "Default")) {
    mlr.write(mPacket, sizeof(mPacket));
    Serial.println("Lights M");
  }

  hub.WidgetSize(33);
  if (hub.Button(0, "Colder")) {
    mlr.write(SplusPacket, sizeof(SplusPacket));
    Serial.println("Lights S+");
  }

  hub.BeginWidgets(120);
  hub.WidgetSize(50);
  if (hub.Button(0, "Fullbright")) {
    mlr.write(maxbrightPacket, sizeof(maxbrightPacket));
    Serial.println("Lights brightness max");
  }
  hub.WidgetSize(50);
  if (hub.Button(0, "Min bright")) {
    mlr.write(minbrightPacket, sizeof(minbrightPacket));
    Serial.println("Lights brightness min");
  }
}

void setup() {
  pinMode(TRANSMIT_PIN, OUTPUT);
  Serial.begin(115200);

  mlr.begin();

  // подключение к WiFi...
  // настройка MQTT
  //hub.setupMQTT("test.mosquitto.org", 1883);
  //hub.setupMQTT("m8.wqtt.ru", 13448, "****", "****");
  disconnectedEventHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& event) {
    Serial.println("Station disconnected");
  });
  gotIpEventHandler = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP& event) {
    Serial.print("Station connected, IP: ");
    Serial.println(WiFi.localIP());
  });

  WiFi.mode(WIFI_STA);
  WiFi.begin("RTK12", "Bk82720311");
  uint8_t mac[6] {0x00, 0xde, 0xad, 0xbe, 0xef, 0x00};
  wifi_set_macaddr(STATION_IF, mac);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("connecting...");
    delay(200);
    yield();
  }
  
  hub.onBuild(build);     // подключаем билдер
  hub.begin();            // запускаем систему
}

void loop() {
  hub.tick();  // обязательно тикаем тут
}
