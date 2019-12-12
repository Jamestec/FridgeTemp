#include "WiFi.h"
#include "SparkFun_Si7021_Breakout_Library.h"
#include <Wire.h>
#include <HTTPClient.h>
#include "soc/rtc.h"
#include <sys/time.h>
#include <LOLIN_EPD.h>
#include <Adafruit_GFX.h>

#define uS_TO_S_FACTOR 1000000  // Conversion factor for micro seconds to seconds

#define TESTING 1
#include "login.h"

#ifdef TESTING
#define ADDR "http://192.168.1.122:8090/sensor"
#else
#define ADDR "http://10.24.20.121:8090/sensor"
#endif

/*D32 Pro*/
#define EPD_CS 14
#define EPD_DC 27
#define EPD_RST 33  // can set to -1 and share with microcontroller Reset!
// 19 is orange cable, looking at back, 5th from left
#define EPD_BUSY 19 // can set to -1 to not use a pin (will wait a fixed delay)
#define BATTERY_PIN 35          // This is with a voltage divider

#define VERBOSE 1
#define SLEEP_TIME 60           // Time ESP32 will go to sleep (in seconds)
#define TIMEOUT 20              // Stops trying to connect to WiFi after TIMEOUT * 0.5 seconds
#define SEND_INTERVAL 5         // How many measurements before trying to send data by WiFi (Time = SLEEP_TIME * SEND_INTERVAL)
#define OFFLINE_MAX 10

#define WORK_RECORD 0
#define WORK_SUCCESS 1
#define WORK_NOCONNECTION 2
#define WORK_HTTPFAIL 3

float getBatteryVoltage(int analogVal=0);
void printWakeReason(int wakeReason=-1);

LOLIN_IL3897 EPD(250, 122, EPD_DC, EPD_RST, EPD_CS, EPD_BUSY); //hardware SPI

const uint64_t rtc_ratio = 154640; // What I think rtc to seconds is (rtc / ratio = seconds)
const unsigned long timer_start_delay = 24000; // Microseconds

RTC_DATA_ATTR int OfflineCount = 0;
RTC_DATA_ATTR float OfflineTemp[OFFLINE_MAX];
RTC_DATA_ATTR float OfflineHumid[OFFLINE_MAX];
RTC_DATA_ATTR int OfflineWake[OFFLINE_MAX];

char send_str[50 * OFFLINE_MAX];

void setup() {
  Serial.begin(115200);
  send_str[50 * OFFLINE_MAX - 1] = '\0';

  if (VERBOSE) Serial.printf("%"PRIu64" = ""%"PRIu64" seconds since start\n", rtc_time_get(), rtc_time_get() / rtc_ratio);
  if (VERBOSE) {
    struct timeval tv;
    gettimeofday(&tv, NULL); 
    Serial.printf("timeofday %d\n", tv.tv_sec);
  }
  if (VERBOSE) Serial.printf("esp_timer_get_time %d\n", esp_timer_get_time);

  int wakeReason = getWakeReason();
  if (VERBOSE) printWakeReason(wakeReason);

  int workResult = doWork(wakeReason);
  int index = OfflineCount - 1;
  if (VERBOSE) Serial.printf("Showing %d %f %f %d\n", index, OfflineTemp[index], OfflineHumid[index], OfflineWake[index]);
  showTemp(OfflineTemp[index], OfflineHumid[index], OfflineWake[index], getBatteryVoltage(), wakeReason);
//  delay(500);
//  if (workResult != WORK_RECORD) {
//    delay(250);
//  }
  if (workResult == WORK_SUCCESS) OfflineCount = 0;

  doDeepSleep(SLEEP_TIME);
}

int doWork(int wakeReason) {
  Weather sensor;
  Wire.begin();
  //sensor.changeResolution(1);
  if (VERBOSE) printf("Reading sensor ");
  if (VERBOSE) printf("%u:\n", sensor.checkID());

  float humid = sensor.getRH();
  float temp = sensor.getTemp();
  addOffline(temp, humid, wakeReason);
  Serial.printf("temp=%07.2lf humid=%07.2lf wake=%d\n", temp, humid, wakeReason);

  if (OfflineCount >= SEND_INTERVAL) {
    int tempOfflineCount = OfflineCount;
    int chain = 0;
    while (tempOfflineCount > 0) {
      // temp=ttt.tt humid=hhh.hh wake=r\n
      tempOfflineCount -= 1;
      chain += sprintf_P(send_str + chain, "temp=%07.2lf humid=%07.2lf wake=%d\n", OfflineTemp[tempOfflineCount], OfflineHumid[tempOfflineCount], OfflineWake[tempOfflineCount]);
    }

    pinMode(LED_BUILTIN, OUTPUT);
    // Detect WiFi stall
    digitalWrite(LED_BUILTIN, HIGH);

    int wifiCount = 0;

    WiFi.begin(ssid, password);
    if (VERBOSE) Serial.println("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED && wifiCount < TIMEOUT) {
      if (VERBOSE) Serial.print(".");
      wifiCount += 1;
      digitalWrite(LED_BUILTIN, HIGH);
      delay(500);
      digitalWrite(LED_BUILTIN, LOW);
    }
    digitalWrite(LED_BUILTIN, LOW);

    if (wifiCount < TIMEOUT) {
      if (VERBOSE) {
        Serial.printf("\n...Connected to %s in %.1f seconds!\n", ssid, 0.5 * wifiCount);
        Serial.printf("Sending to %s:\n", ADDR);
        Serial.println(send_str);
      }

      HTTPClient http;
      http.begin(ADDR);
      http.addHeader("Content-Type", "text/plain");
      int httpResponseCode = http.POST(send_str);
      if (VERBOSE) printf("HTTP response: %d\n", httpResponseCode);
      http.end();
  
      WiFi.disconnect();
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      btStop();
  
      if (httpResponseCode != 200) {
        return WORK_HTTPFAIL;
      }
      return WORK_SUCCESS;

    } else {
      if (VERBOSE) Serial.printf("\nCould not connect to WiFi in a suitable timeframe.\n");
      return WORK_NOCONNECTION;
    }
  }
  return WORK_RECORD;
}

void loop() {}

void addOffline(float temp, float humid, int wakeReason) {
  if (OfflineCount == OFFLINE_MAX) {
    OfflineCount = 0;
  }
  OfflineTemp[OfflineCount] = temp;
  OfflineHumid[OfflineCount] = humid;
  OfflineWake[OfflineCount] = wakeReason;
  OfflineCount += 1;
}

void doDeepSleep(int sleepSeconds) {
  unsigned long sleepTime = SLEEP_TIME * uS_TO_S_FACTOR - esp_timer_get_time() - timer_start_delay;
  if (VERBOSE) Serial.printf("Setup ESP32 to sleep for every %d Seconds (%lu micro-seconds)\n", SLEEP_TIME, sleepTime);
  esp_sleep_enable_timer_wakeup(sleepTime);
  if (VERBOSE) Serial.printf("Setting ESP32 to wakeup on LOW on GPIO 33\n");
  //esp_sleep_enable_ext0_wakeup(GPIO_NUM_33, LOW);
  if (VERBOSE) Serial.println("Starting deep sleep");
  Serial.flush();
  //Go to sleep now
  esp_deep_sleep_start();
}

int getWakeReason() {
  return esp_sleep_get_wakeup_cause();
}

void printWakeReason(int wakeReason) {
  if (wakeReason == -1) {
    wakeReason = getWakeReason();
  }
  switch (wakeReason)
  {
    // https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/system/sleep_modes.html#_CPPv418esp_sleep_source_t
    case ESP_SLEEP_WAKEUP_EXT0  : Serial.printf("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1  : Serial.printf("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER  : Serial.printf("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD  : Serial.printf("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP  : Serial.printf("Wakeup caused by ULP program"); break;
    case ESP_SLEEP_WAKEUP_GPIO  : Serial.printf("Wakeup caused by GPIO (from light sleep)"); break;
    case ESP_SLEEP_WAKEUP_UART  : Serial.printf("Wakeup caused by UART (from light sleep)"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep"); break;
  }
}

float getBatteryVoltage(int analogVal) {
//  float real = map(analogVal, 0, 4095, 0, 360) * 0.02;
  if (!analogVal) {
    analogVal = analogRead(BATTERY_PIN);
  }
  return analogVal / 568.8;
}


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
      EPD.printf(" HTTP POST fail:\n %s\n", ADDR);
      break;
    default:
      EPD.printf("\n\n");
      break;  
  }

  EPD.printf("                                        %d\n", trigger);
  EPD.display();
}
