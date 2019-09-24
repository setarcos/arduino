#define CLK 16
#define SI 17

unsigned char aov[128];

void delay_us(void)
{
    for (char i = 0; i < 10; ++i);
}

void read_tsl(void)
{
    digitalWrite(CLK, HIGH);
    digitalWrite(SI, LOW);
    delay_us();
    digitalWrite(SI, HIGH);
    digitalWrite(CLK, LOW);
    delay_us();
    digitalWrite(CLK, HIGH);
    digitalWrite(SI, LOW);
    delay_us();
    for (unsigned char i = 0; i < 128; ++i) {
        digitalWrite(CLK, LOW);
        aov[i] = analogRead(1) >> 2;
        digitalWrite(CLK, HIGH);
        delay_us();
    }
}

void setup()
{
    pinMode(CLK, OUTPUT);
    pinMode(SI, OUTPUT);
    Serial.begin(115200);
    read_tsl();
    delay(20);
    read_tsl();
    for (unsigned char i = 0; i < 128; ++i)
        Serial.println(aov[i]);
}

void loop()
{
}
