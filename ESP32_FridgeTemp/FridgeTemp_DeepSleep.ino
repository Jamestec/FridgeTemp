#define uS_TO_S_FACTOR 1000000  // Conversion factor for micro seconds to seconds
const unsigned long timer_start_delay = 350000; // Microseconds

void doDeepSleep(int sleepSeconds) {
  unsigned long sleepTime = sleepSeconds * uS_TO_S_FACTOR;
  unsigned long offset = esp_timer_get_time() + timer_start_delay;
  if (offset > sleepTime) {
    offset = sleepTime - 1;
  }
  sleepTime -= offset;
  if (VERBOSE) Serial.printf("Setup ESP32 to sleep for %d Seconds (%lu micro-seconds)\n", SLEEP_TIME, sleepTime);
  esp_sleep_enable_timer_wakeup(sleepTime);
//  if (VERBOSE) Serial.printf("Setting ESP32 to wakeup on LOW on GPIO 33\n");
//  esp_sleep_enable_ext0_wakeup(GPIO_NUM_33, LOW);
  if (VERBOSE) Serial.println("Starting deep sleep");
  if (VERBOSE) Serial.flush();
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
  Serial.println("");
}
