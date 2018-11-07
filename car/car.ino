/* Based on code from www.minibalance.com
 */
#include <SSD1306.h>
#include <Servo.h>
#include <PinChangeInt.h>
#include <MsTimer2.h>

#define OLED_DC 10
#define OLED_CLK 19
#define OLED_MOSI 13
#define OLED_RESET 12

#define AIN1 11
#define AIN2 5
#define BIN1 6
#define BIN2 3
#define SERVO 9

#define ENCODER_L 8
#define DIRECTION_L 4
#define ENCODER_R 7
#define DIRECTION_R 2

#define KEY 18
#define T 0.156f
#define L 0.1445f
#define pi 3.1415926

Servo myservo;
SSD1306 oled(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, 0);
volatile long Velocity_L, Velocity_R;
int Velocity_Left, Velocity_Right = 0, Velocity, Angle;
char Flag_Direction;
float Velocity_KP = 0.3, Velocity_KI = 0.3;
unsigned char Flag_Stop = 0, PID_Send, Flash_Send, Bluetooth_Velocity = 15;
float Target_A, Target_B;
int Battery_Voltage;
unsigned char servo;
// void (* resetFunc) (void) = 0;// Reset func

unsigned char My_click(void)
{
	static byte flag_key = 1;
	if (flag_key && (digitalRead(KEY) == 0)) {	// Clicked
		flag_key = 0;
		if (digitalRead(KEY) == 0)
			return 1;
	} else if (digitalRead(KEY) == 1)
		flag_key = 1;
	return 0;
}

void OLED_ShowNumber(uint8_t x, uint8_t y, uint32_t num, uint8_t len)
{
	int8_t t;
	for (t = len - 1; t >= 0; t--) {
		oled.drawchar(x + 6 * t, y, num % 10 + '0');
		num /= 10;
	}
}

void Set_Pwm(int motora, int motorb)
{
	if (motora > 0)
		analogWrite(AIN2, motora), digitalWrite(AIN1, LOW);
	else
		digitalWrite(AIN1, HIGH), analogWrite(AIN2, 255 + motora);

	if (motorb > 0)
		digitalWrite(BIN2, LOW), analogWrite(BIN1, motorb);
	else
		analogWrite(BIN1, 255 + motorb), digitalWrite(BIN2, HIGH);
}

unsigned char Turn_Off()
{
	if (Flag_Stop == 1 || Battery_Voltage < 700) {
		digitalWrite(AIN1, LOW);
		digitalWrite(AIN2, LOW);
		digitalWrite(BIN1, LOW);
		digitalWrite(BIN2, LOW);
		return 1;
	}
	return 0;
}

void Kinematic_Analysis(float velocity, float angle)
{
	char K = 1;
	Target_A = velocity * (1 + T * tan(angle * pi / 180) / 2 / L);
	Target_B = velocity * (1 - T * tan(angle * pi / 180) / 2 / L);
	servo = 95 + angle * K;
	//  if(servo>95)servo=servo*1.15;
	myservo.write(servo);
}

int Incremental_PI_A(int Encoder, int Target)
{
	static float Bias, Pwm, Last_bias;
	Bias = Encoder - Target;
	Pwm += Velocity_KP * (Bias - Last_bias) + Velocity_KI * Bias;
	if (Pwm > 255)
		Pwm = 255;
	if (Pwm < -255)
		Pwm = -255;
	Last_bias = Bias;
	return Pwm;
}

int Incremental_PI_B(int Encoder, int Target)
{
	static float Bias, Pwm, Last_bias;
	Bias = Encoder - Target;
	Pwm += Velocity_KP * (Bias - Last_bias) + Velocity_KI * Bias;
	if (Pwm > 255)
		Pwm = 255;
	if (Pwm < -255)
		Pwm = -255;
	Last_bias = Bias;
	return Pwm;
}

void control()
{
	int Temp, Temp2, Motora, Motorb;
	static float Voltage_All;
	static unsigned char Position_Count, Voltage_Count;
	sei();
	Velocity_Left = Velocity_L;
	Velocity_L = 0;
	Velocity_Right = Velocity_R;
	Velocity_R = 0;
	Get_RC();
	Kinematic_Analysis(Velocity, Angle);
	Motora = Incremental_PI_A(Target_A, Velocity_Left);
	Motorb = Incremental_PI_B(Target_B, Velocity_Right);
	if (Turn_Off() == 0)
		Set_Pwm(Motora, Motorb);
	Temp2 = analogRead(0);
	Voltage_Count++;
	Voltage_All += Temp2;
	if (Voltage_Count == 200)
		Battery_Voltage = Voltage_All * 0.05371 / 2, Voltage_All = 0, Voltage_Count = 0;
	Temp = My_click();
	if (Temp == 1)
		Flag_Stop = !Flag_Stop;
}

void Get_RC(void)
{
	char Yuzhi = 2;
	static float Last_Bias;
	float Bias, LY, RX;
	if (Flag_Direction == 0)  // Stop
		Velocity = 0, Angle = 0;
	else if (Flag_Direction == 1)  // Forward
		Velocity = Bluetooth_Velocity, Angle = 0;
	else if (Flag_Direction == 2)  // Right Forward
		Velocity = Bluetooth_Velocity, Angle = 36;
	else if (Flag_Direction == 3)  // Turn Right
		Velocity = 0, Angle = 0;
	else if (Flag_Direction == 4)  // Right Backward
		Velocity = -Bluetooth_Velocity, Angle = 36;
	else if (Flag_Direction == 5)  // Backward
		Velocity = -Bluetooth_Velocity, Angle = 0;
	else if (Flag_Direction == 6)  // Left Backward
		Velocity = -Bluetooth_Velocity, Angle = -36;
	else if (Flag_Direction == 7)  // Turn Left
		Velocity = 0, Angle = 0;
	else if (Flag_Direction == 8)  // Left Forward
		Velocity = Bluetooth_Velocity, Angle = -36;
	if (Angle < -45)
		Angle = -45;
	if (Angle > 45)
		Angle = 45;
}

void OLED()
{
	char i, t;
	oled.drawstring(00, 00, "SPEED");
	OLED_ShowNumber(45, 00, Bluetooth_Velocity, 3);
	oled.drawstring(00, 01, "RX");
	OLED_ShowNumber(30, 01, Flag_Direction, 3);
	oled.drawstring(00, 02, "EncoLEFT");
	if (Velocity_Left < 0)
		oled.drawstring(80, 02, "-"),
		    OLED_ShowNumber(95, 02, -Velocity_Left, 3);
	else
		oled.drawstring(80, 02, "+"), OLED_ShowNumber(95, 02,
							      Velocity_Left, 3);
	oled.drawstring(00, 03, "EncoRIGHT");
	if (Velocity_Right < 0)
		oled.drawstring(80, 03, "-"),
		    OLED_ShowNumber(95, 03, -Velocity_Right, 3);
	else
		oled.drawstring(80, 03, "+"),
		    OLED_ShowNumber(95, 03, Velocity_Right, 3);
	oled.drawstring(00, 4, "VOLTAGE:");
	oled.drawstring(71, 4, ".");
	oled.drawstring(93, 4, "V");
	OLED_ShowNumber(58, 4, Battery_Voltage / 100, 2);
	OLED_ShowNumber(81, 4, Battery_Voltage % 100, 2);
	if (Flag_Stop == 0)
		oled.drawstring(103, 04, "O-N");
	if (Flag_Stop == 1)
		oled.drawstring(103, 04, "OFF");
	oled.drawstring(80, 05, "S");
	OLED_ShowNumber(95, 05, servo, 3);
	oled.display();
}

void setup()
{
	char error;
	oled.ssd1306_init(SSD1306_SWITCHCAPVCC);
	oled.clear();
	pinMode(AIN1, OUTPUT);
	pinMode(AIN2, OUTPUT);
	pinMode(BIN1, OUTPUT);
	pinMode(BIN2, OUTPUT);
	myservo.attach(SERVO);

	pinMode(ENCODER_L, INPUT);
	pinMode(DIRECTION_L, INPUT);
	pinMode(ENCODER_R, INPUT);
	pinMode(DIRECTION_R, INPUT);
	pinMode(KEY, INPUT);
	delay(200);
	attachInterrupt(0, READ_ENCODER_R, CHANGE);
	attachPinChangeInterrupt(4, READ_ENCODER_L, CHANGE);
	MsTimer2::set(10, control);
	MsTimer2::start();
	Serial.begin(9600);
}

void loop()
{
	static char flag;
	int Voltage_Temp;
	OLED();
	flag = !flag;
	Voltage_Temp = (Battery_Voltage - 740);
	if (Voltage_Temp > 100)
		Voltage_Temp = 100;
	if (Voltage_Temp < 0)
		Voltage_Temp = 0;
	if (PID_Send == 1) {
		Serial.print("{C");
		Serial.print(Bluetooth_Velocity);
		Serial.print(":");
		Serial.print((int)(Velocity_KP * 100));
		Serial.print(":");
		Serial.print((int)(Velocity_KI * 100));
		Serial.print("}$");
		PID_Send = 0;
	} else if (flag == 0) {
		Serial.print("{A");
		Serial.print(abs(Velocity_Left));
		Serial.print(":");
		Serial.print(abs(Velocity_Right));
		Serial.print(":");
		Serial.print(Voltage_Temp);
		Serial.print(":");
		Serial.print(servo - 90);
		Serial.print("}$");
	} else {
		Serial.print("{B");
		Serial.print(servo);
		Serial.print(":");
		Serial.print(Voltage_Temp);
		Serial.print(":");
		Serial.print(Velocity_Left);
		Serial.print(":");
		Serial.print(Velocity_Right);
		Serial.print("}$");
	}
}

void READ_ENCODER_L()
{
	if (digitalRead(ENCODER_L) == LOW) { // Falling Edge
		if (digitalRead(DIRECTION_L) == LOW)
			Velocity_L--;
		else
			Velocity_L++;
	} else { // Rising Edge
		if (digitalRead(DIRECTION_L) == LOW)
			Velocity_L++;
		else
			Velocity_L--;
	}
}

void READ_ENCODER_R()
{
	if (digitalRead(ENCODER_R) == LOW) { // Falling Edge
		if (digitalRead(DIRECTION_R) == LOW)
			Velocity_R++;
		else
			Velocity_R--;
	} else {  // Rising Edge
		if (digitalRead(DIRECTION_R) == LOW)
			Velocity_R--;
		else
			Velocity_R++;
	}
}

void serialEvent()
{
	static unsigned char Flag_PID, Receive[10], Receive_Data, i, j;
	static float Data;
	while (Serial.available()) {
		Receive_Data = Serial.read();
		if (Receive_Data >= 0x41 && Receive_Data <= 0x48)
			Flag_Direction = Receive_Data - 0x40;
		else if (Receive_Data < 10)
			Flag_Direction = Receive_Data;
		else if (Receive_Data == 0X5A)
			Flag_Direction = 0;
		else if (Receive_Data == 'I')
			Flag_Stop = 1;
		if (Flag_Stop)
			Flag_Direction = 0;
		if (Receive_Data == 0x7B)
			Flag_PID = 1;
		if (Receive_Data == 0x7D)
			Flag_PID = 2;
		if (Flag_PID == 1)
			Receive[i] = Receive_Data, i++;
		else if (Flag_PID == 2) {
			if (Receive[3] == 0x50)
				PID_Send = 1;
			//   else  if (Receive[3] == 0x57)    Flash_Send = 1;
			else if (Receive[1] != 0x23) {	// Update PID Parameters
				for (j = i; j >= 4; j--) {
					Data += (Receive[j - 1] - 48) * pow(10, i - j);
				}
				switch (Receive[1]) {
				case 0x30:
					Bluetooth_Velocity = Data;
					break;
				case 0x31:
					Velocity_KP = Data / 100;
					break;
				case 0x32:
					Velocity_KI = Data / 100;
					break;
				case 0x33:
					break;
				case 0x34:
					break;
				case 0x35:
					break;
				case 0x36:
					break;
				case 0x37:
					break;
				case 0x38:
					break;
				}
			}
			Flag_PID = 0;
			i = 0;
			j = 0;
			Data = 0;
		}
	}
}
