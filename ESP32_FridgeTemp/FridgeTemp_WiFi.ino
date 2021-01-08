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
  Serial.println("");

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
  httpResponse = http.POST(toSend);
  //String reply = http.getString(); // Ideally we don't use String class, but idk another method to get reply
  http.end();
  if (VERBOSE) Serial.printf("HTTP response: %d\n", httpResponse);
  //if (VERBOSE) Serial.printf("HTTP reply: %s\n", reply);
  return httpResponse;
}

int WiFiCount = 0;
// May change toSend contents
int WiFiSendPacket(char *address, char *toSend) {
  if (!WiFiBegan) WiFiConnect();
  HTTPClient http;
  http.begin(address);
  http.addHeader("Content-Type", "text/plain");
  httpResponse = http.POST(toSend);
  String reply = http.getString(); // Ideally we don't use String class, but idk another method to get reply
  http.end();
  if (VERBOSE) Serial.printf("HTTP response: %d\n", httpResponse);
  if (VERBOSE) Serial.printf("HTTP reply: %s\n", reply);
  WiFiCount += 1;
  if (reply.toInt() != WiFiCount) {
    Serial.printf("WiFiCount mismatch: %d, %s\n", WiFiCount, reply);
    sprintf(toSend, "WiFiCount mismatch: %d, %s\n", WiFiCount, reply);
    appendFile(getLogPath(), toSend);
    return -2; // Meaning data should be resent from the start
  }
  return httpResponse;
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
  http.begin(ADDR_SD_STATUS);
  http.addHeader("Content-Type", "text/plain");
  // C:\Users\User\AppData\Local\Arduino15\packages\esp32\hardware\esp32\1.0.4\libraries\WiFi\src\WiFiClient.cpp
  // line 107 add fillBuffer();
  // else core panic
  if (useSD) {
    httpResponse = http.PUT("true");
  } else {
    httpResponse = http.PUT("false");
  }
  String temp = http.getString();
  if (VERBOSE && httpResponse == 200) Serial.printf("SD http reply: %s\n", temp);
  if (VERBOSE && httpResponse != 200) Serial.printf("Failed to send SD status, response: %d\n", httpResponse);
  http.end();
}

// Should be called when we're 100% finished with WiFi, before sleeping
void WiFiEnd() {
  if (!WiFiBegan) return;
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  btStop();
  WiFiBegan = false;
}
