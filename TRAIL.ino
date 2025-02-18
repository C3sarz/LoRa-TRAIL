
#include "storage_manager.h"
#include "lora.h"
#include "data_transfer.h"
#include "conf.h"
#include "send_mqtt.h"

extern FileStructure file;
bool filePresent = false;
const bool sender = true;
extern byte transferState;

// Timing
extern unsigned long uplinkPeriod;
unsigned long lastRequest = millis();
unsigned long periodResult;


void setup() {


  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_BLUE, LOW);

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

#ifdef SERVER
  Serial.printf("===============\r\n TRAIL STARTING\r\nROLE:%s\r\n===============\r\n", "Server");
#else

  Serial.printf("===============\r\n TRAIL STARTING\r\nROLE:%s\r\n===============\r\n", "CLIENT");
#endif

  delay(100);
  setupStorage();
  readFile();
  initTransferLogic();
  delay(500);
  setupRF();
  mqttSetup();


  Serial.println("Setup done");

  // MISSING RESTORE STATE



  delay(2000);
}

void loop() {

  if (Serial.available() > 0) {

    // Serial console debug commands
    int rcvd = Serial.read();
    Serial.printf("Rcvd: %c\r\n", rcvd);
    switch (rcvd) {
      case 's':
#ifndef SERVER
        requestTransfer(0x06);
#endif
        break;
      case 'd':
        printHeader(file.header);
        break;
      case 't':
        {
          sendFileMqtt(file);
          break;
        }
      case 'f':
        {
          readFile();
          break;
        }
      case 'l':
        readEntireChip();
        break;
      case 'v':
        verifyFile();
        break;
      case 'w':
        writeDefaultFile();
        break;
      case 'r':
        // Serial.println("Reboot requested...");
        // setReboot = true;
        break;
    }
  }

  dataTransferStateLogic();
  mqttKeepAlive();
}