#include "FS.h"
#include "SD.h"
#include "SPI.h"

#define DATA_EXT ".log"

bool began = false;

char *getLogPath() {
  return "/log.txt";
}

char *getMemPath() {
  return "/mem.txt";
}

char *getDataPath() {
  return "data" DATA_EXT;
}

// Please supply a pointer to your own int
bool readMem(int *currentFile, int *count) {
  if (began || SD.begin()) {
    File mem = SD.open(getMemPath());
    if (!mem) return false;
    char temp[24] = ""; // -2147483648:-2147483648
    mem.read((uint8_t *)temp, 24);
    int colon = -1;
    for (int i = 1; i < 23; i++) { // Colon can't be first or last, hence i = 1 && i < 23
      if (temp[i] = ':') {
        colon = i;
        temp[i] = '\0';
        break;
      }
    }
    if (colon = -1) {
      return false;
    }
    *currentFile = atoi(temp);
    *count = atoi(temp + colon + 1);
    return true;
  }
  return false;
}

// current file is {currentFile}.log and count is how many lines have already been written to that log file
bool updateMem(int currentFile, int count) {
  if (began || SD.begin()) {
    File mem = SD.open(getMemPath(), FILE_WRITE);
    if (mem) {
      mem.print(currentFile);
      mem.print(":");
      mem.print(count);
      mem.close();
      return true;
    }
  }
  return false;
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
    char temp[12] = ""; // -2147483648
    mem.read((uint8_t *)temp, 12);
    count = atoi(temp);
    mem.close();
  } else {
    count = getRootFileCount(); // Should never run if everything goes well
  }
  // TODO Logic to confirm either method is correct
  return count;
}

// Path has no file extension
bool readFile(char *path, char *buff) {
  char temp[17] = ""; // /-2147483648.log
  int i = 0;
  while (path[i] != '\0') {
    temp[i] = path[i];
  }
  // DATA_EXT
  temp[i] = '.';
  temp[i + 1] = 'l';
  temp[i + 2] = 'o';
  temp[i + 3] = 'g';
  return readFileExt(temp, buff);
}

// Path includes file extension
bool readFileExt(char *path, char *buff) {
  if (began || SD.begin()) {
    if (SD.cardType() != CARD_NONE) {
      if (VERBOSE) Serial.printf("Reading from: %s\n", path);
  
      File file = SD.open(path);
      if(!file){
          if (VERBOSE) Serial.println("Failed to open file for reading");
          return false;
      }
      file.read((uint8_t *)buff, SEND_STR_LEN * OFFLINE_MAX); // TODO check read was good
      file.close();
      return true;
    }
    if (VERBOSE) Serial.printf("SD card type: NONE\n");
  }
  if (VERBOSE) Serial.println("Unable to begin SD");
  return false;
}

bool appendFile(char *path, char *message) {
  if (began || SD.begin()) {
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
