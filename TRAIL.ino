
#include "storage_manager.h"
#include "lora.h"
#include "data_transfer.h"
#include "conf.h"

extern FileStructure localFile;
bool filePresent = false;
const bool sender = true;
extern volatile bool transferOngoing;
extern volatile bool rxReady;

// Timing
extern unsigned long uplinkPeriod;
unsigned long lastRequest = millis();
unsigned long periodResult;


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
  transferOngoing = !filePresent || !(localFile.header.isPending);

  loraSetup();


  Serial.printf("===============\r\n TRAIL STARTING\r\nROLE:%s\r\n===============\r\n",SERVER == 1 ? "Server" : "Client");
  delay(1000);
}

void loop() {

  if(rxReady){
    tryRx(20000);
  }

  if (Serial.available() > 0) {

    // Serial console debug commands
    int rcvd = Serial.read();
    Serial.printf("Rcvd: %c\r\n", rcvd);
    switch (rcvd) {
      case 's':
      #ifndef SERVER
        requestTransfer(0x06);
        Radio.Rx(10000);
        #endif
        break;
      case 'd':
        break;
      case 't':
        {
          // readWriteTest();
          break;
        }
      case 'f':
        {
          readFile();
          rxReady = true;
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