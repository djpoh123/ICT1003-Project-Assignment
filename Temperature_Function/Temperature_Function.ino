/*************************************************************************
   BMA250 Tutorial:
   This example program will show the basic method of printing out the
   accelerometer values from the BMA250 to the Serial Monitor, and the
   Serial Plotter

   Hardware by: TinyCircuits
   Code by: Laverena Wienclaw for TinyCircuits

   Initiated: Mon. 11/1/2018
   Updated: Tue. 11/2/2018
 ************************************************************************/

#include <Wire.h>         // For I2C communication with sensor
#include "BMA250.h"       // For interfacing with the accel. sensor
#include <TinyScreen.h>

// Accelerometer sensor variables for the sensor and its values
BMA250 accel_sensor;
int x, y, z;
double temp;


#if defined (ARDUINO_ARCH_AVR)
TinyScreen display = TinyScreen(TinyScreenDefault);
#define SerialMonitorInterface Serial

#elif defined(ARDUINO_ARCH_SAMD)
TinyScreen display = TinyScreen(TinyScreenDefault);
#define SerialMonitorInterface SerialUSB
#endif

void setup() {
  SerialMonitorInterface.begin(115200);
  Wire.begin();
  display.begin();
  display.setFlip(true);
  display.on();
  display.setFont(liberationSans_10ptFontInfo);

  USBDevice.init();
  USBDevice.attach();
  SerialUSB.begin(9600);

  SerialMonitorInterface.print("Initializing BMA...");
  // Set up the BMA250 acccelerometer sensor
  accel_sensor.begin(BMA250_range_2g, BMA250_update_time_64ms);
}

void loop() {
  accel_sensor.read();//This function gets new data from the acccelerometer

  // Get the acceleration values from the sensor and store them into global variables
  // (Makes reading the rest of the program easier)
  temp = ((accel_sensor.rawTemp * 0.5) + 24.0);

  // If the BMA250 is not found, nor connected correctly, these values will be produced
  // by the sensor
  if (x == -1 && y == -1 && z == -1) {
    // Print error message to Serial Monitor
    display.setCursor(12,25);
    display.print("ERROR! NO BMA250 DETECTED!");
  }

  else { // if we have correct sensor readings:
    showDisplay();                 //Print to Serial Monitor or Plotter
  }

  // The BMA250 can only poll new sensor values every 64ms, so this delay
  // will ensure that we can continue to read values
  delay(100);
  // ***Without the delay, there would not be any sensor output***
}

// Prints the sensor values to the Serial Monitor, or Serial Plotter (found under 'Tools')
void showDisplay() {
  display.clearScreen();
  // Initialization of Display Cursor
  display.setCursor(10,5);
  
  // Title Message
  display.print("Current Temp");

  // Print Temperature Value
  if (temp > 34.5){
    display.setCursor(10,25);
    display.print("Temp: ");
    display.print(temp);
    display.print("C");
    display.setCursor(10,40);
    display.print("Please Hydrate!");
  }
  
    display.setCursor(10,25);
    display.print("Temp: ");
    display.print(temp);
    display.print("C");
    display.setCursor(10,40);
  
}
