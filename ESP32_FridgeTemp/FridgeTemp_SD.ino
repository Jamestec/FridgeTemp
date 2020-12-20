#include "FS.h"
#include "SD.h"
#include "SPI.h"

#define DATA_EXT ".log"

bool began = false;

char *getSDPath() {
  return "/log.txt";
}

char *getMemPath() {
  return "/mem.txt";
}

int getRootFileCount() {
  int count = -1;
  if (began || SD.begin()) {
    count = 0;
    File root = SD.open("/");
    File entry = root.openNextFile();
    while (entry) {
      if (!entry.isDirectory()) {
        count += 1;
      }
      entry.close();
      entry = root.openNextFile();
    }
    root.close();
  }
  return count;
}

int getDataFileCount() {
  int count = -1;
  if (began || (SD.begin() && SD.exists(getMemPath())) && false) { // TODO actually write the number
    File mem = SD.open(getMemPath());
    char temp[11] = ""; // 2147483647
    mem.read((uint8_t *)temp, 11);
    count = atoi(temp);
    mem.close();
  } else {
    count = getRootFileCount(); // Should never run if everything goes well
  }
  // TODO Logic to confirm either method is correct
  return count;
}

bool appendFile(char *path, char *message) {
  if (began || SD.begin()) {
    if (SD.cardType() != CARD_NONE) {
      Serial.print("SD Card Type: ");
      switch (SD.cardType()) {
        case CARD_MMC: Serial.println("MMC"); break;
        case CARD_SD: Serial.println("SD"); break;
        case CARD_SDHC: Serial.println("SDHC"); break;
        default: Serial.println("UNKNOWN"); break;
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
