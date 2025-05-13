#include <Arduino.h>
#include "PCF8575.h"
PCF8575 pcf8575(0x22);

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
int buttonStatePrevious = HIGH;

static const int buttonPin2 = 25;
int buttonStatePrevious2 = HIGH;

static const int buttonPin3 = 26;
int buttonStatePrevious3 = HIGH;

unsigned long minButtonLongPressDuration = 2000;
unsigned long buttonLongPressUpMillis;
unsigned long buttonLongPressDownMillis;
unsigned long buttonLongPressEnterMillis;
bool buttonStateLongPressUp = false;
bool buttonStateLongPressDown = false;
bool buttonStateLongPressEnter = false;

const int intervalButton = 50;
unsigned long previousButtonMillis;
unsigned long buttonPressDuration;
unsigned long currentMillis;

const int intervalButton2 = 50;
unsigned long previousButtonMillis2;
unsigned long buttonPressDuration2;
unsigned long currentMillis2;

const int intervalButton3 = 50;
unsigned long previousButtonMillis3;
unsigned long buttonPressDuration3;
unsigned long currentMillis3;

unsigned long currentMillisRunAuto;
unsigned long previousMillisRunAuto;
unsigned long intervalRunAuto = 200;

const int rCrustExtruder = P1;
const int rFillingExtruder = P2;
const int rCutter = P3;

Control CrustExtruder(0);
Control FillingExtruder(0);
Control Cutter(0);

Control FillingStopTimer(0);
Control CuttingLengthTimer(0);

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

void InitializeButtons()
{
  pinMode(buttonPin, INPUT);
  pinMode(buttonPin2, INPUT);
  pinMode(buttonPin3, INPUT);
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


void readButtonUpState()
{
  if (currentMillis - previousButtonMillis > intervalButton)
  {
    int buttonState = digitalRead(buttonPin);
    if (buttonState == LOW && buttonStatePrevious == HIGH && !buttonStateLongPressUp)
    {
      buttonLongPressUpMillis = currentMillis;
      buttonStatePrevious = LOW;
    }
    buttonPressDuration = currentMillis - buttonLongPressUpMillis;
    if (buttonState == LOW && !buttonStateLongPressUp && buttonPressDuration >= minButtonLongPressDuration)
    {
      buttonStateLongPressUp = true;
    }
    if (buttonStateLongPressUp == true)
    {
      // Insert Fast Scroll Up
      refreshScreen = true;
      if (settingFlag == true)
      {
        if (settingEditFlag == true)
        {
          if (parametersTimer[currentSettingScreen] >= parametersTimerMaxValue[currentSettingScreen] - 1)
          {
            parametersTimer[currentSettingScreen] = parametersTimerMaxValue[currentSettingScreen];
          }
          else
          {
            parametersTimer[currentSettingScreen] += 10;
          }
        }
        else
        {
          if (currentSettingScreen == NUM_SETTING_ITEMS - 1)
          {
            currentSettingScreen = 0;
          }
          else
          {
            currentSettingScreen++;
          }
        }
      }
      else
      {
        if (currentMainScreen == NUM_MAIN_ITEMS - 1)
        {
          currentMainScreen = 0;
        }
        else
        {
          currentMainScreen++;
        }
      }
    }

    if (buttonState == HIGH && buttonStatePrevious == LOW)
    {
      buttonStatePrevious = HIGH;
      buttonStateLongPressUp = false;
      if (buttonPressDuration < minButtonLongPressDuration)
      {
        // Short Scroll Up
        refreshScreen = true;
        if (settingFlag == true)
        {
          if (settingEditFlag == true)
          {
            if (parametersTimer[currentSettingScreen] >= parametersTimerMaxValue[currentSettingScreen] - 1)
            {
              parametersTimer[currentSettingScreen] = parametersTimerMaxValue[currentSettingScreen];
            }
            else
            {
              parametersTimer[currentSettingScreen] += 10;
            }
          }
          else
          {
            if (currentSettingScreen == NUM_SETTING_ITEMS - 1)
            {
              currentSettingScreen = 0;
            }
            else
            {
              currentSettingScreen++;
            }
          }
        }
        else
        {
          if (currentMainScreen == NUM_MAIN_ITEMS - 1)
          {
            currentMainScreen = 0;
          }
          else
          {
            currentMainScreen++;
          }
        }
      }
    }
    previousButtonMillis = currentMillis;
  }
}

void readButtonDownState()
{
  if (currentMillis2 - previousButtonMillis2 > intervalButton2)
  {
    int buttonState2 = digitalRead(buttonPin2);
    if (buttonState2 == LOW && buttonStatePrevious2 == HIGH && !buttonStateLongPressDown)
    {
      buttonLongPressDownMillis = currentMillis2;
      buttonStatePrevious2 = LOW;
    }
    buttonPressDuration2 = currentMillis2 - buttonLongPressDownMillis;
    if (buttonState2 == LOW && !buttonStateLongPressDown && buttonPressDuration2 >= minButtonLongPressDuration)
    {
      buttonStateLongPressDown = true;
    }
    if (buttonStateLongPressDown == true)
    {
      refreshScreen = true;
      if (settingFlag == true)
      {
        if (settingEditFlag == true)
        {
          if (parametersTimer[currentSettingScreen] <= 0)
          {
            parametersTimer[currentSettingScreen] = 0;
          }
          else
          {
            parametersTimer[currentSettingScreen] -= 10;
          }
        }
        else
        {
          if (currentSettingScreen == 0)
          {
            currentSettingScreen = NUM_SETTING_ITEMS - 1;
          }
          else
          {
            currentSettingScreen--;
          }
        }
      }
      else
      {
        if (currentMainScreen == 0)
        {
          currentMainScreen = NUM_MAIN_ITEMS - 1;
        }
        else
        {
          currentMainScreen--;
        }
      }
    }

    if (buttonState2 == HIGH && buttonStatePrevious2 == LOW)
    {
      buttonStatePrevious2 = HIGH;
      buttonStateLongPressDown = false;
      if (buttonPressDuration2 < minButtonLongPressDuration)
      {
        refreshScreen = true;
        if (settingFlag == true)
        {
          if (settingEditFlag == true)
          {
            if (currentSettingScreen == 2)
            {
              if (parametersTimer[currentSettingScreen] <= 2)
              {
                parametersTimer[currentSettingScreen] = 2;
              }
              else
              {
                parametersTimer[currentSettingScreen] -= 1;
              }
            }
            else
            {
              if (parametersTimer[currentSettingScreen] <= 0)
              {
                parametersTimer[currentSettingScreen] = 0;
              }
              else
              {
                parametersTimer[currentSettingScreen] -= 10;
              }
            }
          }
          else
          {
            if (currentSettingScreen == 0)
            {
              currentSettingScreen = NUM_SETTING_ITEMS - 1;
            }
            else
            {
              currentSettingScreen--;
            }
          }
        }
        else
        {
          if (currentMainScreen == 0)
          {
            currentMainScreen = NUM_MAIN_ITEMS - 1;
          }
          else
          {
            currentMainScreen--;
          }
        }
      }
    }
    previousButtonMillis2 = currentMillis2;
  }
}

void readButtonEnterState()
{
  if (currentMillis3 - previousButtonMillis3 > intervalButton3)
  {
    int buttonState3 = digitalRead(buttonPin3);
    if (buttonState3 == LOW && buttonStatePrevious3 == HIGH && !buttonStateLongPressEnter)
    {
      buttonLongPressEnterMillis = currentMillis3;
      buttonStatePrevious3 = LOW;
    }
    buttonPressDuration3 = currentMillis3 - buttonLongPressEnterMillis;
    if (buttonState3 == LOW && !buttonStateLongPressEnter && buttonPressDuration3 >= minButtonLongPressDuration)
    {
      buttonStateLongPressEnter = true;
    }
    if (buttonStateLongPressEnter == true)
    {
      // Insert Fast Scroll Enter
      Serial.println("Long Press Enter");
    }

    if (buttonState3 == HIGH && buttonStatePrevious3 == LOW)
    {
      buttonStatePrevious3 = HIGH;
      buttonStateLongPressEnter = false;
      if (buttonPressDuration3 < minButtonLongPressDuration)
      {
        refreshScreen = true;
        if (currentMainScreen == 0 && settingFlag == true)
        {
          if (currentSettingScreen == NUM_SETTING_ITEMS - 1)
          {
            settingFlag = false;
            // saveSettings();
            // loadSettings();
            currentSettingScreen = 0;
            // setTimers();
          }
          else
          {
            if (settingEditFlag == true)
            {
              settingEditFlag = false;
            }
            else
            {
              settingEditFlag = true;
            }
          }
        }
        else if (currentMainScreen == 2 && runAutoFlag == true)
        {
          // StopAll();
          // runAutoFlag = false;
          // runAutoStatus = 0;
        }
        else
        {
          if (currentMainScreen == 0)
          {
            settingFlag = true;
          }
          else if (currentMainScreen == 2)
          {
            
          }
        }
      }
    }
    previousButtonMillis3 = currentMillis3;
  }
}

void ReadButtons()
{
  currentMillis = millis();
  currentMillis2 = millis();
  currentMillis3 = millis();
  readButtonEnterState();
  readButtonUpState();
  readButtonDownState();
}

void setup()
{
  initRelays();
  InitializeButtons();
  Serial.begin(9600);
  // put your setup code here, to run once:
}

void loop()
{
  // put your main code here, to run repeatedly:
  pcf8575.digitalWrite(rCrustExtruder, digitalRead(buttonPin));
  pcf8575.digitalWrite(rFillingExtruder, digitalRead(buttonPin2));
  pcf8575.digitalWrite(rCutter, digitalRead(buttonPin3));
  Serial.println(digitalRead(buttonPin));
}
