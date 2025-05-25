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
unsigned long intervalRunAuto = 1000;

static const int buttonPin = 32;
static const int buttonPin2 = 25;
static const int buttonPin3 = 13;

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
  button3.setDebounceTime(100);
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
  button1.loop();
  button2.loop();
  button3.loop();
  if (button1.isReleased() && button2.isReleased() && button3.isReleased())
  {
    menuFlag = true;
    stopAll();
    refreshScreen = true;
  }

  if (menuFlag == true)
  {
    if (button1.isReleased())
    {
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

    if (button2.isReleased())
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
            parametersTimer[currentSettingScreen] -= 1;
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

    if (button3.isReleased())
    {
      refreshScreen = true;
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
  else
  {
    if (button1.isPressed())
    {
      // Serial.println("Button 1 is Released");
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
      // Serial.println("Button 2 is Released");
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
      // Serial.println("Button 3 is Released");
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

void printRunAuto(bool Motor1, bool Motor2, bool Motor3, bool Sensor, String TimeRemaining)
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
  if (Motor2 == true)
  {
    lcd.print("Cutter : ON");
  }
  else
  {
    lcd.print("Cutter : OFF");
  }

  lcd.setCursor(0, 2);
  lcd.print(TimeRemaining);
  refreshScreen = false;
}

void printScreen()
{
  if (menuFlag == true)
  {
    printRunAuto(CrustExtruder.getMotorState(), FillingExtruder.getMotorState(), Cutter.getMotorState(), false, CuttingLengthTimer.getTimeRemaining());
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

void setup()
{
  initRelays();
  initButtons();
  Serial.begin(115200);
  Settings.begin("timerSetting", false);
  saveSettings();
  loadSettings();
  initializeLCD();
}

void loop()
{
  readButton();
  runAuto();
  if (menuFlag == false)
  {
    unsigned long currentMillisRunAuto = millis();
    if (currentMillisRunAuto - previousMillisRunAuto >= intervalRunAuto)
    {
      previousMillisRunAuto = currentMillisRunAuto;
      refreshScreen = true;
    }
  }
}
