#include <SoftwareSerial.h>
#define SS_RX A2
#define SS_TX A3
#define ESP_EN 2

SoftwareSerial ss(SS_RX, SS_TX); // RX, TX
String comdata = "";
uint8_t ss_ok;
uint8_t state;

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
}

// This is the server
void loop()
{

  while (Serial.available()){
    uint8_t c = Serial.read();
    if ((c != '\r') && (c != '\n'))
      comdata += char(c);
    else if (c == '\n'){
      if (comdata.length()) ss_ok = 1;
      break;
    }
    if (comdata.length() >= 10) { // receive some msg
      if (comdata.charAt(0) == '+') {
        if (comdata.length() == comdata.charAt(7) - '0' + 9) {
          ss_ok = 1;
          break;
        }
      }
    }
    //delay(1);
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
          state = 7;
          ss.println("Mode Ready");
        }
        break;
      case 2:
        if (comdata == "OK") {
          Serial.print("AT+CIPMUX=1\r\n");
          state = 3;
          ss.println("AP Ready");
        }
        break;
      case 3:
        if (comdata == "OK") {
          Serial.print("AT+CIPSERVER=1\r\n");
          state = 4;
          ss.println("MUX Ready");
        }
        break;
      case 4:
        if (comdata.indexOf("+IPD") == 0) {
          Serial.print("AT+CIPSEND=0,3\r\n");
          state = 5;
        }
        if (comdata.indexOf("BYE") >= 0) {
          Serial.print("AT+CIPSERVER=0\r\n");
          state = 6;
        }
        break;
      case 5:
        if (comdata == "OK") {
          Serial.print("ya!");
          state = 4;
        }
        break;
      case 6:
        if (comdata == "OK") {
          ss.println("Over");
        }
        break;
      case 7:
        if (comdata == "OK") {
          Serial.print("AT+CIFSR\r\n"); // Show IP
          state = 2;
        }
        break;
    }
    comdata = "";
  }
}
