#include <Arduino.h>
#include "PCF8575.h"
PCF8575 pcf8575(0x22);
#include <ezButton.h>

// Declaration for LCD
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 20, 4);

#include "control.h"
#include <Preferences.h>
Preferences Settings;

byte enterChar[] = {
    B10000,
    B10000,
    B10100,
    B10110,
    B11111,
    B00110,
    B00100,
    B00000};

byte fastChar[] = {
    B00110,
    B01110,
    B00110,
    B00110,
    B01111,
    B00000,
    B00100,
    B01110};
byte slowChar[] = {
    B00011,
    B00111,
    B00011,
    B11011,
    B11011,
    B00000,
    B00100,
    B01110};
// Declaration of LCD Variables
const int NUM_MAIN_ITEMS = 1;
const int NUM_SETTING_ITEMS = 2;

int currentMainScreen;
int currentSettingScreen;
int currentTestMenuScreen;
bool settingFlag, settingEditFlag, testMenuFlag, runAutoFlag, refreshScreen = false;

String menu_items[NUM_MAIN_ITEMS][2] = { // array with item names
    {"SETTING", "ENTER TO EDIT"}};

String setting_items[NUM_SETTING_ITEMS][2] = { // array with item names
    {"LENGTH", "MILLIS"},
    {"SAVE"}};

int parametersTimer[NUM_SETTING_ITEMS] = {1};
int parametersTimerMaxValue[NUM_SETTING_ITEMS] = {60000};

static const int buttonPin = 32;
static const int buttonPin2 = 25;
static const int buttonPin3 = 26;

ezButton button1(buttonPin, EXTERNAL_PULLUP);
ezButton button2(buttonPin2, EXTERNAL_PULLUP);
ezButton button3(buttonPin3, EXTERNAL_PULLUP);

const int rCrustExtruder = P1;
const int rFillingExtruder = P2;
const int rCutter = P3;

Control CrustExtruder(0);
Control FillingExtruder(0);
Control Cutter(0);

Control FillingStopTimer(0);
Control CuttingLengthTimer(0);

void initButtons()
{
  button1.setDebounceTime(100);
  button2.setDebounceTime(100);
  button2.setDebounceTime(100);
}

void stopAll()
{
  Cutter.relayOff();
  FillingExtruder.relayOff();
  CrustExtruder.relayOff();
  pcf8575.digitalWrite(rCrustExtruder, HIGH);
  pcf8575.digitalWrite(rFillingExtruder, HIGH);
  pcf8575.digitalWrite(rCutter, HIGH);
}

bool menuFlag = false;

void readButton()
{
  if (button1.isReleased() && button2.isReleased() && button3.isReleased())
  {
    menuFlag = true;
    stopAll();
  }

  if (menuFlag == true)
  {
    // Menu Todo
  }
  else
  {
    if (button1.isPressed())
    {
      if (CrustExtruder.getMotorState() == true)
      {
        CrustExtruder.relayOff();
      }
      else
      {
        CrustExtruder.relayOn();
      }
    }

    if (button2.isPressed())
    {
      if (FillingExtruder.getMotorState() == true)
      {
        FillingExtruder.relayOff();
      }
      else
      {
        FillingExtruder.relayOn();
      }
    }

    if (button3.isPressed())
    {
      if (Cutter.getMotorState() == true)
      {
        Cutter.relayOff();
      }
      else
      {
        Cutter.relayOn();
      }
    }
  }
}

void runAuto()
{
  if (menuFlag == false)
  {
    if (CrustExtruder.getMotorState() == true)
    {
      pcf8575.digitalWrite(rCrustExtruder, LOW);
    }
    else
    {
      pcf8575.digitalWrite(rCrustExtruder, HIGH);
    }

    if (FillingExtruder.getMotorState() == true)
    {
      pcf8575.digitalWrite(rFillingExtruder, LOW);
    }
    else
    {
      pcf8575.digitalWrite(rFillingExtruder, HIGH);
    }

    if (Cutter.getMotorState() == true)
    {
      pcf8575.digitalWrite(rCutter, LOW);
    }
    else
    {
      pcf8575.digitalWrite(rCutter, HIGH);
    }
  }
  else
  {
  }
}

void initRelays()
{
  pcf8575.pinMode(rCrustExtruder, OUTPUT);
  pcf8575.digitalWrite(rCrustExtruder, LOW);

  pcf8575.pinMode(rFillingExtruder, OUTPUT);
  pcf8575.digitalWrite(rFillingExtruder, LOW);

  pcf8575.pinMode(rCutter, OUTPUT);
  pcf8575.digitalWrite(rCutter, LOW);

  pcf8575.begin();
}

void StopAll()
{
  CrustExtruder.stop();
  FillingExtruder.stop();
  Cutter.stop();

  FillingStopTimer.stop();
  CuttingLengthTimer.stop();

  pcf8575.digitalWrite(rCrustExtruder, HIGH);
  pcf8575.digitalWrite(rFillingExtruder, HIGH);
  pcf8575.digitalWrite(rCutter, HIGH);
}

char *secondsToHHMMSS(int total_seconds)
{
  int hours, minutes, seconds;

  hours = total_seconds / 3600;         // Divide by number of seconds in an hour
  total_seconds = total_seconds % 3600; // Get the remaining seconds
  minutes = total_seconds / 60;         // Divide by number of seconds in a minute
  seconds = total_seconds % 60;         // Get the remaining seconds

  // Format the output string
  static char hhmmss_str[7]; // 6 characters for HHMMSS + 1 for null terminator
  sprintf(hhmmss_str, "%02d%02d%02d", hours, minutes, seconds);
  return hhmmss_str;
}

void setup()
{
  initRelays();
  initButtons();
  Serial.begin(9600);
  // put your setup code here, to run once:
}

void loop()
{
  // put your main code here, to run repeatedly:
  readButton();
  runAuto();
}
