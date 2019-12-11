#include "WiFi.h"
#include "SparkFun_Si7021_Breakout_Library.h"
#include <Wire.h>
#include <HTTPClient.h>
#include "soc/rtc.h"
#include <sys/time.h>

#define TESTING 1

#include "login.h"

#define uS_TO_S_FACTOR 1000000  //Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP 60        //Time ESP32 will go to sleep (in seconds)

#define TIMEOUT 10 // Stops trying to connect to WiFi after TIMEOUT * 0.5 seconds
#define VERBOSE 1
#define PRINT_NEXT_SLEEP 1
#ifdef TESTING
#define ADDR "http://192.168.1.122:8090/sensor"
#else
#define ADDR "http://10.24.20.121:8090/sensor"
#endif

const uint64_t rtc_ratio = 154640; // What I think rtc to seconds is (rtc / ratio = seconds)
const unsigned long timer_start_delay = 24000; // Microseconds

RTC_DATA_ATTR int bootCount = 0;

void setup() {
  Serial.begin(115200);

  //Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  //Print the wakeup reason for ESP32
  int wakeReason = print_wakeup_reason();

  int success = doWork(wakeReason);

  unsigned long sleepTime = TIME_TO_SLEEP * uS_TO_S_FACTOR;
  if (success) {
    sleepTime -= - esp_timer_get_time() - timer_start_delay;
  }
  if (VERBOSE) Serial.printf("Setup ESP32 to sleep for every %d Seconds (%lu micro-seconds)\n", TIME_TO_SLEEP, sleepTime);
  esp_sleep_enable_timer_wakeup(sleepTime);
  if (VERBOSE) Serial.printf("Setting ESP32 to wakeup on LOW on GPIO 33\n");
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_33, LOW);
  if (VERBOSE) Serial.println("Starting deep sleep");
  Serial.flush();
  //Go to sleep now
  esp_deep_sleep_start();
}

void loop() {}

//Function that prints the reason by which ESP32 has been awaken from sleep
int print_wakeup_reason()
{
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  switch (wakeup_reason)
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
  Serial.printf(" (%d)\n", wakeup_reason);
  return wakeup_reason;
}

bool doWork(int wakeReason)
{
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

    if (VERBOSE) Serial.printf("\n...Connected to %s in %.1f seconds!\n", ssid, 0.5 * wifiCount);

    Weather sensor;
    Wire.begin();
    //sensor.changeResolution(1);
    if (VERBOSE) printf("Reading sensor ");
    if (VERBOSE) printf("%u:\n", sensor.checkID());
    float humid = sensor.getRH();
    float temp = sensor.getTemp();

    HTTPClient http;
    char *temp_str = (char *)malloc(17);
    temp_str[16] = '\0';
    sprintf(temp_str, "%3.2lf %3.2lf %1d", temp, humid, wakeReason);
    if (VERBOSE) printf("%"PRIu64" %s\n", rtc_time_get(), temp_str);
    if (VERBOSE) printf("%"PRIu64" seconds since start\n", rtc_time_get() / rtc_ratio);

    if (VERBOSE) {
      struct timeval tv;
      gettimeofday(&tv, NULL);
      printf("%d timeofday\n", tv.tv_sec);
    }

    http.begin(ADDR);
    http.addHeader("Content-Type", "text/plain");
    int httpResponseCode = http.POST(temp_str);
    //int httpResponseCode = http.POST(String(temp, 2) + String(" ") + String(humid, 2));
    if (VERBOSE) printf("HTTP response: %d\n", httpResponseCode);

    http.end();
    free(temp_str);

    WiFi.disconnect();
    return true;
  } else {
    if (VERBOSE) Serial.printf("\nCould not connect to WiFi in a suitable timeframe.\n");
    return false;
  }
}
