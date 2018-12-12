#include <MsTimer2.h>
#include <PinChangeInterrupt.h>

#define TR 8
#define EC 9

volatile uint8_t newvalue = 0;
uint16_t duration;

void control()
{
  static int state = 0;
  if (state == 0) {
    digitalWrite(TR, HIGH);
    delayMicroseconds(10);
    digitalWrite(TR, LOW);
  }
  if (state < 5) state++;
  else state = 0;
}

void measure()
{
  uint8_t trigger = getPinChangeInterruptTrigger(digitalPinToPCINT(EC));
  static unsigned long oldmicro;
  if (trigger == RISING) oldmicro = micros();
  if (trigger == FALLING) {
    duration = micros() - oldmicro;
    newvalue = 1;
  }
}

void setup() {
  pinMode(TR, OUTPUT);
  pinMode(EC, INPUT);
  digitalWrite(TR, LOW);
  attachPCINT(digitalPinToPCINT(EC), measure, CHANGE);
  MsTimer2::set(10, control);
  MsTimer2::start();
  Serial.begin(38400);
}

void loop() {
  if (newvalue) {
    Serial.println(duration/29/2);
    newvalue = 0;
  }

}
