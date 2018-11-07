#include <SPI.h>

#define OLED_SCLK 13
#define OLED_SDO  12 // SDO_ZK
#define OLED_SDIN 11
#define OLED_DC   10
#define OLED_RES   9
#define OLED_CS    8
#define OLED_ZK    7

#define MAXROWS      64
#define MAXCOLS     240

void resetDevice();
void clearDisp();
void rectFill();
void sendCmd(uint8_t cmd);
void sendData(uint8_t data);
void drawHz(uint8_t a, uint8_t b);
void getHz(uint8_t *hz);

uint8_t hzbuf[128];

void setup()
{
    pinMode(OLED_DC, OUTPUT);
    pinMode(OLED_RES, OUTPUT);
    pinMode(OLED_CS, OUTPUT);
    pinMode(OLED_ZK, OUTPUT);
    digitalWrite(OLED_CS, HIGH);
    digitalWrite(OLED_ZK, HIGH);
    SPI.begin();
    resetDevice();
}

void loop()
{ 
    clearDisp();
//    rectFill();
    uint8_t *hz="北京大学";
    for (int i = 0; i < 4; ++i) {
        getHz((uint8_t *)&hz[i * 2]);
        drawHz(8 + i * 6, 0);
    }
    for(;;);
}

void sendCmd(uint8_t cmd)
{
    digitalWrite(OLED_CS, LOW);
    digitalWrite(OLED_DC, LOW);
    SPI.transfer(cmd);
    digitalWrite(OLED_CS, HIGH);
}

void sendData(uint8_t data)
{
    digitalWrite(OLED_CS, LOW);
    digitalWrite(OLED_DC, HIGH);
    SPI.transfer(data);
    digitalWrite(OLED_CS, HIGH);
}

void getHz(uint8_t *hz)
{
    uint8_t i;
    uint32_t add;

    if (((hz[0] >= 0xa1) && (hz[0] <= 0xa9)) && (hz[1] >= 0xa1)) {
        add = (hz[0] - 0xa1) * 94;
        add += (hz[1] - 0xa1);
    } else if (((hz[0] >= 0xb0) && (hz[0] <= 0xf7)) && (hz[1] >= 0xa1)) {
        add = (hz[0] - 0xb0) * 94;
        add += (hz[1] - 0xa1 + 846);
    }
    add = (add * 72 + 0x68190);
 //   add += (add * 32 + 0x2c9d0);
    digitalWrite(OLED_ZK, LOW);
    SPI.transfer(0x03);
    SPI.transfer((add & 0xff0000) >> 16);
    SPI.transfer((add & 0xff00) >> 8);
    SPI.transfer(add & 0xff);
   // SPI.transfer(0x00);
    for (i = 0; i < 72; ++i)
        hzbuf[i] = SPI.transfer(0x00);
    digitalWrite(OLED_ZK, HIGH);
}

void setColumn(uint8_t a, uint8_t b)
{
    sendCmd(0x15); // Set Column Address
    sendData(a);
    sendData(b);
}

void setRow(uint8_t a, uint8_t b)
{
    sendCmd(0x75); // Set Row Address
    sendData(a);
    sendData(b);
}

void enableWrite()
{
    sendCmd(0x5c); // Enable MCU to Write into RAM
}

void drawHz(uint8_t a, uint8_t b)
{
    uint8_t i;
    setColumn(0x1c + a, 0x1c + a + 5);
    setRow(b, b + 23);
    enableWrite();
    for (i = 0; i < 72; ++i) {
        uint8_t j, k;
        k = hzbuf[i];
        for (j = 0; j < 4; ++j) {
            switch (k & 0xc0) {
                case 0x00: sendData(0x00);
                           break;
                case 0x40: sendData(0x0f);
                           break;
                case 0x80: sendData(0xf0);
                           break;
                case 0xc0: sendData(0xff);
                           break;
            }
            k <<= 2;
        }
    }
}

void resetDevice()
{
    digitalWrite(OLED_RES, LOW);
    delay(10);
    digitalWrite(OLED_RES, HIGH);

    sendCmd(0xfd); // Set Command Lock (MCU protection status)
    sendData(0x12); // = Reset

    sendCmd(0xae); // Display Off

    sendCmd(0xb3); // Set Front Clock Divider / Oscillator Frequency
    sendData(0x91); // 80 Frames Per Sec

    sendCmd(0xca); // Set MUX Ratio
    sendData(0x3f); // = 63d = 64MUX

    sendCmd(0xa2); // Set Display Offset
    sendData(0x00); // = RESET

    sendCmd(0xa1); // Set Display Start Line
    sendData(0x00); // = register 00h

    sendCmd(0xa0); // Set Re-map and Dual COM Line mode
    sendData(0x14); // = Reset except Enable Nibble Re-map, Scan from COM[N-1] to COM0, where N is the Multiplex ratio
    sendData(0x11); // = Reset except Enable Dual COM mode (MUX = 63)

    sendCmd(0xb5); // Set GPIO
    sendData(0x00); // = GPIO0, GPIO1 = HiZ, Input Disabled

    sendCmd(0xab); // Function Selection
    sendData(0x01); // = reset = Enable internal VDD regulator

    sendCmd(0xb4); // Display Enhancement A
    sendData(0xa0); // = Enable external VSL
    sendData(0xfd); // 0xb5 = Normal (reset), 0xfd for enhanced

    sendCmd(0xc1); // Set Contrast Current
    sendData(0xef); // = reset

    sendCmd(0xc7); // Master Contrast Current Control
    sendData(0x0f); // = no change

    sendCmd(0xb9); // Select Default Linear Gray Scale table

    sendCmd(0xb1); // Set Phase Length
    sendData(0xe2); // = Phase 1 period (reset phase length) = 5 DCLKs, Phase 2 period (first pre-charge phase length) = 14 DCLKs

    sendCmd(0xd1); // Display Enhancement B
    sendData(0xa2); // = Normal (reset)
    sendData(0x20); // n/a

    sendCmd(0xbb); // Set Pre-charge voltage
    sendData(0x1f); // = 0.60 x VCC

    sendCmd(0xb6); // Set Second Precharge Period
    sendData(0x08); // = 8 dclks [reset]

    sendCmd(0xbe); // Set VCOMH
    sendData(0x07); // = 0.86 x VCC

    sendCmd(0xa6); // Set Display Mode = Normal Display

    sendCmd(0xa9); // Exit Partial Display

//    sendCmd(0xaf); // Set Sleep mode OFF (Display ON)
    delay(10);
}

void rectFill()
{
    uint8_t i, j;
    sendCmd(0xae);
    setColumn(0x1c, 0x1c + 0x05); // 32 points, why 0x1c? 
    setRow(0x00, 24);
    enableWrite();
    for (i = 0; i < 24; ++i)
        for (j = 0; j < 6; ++j)
        {
            sendData(0xff);
            sendData(0xff);
        }
    sendCmd(0xaf);
}

void clearDisp()
{
    unsigned int i, j;

    // Turn off display while clearing (also hides noise at powerup)
    sendCmd(0xae); // Set Display Mode = OFF

    setColumn(0x00,0x77);
    setRow(0x00,0x7f);
    enableWrite();

    for(i = 0; i < MAXROWS; i++)
    {
        for(j = 0; j < MAXCOLS / 2; j++)
        {
            sendData(0x00);
            sendData(0x00);
        }
    }
    sendCmd(0xaf); // Set Display Mode = Normal Display
}

