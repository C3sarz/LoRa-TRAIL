
#include "storage_manager.h"
#include "lora.h"
#include "data_transfer.h"

extern FileStructure file;
bool filePresent = false;
const bool sender = true;
extern volatile bool txOngoing;


void setup() {

  // Init serial
  time_t timeout = millis();
  Serial.begin(115200);
  while (!Serial) {
    if ((millis() - timeout) < 5000) {
      delay(100);
    } else {
      break;
    }
  }
  setupStorage();
  filePresent = readFile();
  txOngoing = !filePresent || !(file.header.isPending);

  delay(1000);
}


void loop() {
  if (Serial.available() > 0) {

    // Serial console debug commands
    int rcvd = Serial.read();
    Serial.printf("Rcvd: %c\r\n", rcvd);
    switch (rcvd) {
      case 's':
        if (sender) startTransfer(file);
        break;
      case 'd':
        // printSummary();
        // printFWInfo();
        // Serial.printf("Current DR: %u\r\n", getADRDatarate());
        break;
      case 't':
        {
          readWriteTest();
          break;
        }
      case 'f':
        {
          readFile();
          break;
        }
      case 'w':
        // tryWriteStoredConfig();
        break;
      case 'v':
        verifyFile();
        break;
      case 'l':
        writeDefaultFile();
        break;
      case 'r':
        // Serial.println("Reboot requested...");
        // setReboot = true;
        break;
    }
  }
}