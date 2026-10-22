#define GH_INCLUDE_PORTAL
#include <GyverHub.h>


GyverHub hub("MyDevices", "Roller Blinds", "🪟");  // префикс, имя, иконка

unsigned long lastConnectedTime = 0;
const unsigned long wifiTimeout = 30000;

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

void build(gh::Builder& b) {
  if (b.Button().icon("f077").click()) {
    sendAOKCommand("10100011000000001101100000101100000001000000000000001011000100111");
    //sendAOKCommand("10100011000000001101100000101100000001000000000000100100001011001");
    Serial.println("UP");
  }

  if (b.Button().icon("f068").click()) {
    sendAOKCommand("10100011000000001101100000101100000001000000000000100011001010111");
    Serial.println("STOP");
  }

  if (b.Button().icon("f078").click()) {
    sendAOKCommand("10100011000000001101100000101100000001000000000001000011010010111");
    //sendAOKCommand("10100011000000001101100000101100000001000000000000100100001011001");
    Serial.println("DOWN");
  }
}

void setup() {
  pinMode(TRANSMIT_PIN, OUTPUT);
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin("HH71VM", "hh71vm12345");

  
  hub.onBuild(build);     // подключаем билдер
  hub.begin();            // запускаем систему
}

void loop() {
  hub.tick();  // обязательно тикаем тут
  if (WiFi.status() == WL_CONNECTED) {
    // As long as connected, keep refreshing the timestamp
    lastConnectedTime = millis();
  } else {
    // If disconnected, check if the timeout has passed
    Serial.println("Trying to connect");
    if (millis() - lastConnectedTime >= wifiTimeout) {
      Serial.println("Wi-Fi lost for over 30 seconds! Rebooting...");
      delay(500); // Give serial time to print
      ESP.restart(); // Restart the ESP8266
    }
  }
}
