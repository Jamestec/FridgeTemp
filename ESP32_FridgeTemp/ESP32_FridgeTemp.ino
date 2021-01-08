#include "WiFi.h"
#include <HTTPClient.h>
#include "SparkFun_Si7021_Breakout_Library.h"
#include <Wire.h>
#include "soc/rtc.h" // rtc_time_get

#define TESTING 1
#include "login.h"

#if TESTING
#define ADDR "http://192.168.1.122:8090/sensor"
#define ADDR_PACKET "http://192.168.1.122:8090/sensor_packet"
#define ADDR_PACKET_FIN "http://192.168.1.122:8090/sensor_packet_fin"
#define ADDR_PACKET_RESET "http://192.168.1.122:8090/sensor_packet_reset"
#define ADDR_SD_STATUS "http://192.168.1.122:8090/sd_status"
#else
#define ADDR "http://10.24.20.121:8090/sensor"
#define ADDR_PACKET "http://10.24.20.121:8090/sensor_packet"
#define ADDR_PACKET_FIN "http://10.24.20.121:8090/sensor_packet_fin"
#define ADDR_PACKET_RESET "http://10.24.20.121:8090/sensor_packet_reset"
#define ADDR_SD_STATUS "http://10.24.20.121:8090/sd_status"
#endif

#define VERBOSE 1
#define SLEEP_TIME 180              // Seconds ESP32 will go to sleep
#define WIFI_DOT_INTERVAL 50        // Milliseconds between checking if WiFi has connected
#define WIFI_DOT_INTERVAL_SEC 0.05  // WIFI_DOT_INTERVAL / 1000
#define WIFI_DOT_LINE 20            // Amount of dots before new line for dots
#define TIMEOUT 300 //62 //11       // Stops trying to connect to WiFi after TIMEOUT * 0.05 seconds
#define WIFI_SYNC_TRIES 3           // How many attempts we should try if we have to resend data to ensure data integrity
#define SEND_INTERVAL 5             // How many measurements before trying to send data by WiFi (Time = SLEEP_TIME * SEND_INTERVAL)
#define OFFLINE_MAX 60              // How many measurements to store in RTC memory (Max RTC memory available is 896 bytes)
#define SD_POWER_PIN 15             // Since 3.3v pin is occupied by the Si7021

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

#define SEND_STR_LEN 55
char send_str[SEND_STR_LEN * OFFLINE_MAX];
int httpResponse = -1;

float getBatteryVoltage(int analogVal=0);
void printWakeReason(int wakeReason=-1);

void setup() {
  Serial.begin(115200);
  setCpuFrequencyMhz(80);
  send_str[SEND_STR_LEN * OFFLINE_MAX - 1] = '\0';
  pinMode(SD_POWER_PIN, OUTPUT);
  digitalWrite(SD_POWER_PIN, HIGH);

  int wakeReason = getWakeReason();
  if (VERBOSE) printWakeReason(wakeReason);

  int workResult = doWork(wakeReason);

  if (workResult == WORK_SUCCESS) {
    OfflineCount = 0;
    if (useSD) {
      removeFile(getDataPath());
    } else {
      WrappedOfflineCount = false;
    }
  }
  if (workResult == WORK_SDFAIL) {
    if (VERBOSE) Serial.printf("workResult gave SD fail, not using SD");
    switchOffSD();
  }

  if (VERBOSE && workResult == WORK_HTTPFAIL) {
    Serial.println("HTTP fail, check server is responding");
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
  if (VERBOSE && FirstWake) Serial.println("First wake!");
  Serial.printf("temp=%07.2lf humid=%07.2lf wake=%d\n", temp, humid, wakeReason);
  if (!addOffline(temp, humid, wakeReason, getBatteryVoltage())) {
    if (VERBOSE) Serial.println("addOffline failed, not using SD");
    switchOffSD();
    addOffline(temp, humid, wakeReason, getBatteryVoltage());
  }

  if (FirstWake || OfflineCount % SEND_INTERVAL == 0) {
    if (WiFiConnect()) {
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
          Serial.println("WIFI_SYNC_TRIES exceeded, couldn't send data");
        }

      } else {

        int OfflineCountIndex = OfflineCount;
        int chain = 0;
        if (WrappedOfflineCount) {
          while (OfflineCountIndex < OFFLINE_MAX) {
            // temp=tttt.tt humid=hhhh.hh wake=r volt=v.vv index=ii\n
            chain += sprintf_P(send_str + chain, "temp=%07.2lf humid=%07.2lf wake=%hhd volt=%4.2f index=%d\n",
                                                 OfflineTemp[OfflineCountIndex], OfflineHumid[OfflineCountIndex], OfflineWake[OfflineCountIndex], OfflineVolt[OfflineCountIndex], OfflineCountIndex);
            OfflineCountIndex += 1;
          }
        }
        OfflineCountIndex = 0;
        while (OfflineCountIndex < OfflineCount) {
          // temp=tttt.tt humid=hhhh.hh wake=r volt=v.vv index=ii\n
          chain += sprintf_P(send_str + chain, "temp=%07.2lf humid=%07.2lf wake=%hhd volt=%4.2f index=%d\n",
                                               OfflineTemp[OfflineCountIndex], OfflineHumid[OfflineCountIndex], OfflineWake[OfflineCountIndex], OfflineVolt[OfflineCountIndex], OfflineCountIndex);
          OfflineCountIndex += 1;
        }

        WiFiSend(ADDR, send_str);
      }
      WiFiSendSDStatus();
      WiFiEnd();

      if (httpResponse != 200) {
        return WORK_HTTPFAIL;
      }
      return WORK_SUCCESS;

    } else {
      if (VERBOSE) Serial.printf("\nCould not connect to %s in %.2f seconds.\n", ssid, TIMEOUT * WIFI_DOT_INTERVAL_SEC);
      return WORK_NOCONNECTION;
    }
  }
  return WORK_RECORD;
}

void loop() {}

// If this returns false, something wrong with SD
boolean addOffline(float temp, float humid, int wakeReason, float volt) {
  if (useSD) {
    sprintf_P(send_str, "temp=%07.2lf humid=%07.2lf wake=%hhd volt=%4.2f index=%d\n", temp, humid, wakeReason, volt, OfflineCount);
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
