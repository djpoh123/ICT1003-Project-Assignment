#include <Wire.h>
#include <SPI.h>
#include <TinyScreen.h>

TinyScreen display = TinyScreen(TinyScreenDefault);

void setup() {
  Wire.begin();
  display.begin();
  display.setFlip(true);
  delay(100);
}

void loop() {
  buttonTest();
  //delay(1000);
}

void buttonTest() {
  display.clearScreen();
  if (display.getButtons(TSButtonUpperRight)) { crashUI(); }
}

void crashUI() {
  //start timer loop for 15 seconds
  int i = 15;
  //width from the right for '10'
  int j = 6;
  unsigned long startTime = millis();  
  while (millis() - startTime < 15000) {
    unsigned long secondTime = millis();
    // count per second
    while (millis() - secondTime < 1000) { 
      // set the font, color and cursor and print question, buttons, time
      display.setFont(thinPixel7_10ptFontInfo);
      display.fontColor(TS_8b_White,TS_8b_Black);
      display.setCursor(0, 0);
      display.println("Crash?");
      
      display.setFont(liberationSansNarrow_12ptFontInfo);
      display.fontColor(TS_8b_White,TS_8b_Black);
      display.setCursor(0, 16);
      display.println("Are you okay?");
      
      display.setFont(thinPixel7_10ptFontInfo);
      display.fontColor(TS_8b_Green,TS_8b_Black);
      display.setCursor(0, 48);
      display.println("Okay");
      display.fontColor(TS_8b_Red,TS_8b_Black);
      display.setCursor(72, 48);
      display.println("Nope");

      display.setFont(liberationSansNarrow_12ptFontInfo);
      display.fontColor(TS_8b_White,TS_8b_Black);
    
      //set cursor to be based from the right and print width of i
      display.setCursor(48-j, 32);    
      //print timer
      display.println(i);

      // flash
      delay(100);
      display.drawRect(0,0,96,64, TSRectangleFilled, TS_8b_Red);
      delay(100);
      display.clearScreen();
    }

    // one second later or interrupt: break, or clear and decrement
    if (display.getButtons(TSButtonLowerRight)) { break; }
    else if (display.getButtons(TSButtonLowerLeft)) {
      i = 0;
      break;
    }
    else {
      display.clearWindow(48-j, 32, 12, 12);
      i -= 1;
      if (i < 10) {
      j = 0;
      }
    }
  }
  //timer run out or interrupt
  if (i == 0) { crashNotOkay(); }
  else { crashOkay();}
}

void crashOkay() {
  display.clearScreen();
  //set the font, color and cursor and print question
  display.setFont(thinPixel7_10ptFontInfo);
  display.fontColor(TS_8b_Red,TS_8b_Black);
  display.setCursor(0, 16);
  display.println("It's not like I was");
  display.setCursor(0, 30);
  display.println("worried about you..");
  delay(2000);
  display.clearScreen();
  display.setFont(liberationSansNarrow_16ptFontInfo);
  display.setCursor(24, 24);
  display.println("BAKA!");
}

void crashNotOkay() {
  display.clearScreen();
  //start timer loop for 10 seconds
  unsigned long startTime = millis(); 
  while (millis() - startTime < 10000) {
    //set the font, color and cursor and print message
    display.setFont(thinPixel7_10ptFontInfo);
    display.fontColor(TS_8b_White,TS_8b_Black);
    display.setCursor(0, 0);
    display.println("Alright,");
    display.setFont(liberationSansNarrow_12ptFontInfo);
    display.setCursor(16, 16);
    display.println("I'm calling");
    display.setCursor(20, 34);
    display.println("for help.");
  }
  
}
