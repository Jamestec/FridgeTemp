#include "WiFi.h"
#include <HTTPClient.h>

bool WiFiBegan = false;

bool WiFiConnect() {
  // Detect WiFi stall visually on board
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  int wifiCount = 0;

  WiFi.begin(ssid, password);
  if (VERBOSE) Serial.printf("Connecting to %s...", ssid);
  while (WiFi.status() != WL_CONNECTED && wifiCount < TIMEOUT) {
    if (VERBOSE && wifiCount % WIFI_DOT_LINE == 0) Serial.println("");
    if (VERBOSE) Serial.print(".");
    wifiCount += 1;
    digitalWrite(LED_BUILTIN, HIGH);
    delay(WIFI_DOT_INTERVAL / 2);
    digitalWrite(LED_BUILTIN, LOW);
    delay(WIFI_DOT_INTERVAL / 2);
  }
  digitalWrite(LED_BUILTIN, LOW);
  if (VERBOSE) Serial.println("");

  if (wifiCount < TIMEOUT) {
    if (VERBOSE) {
      Serial.printf("...Connected to %s in %.2f second(s)!\n", ssid, WIFI_DOT_INTERVAL_SEC * wifiCount);
      Serial.printf("Sending to %s:\n", ADDR);
      Serial.println(send_str);
    }
    WiFiBegan = true;
    return true;
  }
  if (VERBOSE) Serial.printf("\nCould not connect to %s in %.2f seconds.\n", ssid, TIMEOUT * WIFI_DOT_INTERVAL_SEC);
  WiFiEnd();
  return false;
}

int WiFiSend(char *address, char *toSend) {
  if (!WiFiBegan) WiFiConnect();
  HTTPClient http;
  http.begin(address);
  http.addHeader("Content-Type", "text/plain");
  int response = http.POST(toSend);
  http.end();
  if (VERBOSE) Serial.printf("HTTP response: %d\n", response);
  return response;
}

int WiFiCount = 0;
// May change toSend contents
int WiFiSendPacket(char *address, char *toSend) {
  if (!WiFiBegan) WiFiConnect();
  if (VERBOSE) Serial.printf("Packet %d: %s\n", WiFiCount + 1, toSend);
  HTTPClient http;
  http.begin(address);
  http.addHeader("Content-Type", "text/plain");
  int response = http.POST(toSend);
  String reply = http.getString(); // Ideally we don't use String class, but idk another method to get reply
  http.end();
  if (VERBOSE) Serial.printf("HTTP response: %d\n", response);
  if (VERBOSE) Serial.printf("HTTP reply: %s\n", reply);
  WiFiCount += 1;
  if (reply.toInt() != WiFiCount) {
    Serial.printf("WiFiCount mismatch: %d, %s\n", WiFiCount, reply);
    sprintf(toSend, "WiFiCount mismatch: %d, %s\n", WiFiCount, reply);
    if (response == 200) appendFile(getLogPath(), toSend); // Only actually log if server replied weird, don't log internal error etc.
    return -2; // Meaning data should be resent from the start
  }
  return response;
}

// Call this after you have finished using WiFiSendPacket(...)
boolean WiFiSendPacketFin() {
  if (!WiFiBegan) WiFiConnect();
  WiFiCount = 0;
  HTTPClient http;
  http.begin(ADDR_PACKET_FIN);
  if (http.GET() != 200) {
    if (VERBOSE) Serial.println("Packet Finish signal failed");
    http.end();
    return false;
  }
  http.end();
  return true;;
}

boolean WiFiSendPacketReset() {
  if (!WiFiBegan) WiFiConnect();
  WiFiCount = 0;
  HTTPClient http;
  http.begin(ADDR_PACKET_RESET);
  if (http.GET() != 200) {
    if (VERBOSE) Serial.println("Packet Reset failed");
    http.end();
    return false;
  }
  http.end();
  return true;
}

void WiFiSendSDStatus() {
  if (!WiFiBegan) WiFiConnect();
  HTTPClient http;
  int response;
  http.begin(ADDR_SD_STATUS);
  http.addHeader("Content-Type", "text/plain");
  // C:\Users\User\AppData\Local\Arduino15\packages\esp32\hardware\esp32\1.0.4\libraries\WiFi\src\WiFiClient.cpp
  // line 107 add fillBuffer();
  // else core panic
  if (useSD) {
    response = http.PUT("true");
  } else {
    response = http.PUT("false");
  }
  String temp = http.getString();
  if (VERBOSE && response == 200) Serial.printf("SD http reply: %s\n", temp);
  if (VERBOSE && response != 200) Serial.printf("Failed to send SD status, response: %d\n", response);
  http.end();
}

// returns Epoch/UNIX time or -1 if nope
int WiFiGetTime() {
  if (!WiFiBegan) WiFiConnect();
  HTTPClient http;
  http.begin(ADDR_TIME_REQ);
  int response = http.GET();
  int parsed = 0;
  if (response == 200) {
    parsed = http.getString().toInt();
    if (parsed > 1000000000) { // 2001
      http.end();
      return parsed;
    }
  }
  if (VERBOSE) {
    if (response != 200) {
      Serial.printf("Request for time failed, http response: %d\n", response);
    } else {
      Serial.printf("Request for time failed, parsed: %ul\n", parsed);
    }
  }
  http.end();
  return -1;
}

// Should be called when we're 100% finished with WiFi, before sleeping
void WiFiEnd() {
  if (!WiFiBegan) return;
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  btStop();
  WiFiBegan = false;
}
