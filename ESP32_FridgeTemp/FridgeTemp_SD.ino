#include "FS.h"
#include "SD.h"
#include "SPI.h"

#define DATA_EXT ".log"

bool SDBegan = false;

char *getLogPath() {
  return "/log.txt";
}

char *getDataPath() {
  return "/data" DATA_EXT;
}

// Assumues dataLoc is a propper path (e.g. "/data.log")
// Assumes buff is of size SEND_STR_LEN * OFFLINE_MAX
// Returns 0 if can't use SD
// Returns -2 if we should resend data
// Returns the http response code if it's not 200
// Returns 200 if everything went swimmingly
int sendData(char *dataLoc, char *buff) {
  if (SDBegan || SD.begin()) {
    if (SD.cardType() != CARD_NONE) {
      if (VERBOSE) Serial.printf("Reading from: %s\n", dataLoc);

      File file = SD.open(dataLoc);
      if (!file) {
          if (VERBOSE) Serial.println("Failed to open file for reading");
          return 0;
      }

      int response = 0;
      if (!WiFiSendPacketReset()) return -2;
      while (file.available()) {
        file.read((uint8_t *)buff, SEND_STR_LEN * OFFLINE_MAX); // TODO check read was good
        response = WiFiSendPacket(ADDR_PACKET, buff);
        if (response != 200) break;
      }
      file.close();
      WiFiSendPacketFin();
      return response;
    }
    if (VERBOSE) Serial.printf("SD card type: NONE\n");
  }
  if (VERBOSE) Serial.println("Unable to begin SD");
  return 0;
}

bool appendFile(char *path, char *message) {
  if (SDBegan || SD.begin()) {
    if (SD.cardType() != CARD_NONE) {
      if (VERBOSE) {
        Serial.print("SD Card Type: ");
        switch (SD.cardType()) {
          case CARD_MMC: Serial.println("MMC"); break;
          case CARD_SD: Serial.println("SD"); break;
          case CARD_SDHC: Serial.println("SDHC"); break;
          default: Serial.println("UNKNOWN"); break;
        }
      }
      if (VERBOSE) Serial.printf("Appending to file: %s\n", path);
  
      File file = SD.open(path, FILE_APPEND);
      if(!file){
          file = SD.open(path, FILE_WRITE);
      }
      if(!file){
          if (VERBOSE) Serial.println("Failed to open file for appending");
          return false;
      }
      if(file.print(message)){
          if (VERBOSE) Serial.printf("Message appended: %s\n", message);
      } else {
          if (VERBOSE) Serial.println("Append failed");
          file.close();
          return false;
      }
      file.close();
      return true;
    }
    if (VERBOSE) Serial.printf("SD card type: NONE\n");
  }
  if (VERBOSE) Serial.println("Unable to begin SD");
  return false;
}

void removeFile(char *path) {
  SD.remove(path);
}
