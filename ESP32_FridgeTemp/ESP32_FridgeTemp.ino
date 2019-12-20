#include "WiFi.h"
#include "SparkFun_Si7021_Breakout_Library.h"
#include <Wire.h>
#include <HTTPClient.h>
#include "soc/rtc.h" // rtc_time_get

#define TESTING 0
#include "login.h"

#if TESTING
#define ADDR "http://192.168.1.122:8090/sensor"
#else
#define ADDR "http://10.24.20.121:8090/sensor"
#endif

#define VERBOSE 1
#define SLEEP_TIME 300              // Seconds ESP32 will go to sleep
#define WIFI_DOT_INTERVAL 50        // Milliseconds between checking if WiFi has connected
#define WIFI_DOT_INTERVAL_SEC 0.05  // WIFI_DOT_INTERVAL / 1000
#define WIFI_DOT_LINE 20            // Amount of dots before new line for dots
#define TIMEOUT 200 //62                  // Stops trying to connect to WiFi after TIMEOUT * 0.05 seconds
#define SEND_INTERVAL 1             // How many measurements before trying to send data by WiFi (Time = SLEEP_TIME * SEND_INTERVAL)
#define OFFLINE_MAX 60              // How many measurements to store in RTC memory (Max RTC memory available is 896 bytes)

// Enums
#define WORK_RECORD 0
#define WORK_SUCCESS 1
#define WORK_NOCONNECTION 2
#define WORK_HTTPFAIL 3

const uint64_t rtc_ratio = 154640; // What I think rtc to seconds is (rtc / ratio = seconds)

RTC_DATA_ATTR bool FirstWake = true;
RTC_DATA_ATTR int OfflineCount = 0;
RTC_DATA_ATTR bool WrappedOfflineCount = false;
RTC_DATA_ATTR float OfflineTemp[OFFLINE_MAX];
RTC_DATA_ATTR float OfflineHumid[OFFLINE_MAX];
RTC_DATA_ATTR int8_t OfflineWake[OFFLINE_MAX];
RTC_DATA_ATTR float OfflineVolt[OFFLINE_MAX];

#define SEND_STR_LEN 55
char send_str[SEND_STR_LEN * OFFLINE_MAX];
int httpResponse = -1;

float getBatteryVoltage(int analogVal=0);
void printWakeReason(int wakeReason=-1);

void setup() {
  Serial.begin(115200);
  setCpuFrequencyMhz(80);
  send_str[SEND_STR_LEN * OFFLINE_MAX - 1] = '\0';

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
//  if (VERBOSE) Serial.printf("Showing %d %f %f %d\n", index, OfflineTemp[index], OfflineHumid[index], OfflineWake[index]);
//  showTemp(OfflineTemp[index], OfflineHumid[index], OfflineWake[index], getBatteryVoltage(), wakeReason);
//  delay(500);
//  if (workResult != WORK_RECORD) {
//    delay(250);
//  }
  if (workResult == WORK_SUCCESS) {
    OfflineCount = 0;
    WrappedOfflineCount = false;
  }

  FirstWake = false;
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
  addOffline(temp, humid, wakeReason, getBatteryVoltage());
  Serial.printf("temp=%07.2lf humid=%07.2lf wake=%d\n", temp, humid, wakeReason);

  if (VERBOSE && FirstWake) Serial.println("First wake!");
  if (FirstWake || OfflineCount >= SEND_INTERVAL) {
    int OfflineCountIndex = OfflineCount;
    int tempOfflineCount = OfflineCount;
    if (WrappedOfflineCount) {
      tempOfflineCount = OFFLINE_MAX;
    }
    int chain = 0;
    while (OfflineCountIndex > 0) {
      // temp=tttt.tt humid=hhhh.hh wake=r volt=v.vv index=ii\n
      OfflineCountIndex -= 1;
      tempOfflineCount -= 1;
      chain += sprintf_P(send_str + chain, "temp=%07.2lf humid=%07.2lf wake=%hhd volt=%4.2f index=%d\n",
                                           OfflineTemp[tempOfflineCount], OfflineHumid[tempOfflineCount], OfflineWake[tempOfflineCount], OfflineVolt[tempOfflineCount], tempOfflineCount);
    }
    if (WrappedOfflineCount) {
      OfflineCountIndex = OFFLINE_MAX;
      while (OfflineCountIndex > OfflineCount) {
        // temp=tttt.tt humid=hhhh.hh wake=r volt=v.vv index=ii\n
        OfflineCountIndex -= 1;
        tempOfflineCount -= 1;
        chain += sprintf_P(send_str + chain, "temp=%07.2lf humid=%07.2lf wake=%hhd volt=%4.2f index=%d\n",
                                             OfflineTemp[tempOfflineCount], OfflineHumid[tempOfflineCount], OfflineWake[tempOfflineCount], OfflineVolt[tempOfflineCount], tempOfflineCount);
      }
    }

    pinMode(LED_BUILTIN, OUTPUT);
    // Detect WiFi stall
    digitalWrite(LED_BUILTIN, HIGH);

    int wifiCount = 0;

    WiFi.begin(ssid, password);
    if (VERBOSE) Serial.print("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED && wifiCount < TIMEOUT) {
      if (VERBOSE && wifiCount % WIFI_DOT_LINE == 0) Serial.println("");
      if (VERBOSE) Serial.print(".");
      wifiCount += 1;
      digitalWrite(LED_BUILTIN, HIGH);
      delay(WIFI_DOT_INTERVAL);
      digitalWrite(LED_BUILTIN, LOW);
    }
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("");

    char log_str[100];
    sprintf_P(log_str, "%d %d %4.2f\n", wifiCount, WiFi.status(), getBatteryVoltage());
    appendFile(getSDPath(), log_str);

    if (wifiCount < TIMEOUT) {
      if (VERBOSE) {
        Serial.printf("...Connected to %s in %.2f second(s)!\n", ssid, WIFI_DOT_INTERVAL_SEC * wifiCount);
        Serial.printf("Sending to %s:\n", ADDR);
        Serial.println(send_str);
      }

      HTTPClient http;
      http.begin(ADDR);
      http.addHeader("Content-Type", "text/plain");
      httpResponse = http.POST(send_str);
      if (VERBOSE) printf("HTTP response: %d\n", httpResponse);
      http.end();
  
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      btStop();
  
      if (httpResponse != 200) {
        return WORK_HTTPFAIL;
      }
      return WORK_SUCCESS;

    } else {
      if (VERBOSE) Serial.printf("\nCould not connect to WiFi in %.2f seconds.\n", TIMEOUT * WIFI_DOT_INTERVAL_SEC);
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      btStop();
      return WORK_NOCONNECTION;
    }
  }
  return WORK_RECORD;
}

void loop() {}

void addOffline(float temp, float humid, int wakeReason, float volt) {
  if (OfflineCount == OFFLINE_MAX) {
    WrappedOfflineCount = true;
    OfflineCount = 0;
  }
  OfflineTemp[OfflineCount] = temp;
  OfflineHumid[OfflineCount] = humid;
  OfflineWake[OfflineCount] = wakeReason;
  OfflineVolt[OfflineCount] = volt;
  OfflineCount += 1;
}
