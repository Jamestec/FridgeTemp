#include <LOLIN_EPD.h>
#include <Adafruit_GFX.h>

/*D32 Pro*/
#define EPD_CS 14
#define EPD_DC 27
#define EPD_RST 33  // can set to -1 and share with microcontroller Reset!
// 19 is orange cable, looking at back, 5th from left
#define EPD_BUSY 19 // can set to -1 to not use a pin (will wait a fixed delay)

LOLIN_IL3897 EPD(250, 122, EPD_DC, EPD_RST, EPD_CS, EPD_BUSY); //hardware SPI

void showTemp(float temp, float humid, int trigger, float batVolt, int workReturn) {
  EPD.begin();
  EPD.setRotation(0);
  EPD.setTextWrap(true);
  EPD.setTextColor(EPD_BLACK);
  EPD.clearBuffer();
  EPD.setCursor(0, 0);
  char buff[20];

  EPD.setTextSize(1);
  EPD.println("");

  EPD.print(" ");
  EPD.setTextSize(3);
  sprintf_P(buff, "Temp: %+6.2fC", temp);
  EPD.println(buff);
  
  EPD.setTextSize(1);
  EPD.println("");

  EPD.print(" ");
  EPD.setTextSize(3);
  sprintf_P(buff, "Humid:%6.2f", humid);
  EPD.print(buff);
  if (humid > -100) {
    EPD.print("%");
  }
  EPD.println("");

  EPD.setTextSize(1);
  for (int i = 0; i < 2; i++) EPD.println("");

  EPD.setTextSize(1);
  EPD.printf(" Battery = %1.2fv", batVolt);
  if (batVolt < 3.70) {
    EPD.printf("            low battery!\n");
  } else {
    EPD.printf("\n");
  }

  EPD.setTextSize(1);
  EPD.println("");

  switch (workReturn) {
    case WORK_NOCONNECTION:
      EPD.printf(" Couldn't connect to %s\n\n", ssid);
      break;
    case WORK_HTTPFAIL:
      EPD.printf(" HTTP POST fail (%d):\n %s\n", httpResponse, ADDR);
      break;
    default:
      EPD.printf("\n\n");
      break;  
  }

  EPD.printf("                                        %d\n", trigger);
  EPD.display();
}
