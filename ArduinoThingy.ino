#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Sodaq_DS3231.h"
#include <Fonts/FreeMono9pt7b.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

char weekDay[][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

int switch1 = 2;
int switch2 = 3;
int switch3 = 4;
int ledPWM = 6;
int fanPWM = 10;

int state = 0;
int maxState = 3;

unsigned long currentMillis;
unsigned long millisDiff;
unsigned long savedMillisTemp = 0;
unsigned long savedMillis = 0;

boolean timeAction = false;
boolean allowStateChange = true;

int ledValuePercent = 0;
boolean ledActivated = false;
int fanValuePercent = 0;
boolean fanActivated = false;

int ThermistorPin = 0;
float Vo;
static float R1 = 10000;
float logR2, R2, T;
static float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;

void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  rtc.begin();

  pinMode(switch1 , INPUT);
  pinMode(switch2 , INPUT);
  pinMode(switch3 , INPUT);
  pinMode(ledPWM , OUTPUT);

  display.display();

  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(10, 0);
  display.clearDisplay();
  Serial.begin(9600);

  //setPwmFrequency(ledPWM, 1024);
  setPwmFrequency(fanPWM, 1024);
}

void loop() {

  currentMillis = millis();

  if (state == 0)
  {
    screen0();
  }
  else if (state == 1 )
  {
    screen1();
    ledDispState();
  }
  else if (state == 2)
  {
    screen2();
    fanDispState();
  }

  if (digitalRead(switch2) == 1 && allowStateChange == true && (currentMillis - savedMillis) >= 500UL)
  {
    state++;
    changeState();
  }
  if (digitalRead(switch1) == 1 && allowStateChange == true && (currentMillis - savedMillis) >= 500UL)
  {
    state--;
    changeState();
  }

  if (ledActivated == true && (currentMillis - savedMillis) >= 200UL )
  {
    ledValueChange();
    timeAction = true;
  }

  if (fanActivated == true && (currentMillis - savedMillis) >= 200UL )
  {
    fanValueChange();
    timeAction = true;
  }

  display.display();

  if (timeAction == true)
  {
    savedMillis = currentMillis;
    timeAction = false;
  }

}

void screen0()
{
  String dateS;

  DateTime now = rtc.now();
  display.clearDisplay();
  display.setCursor(10, 0);
  display.print(now.hour());
  display.print(":");
  display.print(now.minute());
  display.print(":");
  display.print(now.second());
  dateS += now.date();
  dateS += '/';
  dateS += now.month();
  dateS += '/';
  dateS += now.year();
  //display.setCursor(63 - dateS.length() * 6, 20);
  //displayCentered(dateS , 20 , 6);
  display.print(dateS);
  display.println();
  display.setCursor(10, 40);

  if ((currentMillis - savedMillisTemp) >= 500UL)
  {
    Vo = analogRead(ThermistorPin);
    R2 = R1 * (1023.0 / Vo - 1.0);
    logR2 = log(R2);
    T = (1.0 / (c1 + c2 * logR2 + c3 * logR2 * logR2 * logR2)) - 273.15;
    savedMillisTemp = currentMillis;
  }

  display.print(T);
  display.print((char)247);
  display.print("C");
}

void screen1()
{
  display.clearDisplay();
  display.setCursor(10, 0);
  display.print("LED: ");
  display.print(ledValuePercent);
  display.print("%");
  if (ledActivated == true)
  {
    display.print("_");
  }

}

void screen2()
{
  display.setFont(&FreeMono9pt7b);
  display.setTextSize(1);
  rtc.convertTemperature();
  display.clearDisplay();
  display.setCursor(0, 9);
  display.println("Temp Inside: ");
  display.setCursor(63 - (7*5), 26);
  display.print(rtc.getTemperature());
  display.setFont();
  display.print((char)247);
  display.setFont(&FreeMono9pt7b);
  display.print("C");
  display.setCursor(63 - (10*5), 42);
  display.print("Fan Speed: ");
  if(fanValuePercent == 0)
  {
    display.setCursor(63 - (2*5), 58);
  }
  else if(fanValuePercent != 0 and fanValuePercent != 100)
  {
    display.setCursor(63 - (3*5), 58);
  }
  else if(fanValuePercent == 100)
  {
    display.setCursor(63 - (4*5), 60);
  }
  display.print(fanValuePercent);
  display.print("%");
  if (fanActivated == true)
  {
    display.print("_");
  }
  display.setTextSize(2);
  display.setFont();
  
}

void changeState()
{
  if (state >= maxState)
    state = 0;
  else if (state < 0)
    state = maxState - 1;
  timeAction = true;
}

void ledValueChange()
{
  int ledValue;
  if (digitalRead(switch2) == 1 && ledValuePercent != 100) ledValuePercent += 10;
  if (digitalRead(switch1) == 1 && ledValuePercent != 0) ledValuePercent -= 10;
  ledValue = map(ledValuePercent , 0 , 100 , 0 , 255);
  analogWrite(ledPWM , ledValue);
}

void fanValueChange()
{
  int fanValue;
  if (digitalRead(switch2) == 1 && fanValuePercent != 100) fanValuePercent += 10;
  if (digitalRead(switch1) == 1 && fanValuePercent != 0) fanValuePercent -= 10;

  if (fanValuePercent == 0)
  {
    digitalWrite(fanPWM , LOW);
  }
  else
  {
    fanValue = map(fanValuePercent , 0 , 100 , 77 , 255);
    analogWrite(fanPWM , fanValue);
  }


}

void ledDispState()
{
  if (digitalRead(switch3) == HIGH && (currentMillis - savedMillis) >= 200)
  {
    if (ledActivated == true)ledActivated = false;
    else ledActivated = true;
    if (allowStateChange == true)allowStateChange = false;
    else allowStateChange = true;
    timeAction = true;
  }
}

void fanDispState()
{
  if (digitalRead(switch3) == HIGH && (currentMillis - savedMillis) >= 200)
  {
    if (fanActivated == true)fanActivated = false;
    else fanActivated = true;
    if (allowStateChange == true)allowStateChange = false;
    else allowStateChange = true;
    timeAction = true;
  }
}

void displayCentered(char text , int y, int multiplier)
{
  display.setCursor(63 - strlen(text) * multiplier, y);
  display.print(text);
}

void setPwmFrequency(int pin, int divisor) {
  byte mode;
  if (pin == 5 || pin == 6 || pin == 9 || pin == 10) {
    switch (divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 64: mode = 0x03; break;
      case 256: mode = 0x04; break;
      case 1024: mode = 0x05; break;
      default: return;
    }
    if (pin == 5 || pin == 6) {
      TCCR0B = TCCR0B & 0b11111000 | mode;
    } else {
      TCCR1B = TCCR1B & 0b11111000 | mode;
    }
  } else if (pin == 3 || pin == 11) {
    switch (divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 32: mode = 0x03; break;
      case 64: mode = 0x04; break;
      case 128: mode = 0x05; break;
      case 256: mode = 0x06; break;
      case 1024: mode = 0x07; break;
      default: return;
    }
    TCCR2B = TCCR2B & 0b11111000 | mode;
  }
}
