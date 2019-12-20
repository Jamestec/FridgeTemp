#include "FS.h"
#include "SD.h"
#include "SPI.h"

char *getSDPath() {
  return "/log.txt";
}

bool appendFile(char *path, char *message) {
  if (SD.begin() && SD.cardType() != CARD_NONE) {
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
        return false;
    }
    file.close();
    return true;
  }
  return false;
}
