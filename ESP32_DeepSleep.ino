// https://techtutorialsx.com/2017/06/29/esp32-arduino-getting-started-with-wifi/
#include "WiFi.h"
#include "SparkFun_Si7021_Breakout_Library.h"
#include <Wire.h>
#include <HTTPClient.h> 
#include "soc/rtc.h"
#include <sys/time.h>

#define TESTING 1

#include "login.h"

#define FALSE 0
#define TRUE 1
#define uS_TO_S_FACTOR 1000000  //Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP 60        //Time ESP32 will go to sleep (in seconds)

#define VERBOSE 0
#define PRINT_NEXT_SLEEP 1
#ifdef TESTING
  #define ADDR "http://192.168.1.122:8090/sensor"
#else
  #define ADDR "http://10.24.20.121:8090/sensor"
#endif

const uint64_t rtc_ratio = 154640; // What I think rtc to seconds is (rtc / ratio = seconds)
const unsigned long timer_start_delay = 24000; // Microseconds

unsigned int localPort = 2390;      // local port to listen for UDP packets
IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
WiFiUDP Udp; // A UDP instance to let us send and receive packets over UDP

RTC_DATA_ATTR int bootCount = 0;

void setup(){
  Serial.begin(115200);
  //delay(1000); //Take some time to open up the Serial Monitor

  //Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  //Print the wakeup reason for ESP32
  int wakeReason = print_wakeup_reason();

  int success = doWork(wakeReason);

  /* // Default is AUTO (OFF)so these aren't neccessary
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);*/

  // Can't access s_config, we're using Arduino-ESP32 library
  //const char* option_str[] = {"OFF", "ON", "AUTO(OFF)" /* Auto works as OFF */};
  /*Serial.printf("\nRTC_PERIPH: %s, RTC_SLOW_MEM: %s, RTC_FAST_MEM: %s",
            option_str[s_config.pd_options[ESP_PD_DOMAIN_RTC_PERIPH]],
            option_str[s_config.pd_options[ESP_PD_DOMAIN_RTC_SLOW_MEM]],
            option_str[s_config.pd_options[ESP_PD_DOMAIN_RTC_FAST_MEM]]);*/

  //Set timer to 5 seconds
  unsigned long sleepTime = TIME_TO_SLEEP * uS_TO_S_FACTOR;
  if (success) {
     sleepTime -= - esp_timer_get_time() - timer_start_delay;
  }
  if (VERBOSE) Serial.printf("Setup ESP32 to sleep for every %d Seconds (%lu micro-seconds)\n", TIME_TO_SLEEP, sleepTime);
  esp_sleep_enable_timer_wakeup(sleepTime);
  if (VERBOSE) Serial.printf("Setting ESP32 to wakeup on LOW on GPIO 33\n");
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_33,LOW);
  if (VERBOSE) Serial.println("Starting deep sleep");
  Serial.flush();
  //Go to sleep now
  esp_deep_sleep_start();
}

void loop(){}

//Function that prints the reason by which ESP32 has been awaken from sleep
int print_wakeup_reason()
{
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  switch(wakeup_reason)
  {
    // https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/system/sleep_modes.html#_CPPv418esp_sleep_source_t
    case ESP_SLEEP_WAKEUP_EXT0  : Serial.println("Wakeup caused by external signal using RTC_IO"); return 2;
    case ESP_SLEEP_WAKEUP_EXT1  : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER  : Serial.println("Wakeup caused by timer"); return 1;
    case ESP_SLEEP_WAKEUP_TOUCHPAD  : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP  : Serial.println("Wakeup caused by ULP program"); break;
    case ESP_SLEEP_WAKEUP_GPIO  : Serial.println("Wakeup caused by GPIO (from light sleep)"); break;
    case ESP_SLEEP_WAKEUP_UART  : Serial.println("Wakeup caused by UART (from light sleep)"); break;
    default : Serial.println("Wakeup was not caused by deep sleep"); break;
  }
  return 0;
}

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);

  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
  if (VERBOSE) Serial.println("Sent NTP request...");
}

bool checkNTP() 
{
  if (Udp.parsePacket()) {
    if (VERBOSE) Serial.println("packet received");
    // We've received a packet, read the data from it
    Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    if (VERBOSE) Serial.print("Seconds since Jan 1 1900 = ");
    if (VERBOSE) Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    if (VERBOSE) Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;
    // print Unix time:
    if (VERBOSE) Serial.println(epoch);

    // print the hour, minute and second:
    Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
    Serial.print(':');
    if (((epoch % 3600) / 60) < 10) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
    Serial.print(':');
    if ((epoch % 60) < 10) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.println(epoch % 60); // print the second

    if (PRINT_NEXT_SLEEP) {
      epoch += TIME_TO_SLEEP;
      Serial.print("The next wakeup is ");       // UTC is the time at Greenwich Meridian (GMT)
      Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
      Serial.print(':');
      if (((epoch % 3600) / 60) < 10) {
        // In the first 10 minutes of each hour, we'll want a leading '0'
        Serial.print('0');
      }
      Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
      Serial.print(':');
      if ((epoch % 60) < 10) {
        // In the first 10 seconds of each minute, we'll want a leading '0'
        Serial.print('0');
      }
      Serial.println(epoch % 60); // print the second
    }
    
    return true;
  }
  return false;
}

int doWork(int wakeReason)
{
  // Fake work - get time
  // https://www.arduino.cc/en/Tutorial/UdpNTPClient
  
  pinMode(LED_BUILTIN, OUTPUT);
  // Detect WiFi stall
  digitalWrite(LED_BUILTIN, HIGH);

  int wifiCount = 0;

  WiFi.begin(ssid, password);
  if (VERBOSE) Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED && wifiCount < 5){
    if (VERBOSE) Serial.print(".");
    wifiCount += 1;
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
  }
  
  digitalWrite(LED_BUILTIN, LOW);

  if (wifiCount < 5) {

    if (VERBOSE) Serial.printf("\n...Connected to %s!\n", ssid);

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

    //printf("%"PRIu64" esp timer\n", esp_timer_get_time());

    //Serial.print("Response = ");
    //Serial.println(http.getString().toInt());

    http.end();
    free(temp_str);

    /*Udp.begin(localPort); // A UDP instance to let us send and receive packets over UDP
    if (VERBOSE) Serial.println("Requesting Time via NTP...");
    sendNTPpacket(timeServer); // send an NTP packet to a time server
    int tries = 1;
    int NTPDown = 1;
    delay(750);
    while (!checkNTP()) {
      Serial.printf("No response yet, try %d/%d...", NTPDown, tries);
      if (tries++ > 10) {
        sendNTPpacket(timeServer);
        tries == 0;
        NTPDown++;
        if (NTPDown > 6) {
          break;
        }
      }
      delay(500);
    }

    if (NTPDown > 6) {
      // Sleep for an hour if can't get NTP response
      Serial.println("No NTP response for 30 seconds, sleeping for 1 hour...");
      Serial.flush();
      esp_deep_sleep(3600000000);
    }*/

    WiFi.disconnect();
    return TRUE;
  } else {
    if (VERBOSE) Serial.print("Could not connect to WiFi in a suitable timeframe.");
    return FALSE;
  }
}
