#include "WiFi.h"
#include "SparkFun_Si7021_Breakout_Library.h"
#include <Wire.h>
#include <HTTPClient.h>
#include "soc/rtc.h" // rtc_time_get

#define TESTING 1
#include "login.h"

#ifdef TESTING
#define ADDR "http://192.168.1.122:8090/sensor"
#else
#define ADDR "http://10.24.20.121:8090/sensor"
#endif

#define VERBOSE 1
#define SLEEP_TIME 20           // Time ESP32 will go to sleep (in seconds)
#define TIMEOUT 20              // Stops trying to connect to WiFi after TIMEOUT * 0.5 seconds
#define SEND_INTERVAL 2         // How many measurements before trying to send data by WiFi (Time = SLEEP_TIME * SEND_INTERVAL)
#define OFFLINE_MAX 10

#define WORK_RECORD 0
#define WORK_SUCCESS 1
#define WORK_NOCONNECTION 2
#define WORK_HTTPFAIL 3

const uint64_t rtc_ratio = 154640; // What I think rtc to seconds is (rtc / ratio = seconds)
const unsigned long timer_start_delay = 24000; // Microseconds

RTC_DATA_ATTR int OfflineCount = 0;
RTC_DATA_ATTR float OfflineTemp[OFFLINE_MAX];
RTC_DATA_ATTR float OfflineHumid[OFFLINE_MAX];
RTC_DATA_ATTR int OfflineWake[OFFLINE_MAX];

char send_str[50 * OFFLINE_MAX];
int httpResponse = -1;

float getBatteryVoltage(int analogVal=0);
void printWakeReason(int wakeReason=-1);

void setup() {
  Serial.begin(115200);
  send_str[50 * OFFLINE_MAX - 1] = '\0';

  if (VERBOSE) Serial.printf("%"PRIu64" = ""%"PRIu64" seconds since start\n", rtc_time_get(), rtc_time_get() / rtc_ratio);
  if (VERBOSE) {
    struct timeval tv;
    gettimeofday(&tv, NULL); 
    Serial.printf("timeofday %d\n", tv.tv_sec);
  }
  if (VERBOSE) Serial.printf("esp_timer_get_time %d\n", esp_timer_get_time());

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
        Serial.printf("\n...Connected to %s in %.1f second(s)!\n", ssid, 0.5 * wifiCount);
        Serial.printf("Sending to %s:\n", ADDR);
        Serial.println(send_str);
      }

      HTTPClient http;
      http.begin(ADDR);
      http.addHeader("Content-Type", "text/plain");
      httpResponse = http.POST(send_str);
      if (VERBOSE) printf("HTTP response: %d\n", httpResponse);
      http.end();
  
      WiFi.disconnect();
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      btStop();
  
      if (httpResponse != 200) {
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
