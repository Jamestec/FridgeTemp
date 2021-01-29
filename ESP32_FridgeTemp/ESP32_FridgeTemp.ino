
#define TESTING 1
#define VERBOSE 0
#define SI7021 0                    // 1 or else we're using the SparkFun TMP117
#define SENSOR_ID "1"               // Unique ID, 0 is reserved for sensors without an id
#define STORE_TIME 1                // 0 for ESPs that can't keep track of time
#include "login.h"

#include "WiFi.h"
#include <HTTPClient.h>
#if SI7021 == 1
#include "SparkFun_Si7021_Breakout_Library.h"
#else
#include <SparkFun_TMP117.h>
#endif
#include <Wire.h>
#include "soc/rtc.h" // rtc_time_get

#if TESTING
#define ADDR "http://192.168.1.122:8090/sensor"
#define ADDR_PACKET "http://192.168.1.122:8090/sensor_packet"
#define ADDR_PACKET_FIN "http://192.168.1.122:8090/sensor_packet_fin"
#define ADDR_PACKET_RESET "http://192.168.1.122:8090/sensor_packet_reset"
#define ADDR_SD_STATUS "http://192.168.1.122:8090/sd_status"
#define ADDR_TIME_REQ "http://192.168.1.122:8090/epoch"
#else
#define ADDR "http://10.24.20.121:8090/sensor"
#define ADDR_PACKET "http://10.24.20.121:8090/sensor_packet"
#define ADDR_PACKET_FIN "http://10.24.20.121:8090/sensor_packet_fin"
#define ADDR_PACKET_RESET "http://10.24.20.121:8090/sensor_packet_reset"
#define ADDR_SD_STATUS "http://10.24.20.121:8090/sd_status"
#define ADDR_TIME_REQ "http://10.24.20.121:8090/epoch"
#endif

#define SLEEP_TIME 180              // Seconds ESP32 will go to sleep
#define WIFI_DOT_INTERVAL 50        // Milliseconds between checking if WiFi has connected
#define WIFI_DOT_INTERVAL_SEC 0.05  // WIFI_DOT_INTERVAL / 1000
#define WIFI_DOT_LINE 20            // Amount of dots before new line for dots
#define TIMEOUT 300 //62 //11       // Stops trying to connect to WiFi after TIMEOUT * 0.05 seconds
#define WIFI_SYNC_TRIES 3           // How many attempts we should try if we have to resend data to ensure data integrity
#define SEND_INTERVAL 5             // How many measurements before trying to send data by WiFi (Time = SLEEP_TIME * SEND_INTERVAL)
#define OFFLINE_MAX 60              // How many measurements to store in RTC memory (Max RTC memory available is 896 bytes)

// Enums
#define WORK_RECORD 0
#define WORK_SUCCESS 1
#define WORK_NOCONNECTION 2
#define WORK_HTTPFAIL 3
#define WORK_SDFAIL 4

RTC_DATA_ATTR bool FirstWake = true;
RTC_DATA_ATTR bool useSD = true;
RTC_DATA_ATTR unsigned int OfflineCount = 0; // Index that is next to be filled
RTC_DATA_ATTR bool WrappedOfflineCount = false;
RTC_DATA_ATTR float OfflineTemp[OFFLINE_MAX];
RTC_DATA_ATTR float OfflineHumid[OFFLINE_MAX];
RTC_DATA_ATTR int8_t OfflineWake[OFFLINE_MAX];
RTC_DATA_ATTR float OfflineVolt[OFFLINE_MAX];

#define SEND_STR_LEN 70
char send_str[SEND_STR_LEN * OFFLINE_MAX];
int httpResponse = -1;

float getBatteryVoltage(int analogVal=0);
void printWakeReason(int wakeReason=-1);

void setup() {
  Serial.begin(115200);
  setCpuFrequencyMhz(80);
  send_str[SEND_STR_LEN * OFFLINE_MAX - 1] = '\0';

  int wakeReason = getWakeReason();
  if (VERBOSE) printWakeReason(wakeReason);

  int workResult = doWork(wakeReason);

  if (workResult == WORK_SUCCESS) {
    OfflineCount = 0;
    if (useSD) {
      removeFile(getDataPath());
    } else {
      WrappedOfflineCount = false;
      useSD = true; // Try use the SD again, only a problem if SD access works 50/50 of the time
    }
  }
  if (workResult == WORK_SDFAIL) {
    if (VERBOSE) Serial.printf("workResult gave SD fail, not using SD");
    switchOffSD();
  }

  if (VERBOSE && workResult == WORK_HTTPFAIL) {
    Serial.println("HTTP fail, check server is responding");
  }

  if (VERBOSE && !useSD) Serial.println("SD is not in use");

  FirstWake = false;
  doDeepSleep(SLEEP_TIME);
}

int doWork(int wakeReason) {
  float humid = 0;
  float temp = 0;
  #if SI7021 == 1
  Weather sensor;
  Wire.begin();
  if (VERBOSE) printf("Reading sensor ");
  if (VERBOSE) printf("%u:\n", sensor.checkID());
  humid = sensor.getRH();
  temp = sensor.getTemp();
  #else
  Wire.begin();
  Wire.setClock(400000);
  TMP117 sensor;
  if (sensor.begin() == true) {
    if (VERBOSE) Serial.println("Reading from TMP117");
  } else {
    if (VERBOSE) Serial.println("TMP117 sensor failed to start");
  }
  temp = sensor.readTempC();
  #endif

  if (VERBOSE && FirstWake) Serial.println("First wake!");

  struct timeval tv;
  if (gettimeofday(&tv, NULL) == -1) {
    Serial.println("Unable to gettimeofday()");
  }

  if (VERBOSE) Serial.printf("temp=%07.2lf humid=%07.2lf wake=%d OfflineCount=%d time=%d\n", temp, humid, wakeReason, OfflineCount, tv.tv_sec);
  if (!addOffline(temp, humid, wakeReason, getBatteryVoltage(), tv.tv_sec)) {
    if (VERBOSE) Serial.println("addOffline failed, not using SD");
    switchOffSD();
    addOffline(temp, humid, wakeReason, getBatteryVoltage(), tv.tv_sec);
  }

  if (FirstWake || OfflineCount % SEND_INTERVAL == 0) {
    if (WiFiConnect()) {

      #if STORE_TIME == 1
      // Update time
      int epoch = WiFiGetTime();
      if (epoch != -1) {
        tv.tv_sec = epoch;
        if (settimeofday(&tv, NULL) == -1) {
          if (VERBOSE) Serial.println("Unable to settimeofday()");
        } else {
          if (VERBOSE) Serial.printf("Set new time to: %d\n", tv.tv_sec);
        }
      }
      #endif

      bool success = false;
      if (useSD) {

        int sendCount = 0;
        while (sendCount < WIFI_SYNC_TRIES) {
          if (VERBOSE) Serial.printf("Sending SD data...\n");
          httpResponse = sendData(getDataPath(), send_str);
          if (httpResponse == 200) break;
          if (httpResponse == 0) return WORK_SDFAIL;
          if (httpResponse == -1 || httpResponse == -2) {
            if (!WiFiSendPacketReset()) return WORK_HTTPFAIL;
          }
          sendCount += 1;
          if (VERBOSE) Serial.printf("Sending failed: %d, retrying %d...\n", httpResponse, sendCount);
        }
        if (VERBOSE && sendCount >= WIFI_SYNC_TRIES) {
          if (VERBOSE) Serial.println("WIFI_SYNC_TRIES exceeded, couldn't send data");
        }
        if (httpResponse == 200 && sendCount < WIFI_SYNC_TRIES) {
          success = true;
        }

      } else {

        int OfflineCountIndex = OfflineCount;
        int chain = 0;
        if (WrappedOfflineCount) {
          while (OfflineCountIndex < OFFLINE_MAX) {
            // temp=tttt.tt humid=hhhh.hh wake=r volt=v.vv index=ii id=dd time=eeeeeeeeee\n
            chain += sprintf_P(send_str + chain, "temp=%07.2lf humid=%07.2lf wake=%hhd volt=%4.2f index=%d id=%s time=%d\n",
                                         OfflineTemp[OfflineCountIndex], OfflineHumid[OfflineCountIndex], OfflineWake[OfflineCountIndex], OfflineVolt[OfflineCountIndex], OfflineCountIndex, SENSOR_ID, tv.tv_sec);
            OfflineCountIndex += 1;
          }
        }
        OfflineCountIndex = 0;
        while (OfflineCountIndex < OfflineCount) {
          // temp=tttt.tt humid=hhhh.hh wake=r volt=v.vv index=ii id=dd time=eeeeeeeeee\n
          chain += sprintf_P(send_str + chain, "temp=%07.2lf humid=%07.2lf wake=%hhd volt=%4.2f index=%d id=%s time=%d\n",
                                       OfflineTemp[OfflineCountIndex], OfflineHumid[OfflineCountIndex], OfflineWake[OfflineCountIndex], OfflineVolt[OfflineCountIndex], OfflineCountIndex, SENSOR_ID, tv.tv_sec);
          OfflineCountIndex += 1;
        }
        if (WiFiSend(ADDR, send_str) == 200) {
          success = true;
        }

      }
      WiFiSendSDStatus();
      WiFiEnd();

      if (success) {
        return WORK_SUCCESS;
      }
      return WORK_HTTPFAIL;

    } else {
      if (VERBOSE) Serial.printf("\nCould not connect to %s in %.2f seconds.\n", ssid, TIMEOUT * WIFI_DOT_INTERVAL_SEC);
      return WORK_NOCONNECTION;
    }
  }
  return WORK_RECORD;
}

void loop() {}

// If this returns false, something wrong with SD
boolean addOffline(float temp, float humid, int wakeReason, float volt, int epoch) {
  if (useSD) {
    sprintf_P(send_str, "temp=%07.2lf humid=%07.2lf wake=%hhd volt=%4.2f index=%d id=%s time=%d\n", temp, humid, wakeReason, volt, OfflineCount, SENSOR_ID, epoch);
    if (!appendFile(getDataPath(), send_str)) {
      return false;
    }
    OfflineCount += 1;
  } else {
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
  return true;
}

// Something wrong with SD, so we'll use RTC memory
void switchOffSD() {
  if (useSD) {
    useSD = false;
    OfflineCount = 0;
  }
}
