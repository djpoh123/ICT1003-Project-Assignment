#include <STBLE.h>
#include <Wire.h>         // For I2C communication with sensor
#include <SPI.h>
#include <TinyScreen.h>
#include "BMA250.h"       // For interfacing with the accel. sensor

// Accelerometer sensor variables for the sensor and its values
int i = 0, z; //i is crash counter, z is direction of screen

//////////////////////////
// Bluetooth portion   //

//Debug output adds extra flash and memory requirements!
#ifndef BLE_DEBUG
#define BLE_DEBUG true
#endif

#if defined (ARDUINO_ARCH_AVR)
#define SerialMonitorInterface Serial
#elif defined(ARDUINO_ARCH_SAMD)
#define SerialMonitorInterface SerialUSB
#endif


//uint8_t ble_rx_buffer[21];
//uint8_t ble_rx_buffer_len = 0;
//uint8_t ble_connection_state = false;
#define PIPE_UART_OVER_BTLE_UART_TX_TX 0

/////endof bluetooh//////

//TinyScreen display = TinyScreen(TinyScreenDefault);

#if defined(ARDUINO_ARCH_SAMD)
#define SerialMonitorInterface SerialUSB
#else
#define SerialMonitorInterface Serial
#endif


typedef struct
{
  const uint8_t amtLines;
  const char* const * strings;
  void (*selectionHandler)(uint8_t);
} menu_info;


uint8_t menuHistory[5];
uint8_t menuHistoryIndex = 0;
uint8_t currentMenu = 0;
uint8_t currentMenuLine = 0;
uint8_t lastMenuLine = -1;
uint8_t currentSelectionLine = 0;
uint8_t lastSelectionLine = -1;
uint8_t cyclingModeState = 0;
uint8_t crash = 0;

void newMenu(int8_t newIndex) {
  currentMenuLine = 0;
  lastMenuLine = -1;
  currentSelectionLine = 0;
  lastSelectionLine = -1;
  if (newIndex >= 0) {
    menuHistory[menuHistoryIndex++] = currentMenu;
    currentMenu = newIndex;
  } else {
    if (currentDisplayState == displayStateMenu) {
      menuHistoryIndex--;
      currentMenu = menuHistory[menuHistoryIndex];
    }
  }
  if (menuHistoryIndex) {
    currentDisplayState = displayStateMenu;
    if (menu_debug_print)SerialMonitorInterface.print("New menu index ");
    if (menu_debug_print)SerialMonitorInterface.println(currentMenu);
  } else {
    if (menu_debug_print)SerialMonitorInterface.print("New menu index ");
    if (menu_debug_print)SerialMonitorInterface.println("home");
    currentDisplayState = displayStateHome;
    initHomeScreen();
  }
}

static const char PROGMEM mainMenuStrings0[] = "Cycling Mode";
static const char PROGMEM mainMenuStrings1[] = "Temperature Sensor";
static const char PROGMEM mainMenuStrings2[] = "Set date/time";
static const char PROGMEM mainMenuStrings3[] = "Set auto off";
static const char PROGMEM mainMenuStrings4[] = "Set brightness";

static const char* const PROGMEM mainMenuStrings[] =
{
  mainMenuStrings0,
  mainMenuStrings1,
  mainMenuStrings2,
  mainMenuStrings3,
  mainMenuStrings4,
};

const menu_info mainMenuInfo =
{
  5,
  mainMenuStrings,
  mainMenu,
};


static const char PROGMEM dateTimeMenuStrings0[] = "Set Year";
static const char PROGMEM dateTimeMenuStrings1[] = "Set Month";
static const char PROGMEM dateTimeMenuStrings2[] = "Set Day";
static const char PROGMEM dateTimeMenuStrings3[] = "Set Hour";
static const char PROGMEM dateTimeMenuStrings4[] = "Set Minute";
static const char PROGMEM dateTimeMenuStrings5[] = "Set Second";

static const char* const PROGMEM dateTimeMenuStrings[] =
{
  dateTimeMenuStrings0,
  dateTimeMenuStrings1,
  dateTimeMenuStrings2,
  dateTimeMenuStrings3,
  dateTimeMenuStrings4,
  dateTimeMenuStrings5,
};

const menu_info dateTimeMenuInfo =
{
  6,
  dateTimeMenuStrings,
  dateTimeMenu,
};

const menu_info menuList[] = {mainMenuInfo, dateTimeMenuInfo};
#define mainMenuIndex 0
#define dateTimeMenuIndex 1

int currentVal = 0;
int digits[4];
int currentDigit = 0;
int maxDigit = 4;
int *originalVal;
void (*editIntCallBack)() = NULL;

uint8_t editInt(uint8_t button, int *inVal, char *intName, void (*cb)()) {
  if (menu_debug_print)SerialMonitorInterface.println("editInt");
  if (!button) {
    if (menu_debug_print)SerialMonitorInterface.println("editIntInit");
    editIntCallBack = cb;
    currentDisplayState = displayStateEditor;
    editorHandler = editInt;
    currentDigit = 0;
    originalVal = inVal;
    currentVal = *originalVal;
    digits[3] = currentVal % 10; currentVal /= 10;
    digits[2] = currentVal % 10; currentVal /= 10;
    digits[1] = currentVal % 10; currentVal /= 10;
    digits[0] = currentVal % 10;
    currentVal = *originalVal;
    display.clearWindow(0, 12, 96, 64);
    display.setFont(font10pt);
    display.fontColor(defaultFontColor, defaultFontBG);
    display.setCursor(0, menuTextY[0]);
    display.print(F("< back/undo"));
    display.setCursor(90, menuTextY[0]);
    display.print('^');
    display.setCursor(10, menuTextY[1]);
    display.print(intName);
    display.setCursor(0, menuTextY[3]);
    display.print(F("< next/save"));
    display.setCursor(90, menuTextY[3]);
    display.print('v');
  } else if (button == upButton) {
    if (digits[currentDigit] < 9)
      digits[currentDigit]++;
  } else if (button == downButton) {
    if (digits[currentDigit] > 0)
      digits[currentDigit]--;
  } else if (button == selectButton) {
    if (currentDigit < maxDigit - 1) {
      currentDigit++;
    } else {
      //save
      int newValue = (digits[3]) + (digits[2] * 10) + (digits[1] * 100) + (digits[0] * 1000);
      *originalVal = newValue;
      viewMenu(backButton);
      if (editIntCallBack) {
        editIntCallBack();
        editIntCallBack = NULL;
      }
      return 1;
    }
  } else if (button == backButton) {
    if (currentDigit > 0) {
      currentDigit--;
    } else {
      if (menu_debug_print)SerialMonitorInterface.println(F("back"));
      viewMenu(backButton);
      return 0;
    }
  }
  display.setCursor(10, menuTextY[2]);
  for (uint8_t i = 0; i < 4; i++) {
    if (i != currentDigit)display.fontColor(inactiveFontColor, defaultFontBG);
    display.print(digits[i]);
    if (i != currentDigit)display.fontColor(defaultFontColor, defaultFontBG);
  }
  display.print(F("   "));
  return 0;
}

void showTemp () {
  double temp;

  display.clearWindow(0, 12, 96, 64);

  while (1) {
    accel_sensor.read();//This function gets new data from the acccelerometer
    temp = ((accel_sensor.rawTemp * 0.5) + 22.0); // Temperature Reading

    display.setFont(font10pt);
    display.fontColor(defaultFontColor, defaultFontBG);

    display.setCursor(0, menuTextY[0]);
    display.print(F("< back"));

    // Message Prompt Condition
    if (temp > 35) {
          display.setFont(font10pt);
          display.fontColor(TS_8b_Red, defaultFontBG);

          display.setCursor(17, menuTextY[1]);
          display.print(F("Current Temp"));

          display.setCursor(30, menuTextY[2]);
          display.print(temp);

          display.setFont(font10pt);
          display.fontColor(TS_8b_Yellow, defaultFontBG);

          display.setCursor(13, menuTextY[3]);
          display.print("Please Hydrate!");
        }
        else {
          display.setFont(font10pt);
          display.fontColor(TS_8b_Green, defaultFontBG);

          display.setCursor(17, menuTextY[1]);
          display.print(F("Current Temp"));

          display.setCursor(30, menuTextY[2]);
          display.print(temp);

          display.fontColor(defaultFontColor, defaultFontBG);
          display.setCursor(10, menuTextY[3]);
          display.print("               ");
        }

    delay(100);

    // While Loop Exit Condition
    if (display.getButtons(TSButtonUpperLeft)) {
      break;
    }
  }
  currentDisplayState = displayStateHome;
  initHomeScreen();
}

void crashDetector() { //////////////////////////////////////////////////////////////////////////////////////////////////////////////////       crash is here
  delay(1000);
  accel_sensor.read();//This function gets new data from the acccelerometer
  SerialMonitorInterface.print("\nDetecting for crash:");
  SerialMonitorInterface.print(i);
  SerialMonitorInterface.print("    Z =  ");
  SerialMonitorInterface.print(z);
  if (display.getButtons(TSButtonUpperLeft)) {
    display.clearScreen();
    cyclingModeState = 0;
    i = 0;
  }
  z = accel_sensor.Z;

  //Value of z is where the screen is facing, if not facing up properly then add 1 to crash detector(i)
  if (z < 210) {
    i += 1;
    if (i == 8) {
      //call function asking if rider is ok, if ok return back here, if not ok sound alarm and send notification to phone
      SerialMonitorInterface.print("\nCRASH DETECTED");
      display.println("Crash?");
      display.clearScreen();
      crashUI();
      i = 0;
    }
  }
  else {
    i = 0;
  }
}

void crashUI() {
  //  display.clearWindow(0, 12, 96, 64);
  display.clearScreen();
  //start timer loop for 15 seconds
  int i = 15;
  //width from the right for '10'
  int j = 6;
  unsigned long startTime = millis();
  while (millis() - startTime < 15000) {
    // set the font, color and cursor and print question, buttons, time
    display.setFont(liberationSansNarrow_12ptFontInfo);
    display.fontColor(TS_8b_White, TS_8b_Black);
    display.setCursor(0, 16);
    display.println("Are you okay?");

    display.setFont(thinPixel7_10ptFontInfo);
    display.fontColor(TS_8b_Green, TS_8b_Black);
    display.setCursor(0, 48);
    display.println("Okay");
    display.fontColor(TS_8b_Red, TS_8b_Black);
    display.setCursor(72, 48);
    display.println("Help");

    display.setFont(liberationSansNarrow_12ptFontInfo);
    display.fontColor(TS_8b_White, TS_8b_Black);

    //set cursor to be based from the right and print width of i
    display.setCursor(48 - j, 32);
    //print timer
    display.println(i);

    delay(1000);
    // one second later or interrupt: break, or clear and decrement
    if (display.getButtons(TSButtonLowerLeft)) {
      break;
    }
    else if (display.getButtons(TSButtonLowerRight)) {
      i = 0;
      break;
    }
    else {
      display.clearWindow(48 - j, 32, 12, 12);
      i -= 1;
      if (i < 10) {
        j = 0;
      }
    }
  }
  //timer run out or interrupt
  if (i == 0) {
    crashNotOkay();
  }
  else {
    crashOkay();
  }
}

void crashOkay() {
  display.clearScreen();
  //set the font, color and cursor and print question
  display.setFont(thinPixel7_10ptFontInfo);
  display.fontColor(TS_8b_Red, TS_8b_Black);
  display.setCursor(0, 16);
  display.println("It's not like I was");
  display.setCursor(0, 30);
  display.println("worried about you..");
  delay(2000);
  display.clearScreen();
  display.setFont(liberationSansNarrow_16ptFontInfo);
  display.setCursor(24, 24);
  display.println("BAKA!");
  delay(2000);
  display.clearScreen();
  display.setFont(thinPixel7_10ptFontInfo);
  display.fontColor(TS_8b_White, TS_8b_Black);
  cyclingModeState = 0;
  i = 0;
  display.clearScreen();
  //  initHomeScreen();
  currentDisplayState = displayStateHome;
  cyclingModeState = 0;
}

void crashNotOkay() {
  display.clearScreen();

  //send message to friends phone to get friends' help
  SendMessage("EMERGENCY!");
  SendMessage("RIDER CRASHED!");
  SendMessage("ASSIST RIDER!!!");

  // break upon pressing okay
  while (!display.getButtons(TSButtonLowerLeft)) {
    // flash
    display.clearWindow(0, 12, 96, 64);
    display.drawRect(0, 12, 96, 64, TSRectangleFilled, TS_8b_Red);
    delay(100);
    display.clearWindow(0, 12, 96, 64);

    //set the font, color and cursor and print message
    display.setFont(thinPixel7_10ptFontInfo);
    display.fontColor(TS_8b_White, TS_8b_Black);
    display.setFont(liberationSansNarrow_12ptFontInfo);
    display.setCursor(16, 16);
    display.println("I'm calling");
    display.setCursor(20, 34);
    display.println("for help.");
    delay(100);
  }
  // Break Condition
  display.clearScreen();
  //  initHomeScreen();
  currentDisplayState = displayStateHome;
  cyclingModeState = 0;
}

void SendMessage(char* data)
{
  if (data != NULL) //check if data is NULL
  {
    uint8_t data_convert[strlen(data)]; //define data_convert as unsigned char with the size of data
    memcpy(data_convert, data, strlen(data)); //copy the value from data variable to the data_convert variable
    lib_aci_send_data(PIPE_UART_OVER_BTLE_UART_TX_TX, (uint8_t*)data_convert, sizeof(data_convert));
  }
}

void mainMenu(uint8_t selection) {
  if (menu_debug_print)SerialMonitorInterface.println("mainMenuHandler");///////////////////////////////////////////////////////////////////////////////////// menu is here
  if (selection == 0) { // Cycling Mode
    // the intention is set an ok sign
    if (cyclingModeState == 0) {
      cyclingModeState = 1;
      display.clearScreen();
      while ( cyclingModeState == 1) {
        display.fontColor(defaultFontColor, inactiveFontBG);
        display.setCursor(16, 12);
        display.setFont(liberationSansNarrow_10ptFontInfo);
        display.print("Cycle mode");
        /// Insertion of Temp Monitoring Code
        double temp = 0;
        temp = ((accel_sensor.rawTemp * 0.5) + 22.0); // Temperature Reading

        // Message Prompt Condition
        if (temp > 35) {
          display.setFont(font10pt);
          display.fontColor(TS_8b_Red, defaultFontBG);

          display.setCursor(17, menuTextY[1]);
          display.print(F("Current Temp"));

          display.setCursor(30, menuTextY[2]);
          display.print(temp);

          display.setFont(font10pt);
          display.fontColor(TS_8b_Yellow, defaultFontBG);

          display.setCursor(13, menuTextY[3]);
          display.print("Please Hydrate!");
        }
        else {
          display.setFont(font10pt);
          display.fontColor(TS_8b_Green, defaultFontBG);

          display.setCursor(17, menuTextY[1]);
          display.print(F("Current Temp"));

          display.setCursor(30, menuTextY[2]);
          display.print(temp);

          display.fontColor(defaultFontColor, defaultFontBG);
          display.setCursor(10, menuTextY[3]);
          display.print("               ");
        }
        /// Insertion of Temp Monitoring Code
        crashDetector();
      }
    }
    else {
      cyclingModeState = 0;
    }
    // access the same page while removing history of doing so
    newMenu(0);
    menuHistoryIndex--;
    currentMenu = menuHistory[menuHistoryIndex];
  }

  if (selection == 1) { // Temperature Monitoring
    showTemp(); // <--- Insert your function here Temperature Monitoring
  }

  if (selection == 2) {
    newMenu(dateTimeMenuIndex);
  }
  if (selection == 3) {
    char buffer[20];
    strcpy_P(buffer, (PGM_P)pgm_read_word(&(menuList[mainMenuIndex].strings[selection])));
    editInt(0, &sleepTimeout, buffer, NULL);
  }
  if (selection == 4) {
    char buffer[20];
    strcpy_P(buffer, (PGM_P)pgm_read_word(&(menuList[mainMenuIndex].strings[selection])));
    editInt(0, &brightness, buffer, NULL);
  }
}


uint8_t dateTimeSelection = 0;
int dateTimeVariable = 0;

void saveChangeCallback() {
#if defined (ARDUINO_ARCH_AVR)
  int timeData[] = {year(), month(), day(), hour(), minute(), second()};
  timeData[dateTimeSelection] = dateTimeVariable;
  setTime(timeData[3], timeData[4], timeData[5], timeData[2], timeData[1], timeData[0]);
#elif defined(ARDUINO_ARCH_SAMD)
  int timeData[] = {RTCZ.getYear(), RTCZ.getMonth(), RTCZ.getDay(), RTCZ.getHours(), RTCZ.getMinutes(), RTCZ.getSeconds()};
  timeData[dateTimeSelection] = dateTimeVariable;
  RTCZ.setTime(timeData[3], timeData[4], timeData[5]);
  RTCZ.setDate(timeData[2], timeData[1], timeData[0] - 2000);
#endif
  if (menu_debug_print)SerialMonitorInterface.print("set time ");
  if (menu_debug_print)SerialMonitorInterface.println(dateTimeVariable);
}


void dateTimeMenu(uint8_t selection) {
  if (menu_debug_print)SerialMonitorInterface.print("dateTimeMenu ");
  if (menu_debug_print)SerialMonitorInterface.println(selection);
  if (selection >= 0 && selection < 6) {
#if defined (ARDUINO_ARCH_AVR)
    int timeData[] = {year(), month(), day(), hour(), minute(), second()};
#elif defined(ARDUINO_ARCH_SAMD)
    int timeData[] = {RTCZ.getYear(), RTCZ.getMonth(), RTCZ.getDay(), RTCZ.getHours(), RTCZ.getMinutes(), RTCZ.getSeconds()};
#endif
    dateTimeVariable = timeData[selection];
    dateTimeSelection = selection;
    char buffer[20];
    strcpy_P(buffer, (PGM_P)pgm_read_word(&(menuList[dateTimeMenuIndex].strings[selection])));
    editInt(0, &dateTimeVariable, buffer, saveChangeCallback);
  }
}

void viewMenu(uint8_t button) {
  if (menu_debug_print)SerialMonitorInterface.print("viewMenu ");
  if (menu_debug_print)SerialMonitorInterface.println(button);
  if (!button) {
    newMenu(mainMenuIndex);
    display.clearWindow(0, 12, 96, 64);
  } else {
    if (button == upButton) {
      if (currentSelectionLine > 0) {
        currentSelectionLine--;
      } else if (currentMenuLine > 0) {
        currentMenuLine--;
      }
    } else if (button == downButton) {
      if (currentSelectionLine < menuList[currentMenu].amtLines - 1 && currentSelectionLine < 3) {
        currentSelectionLine++;
      } else if (currentSelectionLine + currentMenuLine < menuList[currentMenu].amtLines - 1) {
        currentMenuLine++;
      }
    } else if (button == selectButton) {
      if (menu_debug_print)SerialMonitorInterface.print("select ");
      if (menu_debug_print)SerialMonitorInterface.println(currentMenuLine + currentSelectionLine);
      menuList[currentMenu].selectionHandler(currentMenuLine + currentSelectionLine);
    } else if (button == backButton) {
      newMenu(-1);
      if (!menuHistoryIndex)
        return;
    }
  }
  display.setFont(font10pt);
  if (lastMenuLine != currentMenuLine || lastSelectionLine != currentSelectionLine) {
    if (menu_debug_print)SerialMonitorInterface.println("drawing menu ");
    if (menu_debug_print)SerialMonitorInterface.println(currentMenu);
    for (int i = 0; i < 4; i++) {
      display.setCursor(7, menuTextY[i]);
      if (i == currentSelectionLine) {
        display.fontColor(defaultFontColor, inactiveFontBG);
      } else {
        display.fontColor(inactiveFontColor, inactiveFontBG);
      }
      if (currentMenuLine + i < menuList[currentMenu].amtLines) {
        char buffer[20];
        strcpy_P(buffer, (PGM_P)pgm_read_word(&(menuList[currentMenu].strings[currentMenuLine + i])));
        display.print(buffer);
        if (cyclingModeState == 1) {
          display.fontColor(defaultFontColor, inactiveFontBG);
          display.setCursor(70, 12);
          display.print("On");
        }
        else {
          display.fontColor(defaultFontColor, inactiveFontBG);
          display.setCursor(70, 12);
          display.print("Off");
        }
      }

      for (uint8_t i = 0; i < 25; i++)display.write(' ');
      if (i == 0) {
        display.fontColor(defaultFontColor, inactiveFontBG);
        display.setCursor(0, menuTextY[0]);
        display.print('<');
        display.setCursor(90, menuTextY[0]);
        display.print('^');
      }
      if (i == 3) {
        display.fontColor(defaultFontColor, inactiveFontBG);
        display.setCursor(0, menuTextY[3]);
        display.print('>');
        display.setCursor(90, menuTextY[3]);
        display.print('v');
      }
    }
    lastMenuLine = currentMenuLine;
    lastSelectionLine = currentSelectionLine;
  }
}
