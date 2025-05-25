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
const int NUM_MAIN_ITEMS = 2;
const int NUM_SETTING_ITEMS = 2;

int currentMainScreen;
int currentSettingScreen;
int currentTestMenuScreen;
bool settingFlag, settingEditFlag, testMenuFlag, runAutoFlag, refreshScreen = false;

String menu_items[NUM_MAIN_ITEMS][2] = { // array with item names
    {"SETTING", "ENTER TO EDIT"},
    {"RUN AUTO", "CLICK TO EXIT"}};

String setting_items[NUM_SETTING_ITEMS][2] = { // array with item names
    {"LENGTH", "MILLIS"},
    {"SAVE"}};

int parametersTimer[NUM_SETTING_ITEMS] = {1};
int parametersTimerMaxValue[NUM_SETTING_ITEMS] = {60000};

unsigned long currentMillisRunAuto;
unsigned long previousMillisRunAuto;
unsigned long intervalRunAuto = 250;

static const int buttonPin = 32;
static const int buttonPin2 = 25;
static const int buttonPin3 = 13;
int buttonStatePrevious = HIGH;
int buttonStatePrevious2 = HIGH;
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

const int rCrustExtruder = P1;
const int rFillingExtruder = P2;
const int rCutter = P3;

Control CrustExtruder(0);
Control FillingExtruder(0);
Control Cutter(0);

Control FillingStopTimer(0);
Control CuttingLengthTimer(0);

ezButton Sensor(27);

bool cutterRunAutoFlag = false;

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

void saveSettings()
{
  Settings.putInt("length", parametersTimer[0]);
  Serial.println("---- Saving Timer  Settings ----");
  Serial.println("Length Time : " + String(parametersTimer[0]));
  Serial.println("---- Saving Timer  Settings ----");
}
void loadSettings()
{
  Serial.println("---- Start Reading Settings ----");
  parametersTimer[0] = Settings.getInt("length");
  Serial.println("Length Timer : " + String(parametersTimer[0]));
  CuttingLengthTimer.setTimer(secondsToHHMMSS(parametersTimer[0]));
  Serial.println("---- End Reading Settings ----");
}

void initButtons()
{
  pinMode(buttonPin, INPUT);
  pinMode(buttonPin2, INPUT);
  pinMode(buttonPin3, INPUT);
  Sensor.setDebounceTime(30);
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
      // Short Scroll Up
      refreshScreen = true;
      if (menuFlag == false)
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
      else
      {
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
              parametersTimer[currentSettingScreen] += 1;
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

    if (buttonState == HIGH && buttonStatePrevious == LOW)
    {
      buttonStatePrevious = HIGH;
      buttonStateLongPressUp = false;
      if (buttonPressDuration < minButtonLongPressDuration)
      {
        // Short Scroll Up
        refreshScreen = true;
        if (menuFlag == false)
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
        else
        {
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
                parametersTimer[currentSettingScreen] += 1;
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
      if (menuFlag == false)
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
      else
      {
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
                parametersTimer[currentSettingScreen] -= 1;
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

    if (buttonState2 == HIGH && buttonStatePrevious2 == LOW)
    {
      buttonStatePrevious2 = HIGH;
      buttonStateLongPressDown = false;
      if (buttonPressDuration2 < minButtonLongPressDuration)
      {
        refreshScreen = true;
        if (menuFlag == false)
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
        else
        {
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
                  parametersTimer[currentSettingScreen] -= 1;
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
      menuFlag = true;
    }

    if (buttonState3 == HIGH && buttonStatePrevious3 == LOW)
    {
      buttonStatePrevious3 = HIGH;
      buttonStateLongPressEnter = false;
      if (buttonPressDuration3 < minButtonLongPressDuration)
      {
        refreshScreen = true;
        if (menuFlag == false)
        {

          if (cutterRunAutoFlag == false)
          {
            cutterRunAutoFlag = true;
          }
          else
          {
            cutterRunAutoFlag = false;
            Cutter.relayOff();
          }
        }
        else
        {
          if (currentMainScreen == 0 && settingFlag == true)
          {
            if (currentSettingScreen == NUM_SETTING_ITEMS - 1)
            {
              settingFlag = false;
              saveSettings();
              loadSettings();
              currentSettingScreen = 0;
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
          else
          {
            if (currentMainScreen == 0)
            {
              settingFlag = true;
            }
            else if (currentMainScreen == 1)
            {
              menuFlag = false;
            }
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
  Sensor.loop();
}

bool initialMoveCutter = false;
void runAuto()
{
  if (menuFlag == false)
  {
    if (CrustExtruder.getMotorState() == true && Cutter.getMotorState() == false)
    {
      pcf8575.digitalWrite(rCrustExtruder, LOW);
    }
    else
    {
      pcf8575.digitalWrite(rCrustExtruder, HIGH);
    }

    if (FillingExtruder.getMotorState() == true && Cutter.getMotorState() == false)
    {
      pcf8575.digitalWrite(rFillingExtruder, LOW);
    }
    else
    {
      pcf8575.digitalWrite(rFillingExtruder, HIGH);
    }

    if (cutterRunAutoFlag == true)
    {

      if (CuttingLengthTimer.isStopped() == false)
      {
        CuttingLengthTimer.run();
        if (CuttingLengthTimer.isTimerCompleted() == true)
        {
          Cutter.relayOn();
          initialMoveCutter = true;
        }
      }
      else
      {
        if (initialMoveCutter == true)
        {
          if (Sensor.getState() == false)
          {
            Cutter.relayOn();
          }
          else
          {
            initialMoveCutter = false;
          }
        }
        else
        {
          if (Sensor.getState() == true)
          {
            Cutter.relayOff();
            CuttingLengthTimer.start();
          }
          else
          {
            Cutter.relayOn();
          }
        }
      }
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

void initializeLCD()
{
  lcd.init();
  lcd.clear();
  lcd.createChar(0, enterChar);
  lcd.createChar(1, fastChar);
  lcd.createChar(2, slowChar);
  lcd.backlight();
  refreshScreen = true;
}
void printSettingScreen(String SettingTitle, String Unit, int Value, bool EditFlag, bool SaveFlag)
{
  lcd.clear();
  lcd.print(SettingTitle);
  lcd.setCursor(0, 1);

  if (SaveFlag == true)
  {
    lcd.setCursor(0, 3);
    lcd.write(0);
    lcd.setCursor(2, 3);
    lcd.print("ENTER TO SAVE ALL");
  }
  else
  {
    lcd.print(Value);
    lcd.print(" ");
    lcd.print(Unit);
    lcd.setCursor(0, 3);
    lcd.write(0);
    lcd.setCursor(2, 3);
    if (EditFlag == false)
    {
      lcd.print("ENTER TO EDIT");
    }
    else
    {
      lcd.print("ENTER TO SAVE");
    }
  }
  refreshScreen = false;
}
void printMainMenu(String MenuItem, String Action)
{
  lcd.clear();
  lcd.print(MenuItem);
  lcd.setCursor(0, 3);
  lcd.write(0);
  lcd.setCursor(2, 3);
  lcd.print(Action);
  refreshScreen = false;
}

void printRunAuto(bool Motor1, bool Motor2, bool Motor3, bool Sensor, String TimeRemaining, long count)
{
  lcd.clear();
  if (Motor1 == true)
  {
    lcd.print("Extruder : ON");
  }
  else
  {
    lcd.print("Extruder : OFF");
  }

  lcd.setCursor(0, 1);
  if (Motor2 == true)
  {
    lcd.print("Filler : ON");
  }
  else
  {
    lcd.print("Filler : OFF");
  }

  lcd.setCursor(0, 2);
  if (Motor3 == true)
  {
    lcd.print("Cutter : ON");
  }
  else
  {
    lcd.print("Cutter : OFF");
  }
  lcd.setCursor(0, 3);
  lcd.print(TimeRemaining);

  lcd.setCursor(15, 3);
  lcd.print(count);
  refreshScreen = false;
}

void printScreen()
{
  if (menuFlag == false)
  {

    printRunAuto(CrustExtruder.getMotorState(), FillingExtruder.getMotorState(), Cutter.getMotorState(), false, CuttingLengthTimer.getTimeRemaining(), Sensor.getCount());
    refreshScreen = false;
  }
  else
  {
    if (settingFlag == true)
    {
      if (currentSettingScreen == NUM_SETTING_ITEMS - 1)
      {
        printSettingScreen(setting_items[currentSettingScreen][0], setting_items[currentSettingScreen][1], parametersTimer[currentSettingScreen], settingEditFlag, true);
      }
      else
      {
        printSettingScreen(setting_items[currentSettingScreen][0], setting_items[currentSettingScreen][1], parametersTimer[currentSettingScreen], settingEditFlag, false);
      }
    }
    else
    {
      printMainMenu(menu_items[currentMainScreen][0], menu_items[currentMainScreen][1]);
    }
  }
}

void setup()
{
  initRelays();
  initButtons();
  Serial.begin(115200);
  Settings.begin("timerSetting", false);
  // saveSettings();
  loadSettings();
  initializeLCD();
}

void loop()
{
  ReadButtons();
  runAuto();
  if (refreshScreen == true)
  {
    printScreen();
  }

  if (menuFlag == false)
  {
    unsigned long currentMillisRunAuto = millis();
    if (currentMillisRunAuto - previousMillisRunAuto >= intervalRunAuto)
    {
      previousMillisRunAuto = currentMillisRunAuto;
      refreshScreen = true;
      Serial.println("Refresh");
    }
  }
}
