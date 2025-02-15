#include <sys/_stdint.h>
#include "lora.h"
#include "data_transfer.h"

extern FileStructure file;
volatile bool txReady = true;
extern bool filePresent;
volatile bool txOngoing = false;
TX_PAYLOAD payload;
byte dataBuffer[sizeof(TX_PAYLOAD)];

// Start sending main payload in fragments.
bool startTransfer(FileStructure payload) {
  if (txOngoing) {
    Serial.println("Sender busy!");
    return false;
  }

  if (!filePresent) {
    Serial.println("No file present!");
    return false;
  }
  file.header.isPending = true;
  file.header.lastIndex = 0;

  txReady = true;

  return false;
}

void sendDatagram(){

  byte currentIndex = file.header.lastIndex;
  byte sectorCount = file.header.sectorCount;
  void * sectorAddr = file.data[currentIndex];
  double dataSize_D = static_cast<double>(file.header.dataSize);

  uint16_t remainingBytes = static_cast<uint16_t>(dataSize_D-dataSize_D*(currentIndex/(double)sectorCount));

// create logic for payloads under 230B

  //  = file.header.

  if(currentIndex == 0){
    payload.header.opcode = OPCODE_START;
  }
  else if(currentIndex < sectorCount){
    payload.header.opcode = OPCODE_SEND;
  }
    else if(currentIndex >= sectorCount){
    payload.header.opcode = OPCODE_END;
  }


  payload.header.txID = file.header.checksum;
  payload.header.sector = currentIndex;
  payload.header.sectorCount = sectorCount;

// Copy data
  memcpy(payload.data, sectorAddr, DATA_SECTOR_SIZE);

  // Copy payload to buffer
  memcpy(dataBuffer, &payload, sizeof(TX_HEADER)+payload.header.dataLen);
  
  // Send LORA
  send(dataBuffer, sizeof(sizeof(TX_HEADER)+payload.header.dataLen));
}

// RX
bool receiveDatagram(byte* rcvBuf, byte rcvBufSize){
  
  if(rcvBufSize < sizeof(TX_PAYLOAD)){
    return false;
  }
  memcpy(&payload, rcvBuf, rcvBufSize);

  
  // LOGIC FOR RX missing

  if(OPCODE_START){

  currentIndex = file.header.lastIndex;
  byte sectorCount = file.header.sectorCount;
  void * sectorAddr = file.data[currentIndex];
  }
  else if(OPCODE_SEND){

  }
  else if(OPCODE_END){

  }

  return false;
}

