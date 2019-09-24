#include <SoftwareSerial.h>
#define SS_RX A2
#define SS_TX A3
#define ESP_EN 2

SoftwareSerial ss(SS_RX, SS_TX); // RX, TX
String comdata = "";
uint8_t ss_ok;  // AT command ready
uint8_t state;  // state machine
uint8_t msg;    // message count
uint32_t t;     // time

void setup()
{
  pinMode(SS_RX, INPUT);
  pinMode(SS_TX, OUTPUT);
  pinMode(ESP_EN, OUTPUT);
  digitalWrite(ESP_EN, HIGH); // Enable the module
  Serial.begin(115200);
  ss.begin(115200);
  ss_ok = 0;
  state = 0;
  msg = 0;
}

// This is the client
void loop()
{
  // It has to be the hardware serial to be ccnnected to the esp8266,
  // software serial does not work under 115200 at least
  while (Serial.available()){
    uint8_t c = Serial.read();
    if ((c != '\r') && (c != '\n'))
      comdata += char(c);
    else if (c == '\n'){
      if (comdata.length()) ss_ok = 1;
      break;
    }
    if (comdata.length() >= 8) { // receive some msg
      if (comdata.charAt(0) == '+') {
        if (comdata.length() == comdata.charAt(5) - '0' + 7) {
          ss_ok = 1;
          break;
        }
      }
    }
  }
  if (ss_ok){
    ss_ok = 0;
    comdata.trim();
    ss.println(comdata);
    switch (state) {
      case 0:
        if (comdata == "ready"){
          Serial.print("AT+CWMODE=1\r\n");
          state = 1;
          ss.println("Ready");
        }
        break;
      case 1:
        if (comdata == "OK") {
          Serial.print("AT+CWJAP_DEF=\"Arduino\",\"yingcai18\"\r\n");
          state = 2;
          ss.println("Mode Ready");
        }
        break;
      case 2:
        if (comdata == "OK") {
          Serial.print("AT+CIPMUX=0\r\n");
          state = 3;
          ss.println("AP Ready");
        }
        break;
      case 3:
        if (comdata == "OK") {
          Serial.print("AT+CIPSTART=\"TCP\",\"192.168.0.101\",333\r\n");
          state = 4;
          ss.println("MUX Ready");
        }
        break;
      case 4:
        if (comdata == "OK") {
          Serial.print("AT+CIPSEND=3\r\n");
          state = 5;
        }
        break;
      case 5:
        if (comdata == "OK") {
          msg++;
          if (msg < 10) {
            state = 6;
            Serial.print("hi!");
            t = millis();
          }
          else {
            state = 7;
            Serial.print("BYE");
          }
        }
        break;
      case 6:
        if (comdata.indexOf("+IPD") >= 0) {
          Serial.print("AT+CIPSEND=3\r\n");
          state = 5;
          ss.println(millis() - t);
        }
        break;
      case 7:
        if (comdata == "CLOSED")
          ss.println("OVER");
        break;
    }
    comdata = "";
  }
}
