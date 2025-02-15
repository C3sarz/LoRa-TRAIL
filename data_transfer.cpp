#include "lora.h"
#include "data_transfer.h"

extern FileStructure localFile;
extern const uint32_t maxDataSize;
volatile bool txReady = false;
extern bool filePresent;
volatile bool transferOngoing = false;
volatile bool rxReady = false;
byte server;

const byte maxPayloadSize = 250;
byte dataBuffer[maxPayloadSize];

//=========================================================





//=========================================================




void requestTransfer(byte recipient){
  // Build header
  TransferStartReq header;
  header.recipient = recipient;

  // Copy header
  memcpy(dataBuffer, &header, sizeof(header));

  // Send LORA
  send(dataBuffer, sizeof(header));
  rxReady = true;
  return;
}

/// Prepare device for transmission, as requested
void onTransferRequest(byte recipient){
  // Read file from FRAM
  // readFile();

  // Build header
  TransferStartResp header;
  header.recipient = recipient;
  header.checksum = localFile.header.checksum;
  header.sectorCount = localFile.header.sectorCount;
  header.finalSize = localFile.header.dataSize;

  // Copy header
  memcpy(dataBuffer, &header, sizeof(header));

  // Send LORA
  send(dataBuffer, sizeof(header));
  return;
}

/// On transfer request accepted by sender
bool onTransferResponse(byte* rcvBuf, byte rcvBufSize){

  // Validate header size
  if (rcvBufSize < sizeof(TransferStartResp)) {
    return false;
  }

  // Validate and copy
  TransferStartResp header;
  memcpy(&header, rcvBuf, sizeof(TransferStartResp));

  // Validate header
  if(header.sectorCount > DATA_SECTORS_MAX || header.finalSize > maxDataSize){
    return false;
  }
  
  // Prepare file on FRAM
  initFileReceiver(header.checksum, header.sectorCount, header.finalSize);

  // ENABLE DATA RECEPTION LOOP
  transferOngoing = true;
  txReady = true;
  return true;
}

/// Send specific sector to server.
bool sendDatagram(byte sector, byte recipient) {
  byte sectorCount = localFile.header.sectorCount;
  void* sectorAddr = localFile.data[sector];
  double dataSize_D = static_cast<double>(localFile.header.dataSize);
  uint16_t remainingBytes = static_cast<uint16_t>(dataSize_D - dataSize_D * (sector / (double)sectorCount));
  byte dataLen = 0;
  DatagramResp header;

  // create logic for payloads under 230B
  if (remainingBytes < DATA_SECTOR_SIZE) {
    dataLen = remainingBytes;
  } else {
    dataLen = DATA_SECTOR_SIZE;
  }

  // Build header
  header.dataLen = dataLen;
  header.sector = sector;
  header.recipient = recipient;
  header.sender = DEVICE_ID;

  // Copy header
  memcpy(dataBuffer, &header, sizeof(header));

  // Copy payload to buffer
  memcpy(dataBuffer + sizeof(header), sectorAddr, dataLen);

  // Send LORA
  send(dataBuffer, sizeof(header) + dataLen);
  return true;
}

// RX
bool receiveDatagram(byte* rcvBuf, byte rcvBufSize) {

  // Validate header size
  if (rcvBufSize < sizeof(DatagramResp)) {
    return false;
  }

  // Validate and copy dataLen
  DatagramResp header;
  memcpy(&header, rcvBuf, sizeof(DatagramResp));
  if (header.dataLen > rcvBufSize - sizeof(DatagramResp)
      || header.dataLen > DATA_SECTOR_SIZE) {
    return false;
  }

// Validate header
  if(header.sector > localFile.header.sectorCount){
    return false;
  }

  // Copy data
  bool result = writeSector(rcvBuf + sizeof(DatagramResp), header.dataLen, header.sector);

  return result;
}
