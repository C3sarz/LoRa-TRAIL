#include "lora.h"
#include "data_transfer.h"
#include "send_mqtt.h"

extern FileStructure file;
const uint32_t maxDataSize = sizeof(FileStructure) - sizeof(FileHeader);
volatile bool txReady = false;
volatile bool rxReady = false;
extern byte transferState;
static RX_HANDLERS callbackHandler = {
  onRx,
  onTimeout,
  TxDone
};
static byte txRecipient;


const byte maxPayloadSize = 250;
byte dataBuffer[maxPayloadSize];

//=========================================================

void onRx(uint8_t* payload, uint16_t size, int16_t rssi, int8_t snr) {
  rxReady = false;

  // Validate header size
  if (size < sizeof(TX_HEADER)) {
    return;
  }

  // Validate and copy
  TX_HEADER baseHeader;
  memcpy(&baseHeader, payload, sizeof(TX_HEADER));

  Serial.printf("RX PACKET  (%d,%d):\r\n",rssi,snr);
  for (int i = 0; i < size; i++) {
    Serial.printf("%x ", payload[i]);
  }
  Serial.println();

  if (baseHeader.SYNCWORD != TX_SYNC_WORD) {
    return;
  }

  txRecipient = baseHeader.recipient;
  switch (baseHeader.opcode) {
    case OPCODE_HANDSHAKE_REQ:
      transferState = TRANSFER_REQUESTED_HANDSHAKE;
      break;
    case OPCODE_HANDSHAKE_RESP:
      Serial.println("TX RESPONSE");
      onTransferResponse(payload, size);
      transferState = TRANSFER_REQUEST_DATA;
      break;
    case OPCODE_DATAGRAM_REQ:
      onDatagramRequest(payload, size);
      transferState = TRANSFER_SENDING_DATA;
      break;
    case OPCODE_DATAGRAM_RESP:
      if (receiveDatagram(payload, size)) {
      }
      transferState = TRANSFER_WAIT;
      break;
    case OPCODE_HEARTBEAT:
    case OPCODE_LINK:
    case OPCODE_ERROR:
    default:
      Serial.printf("Invalid RX Opcode: %u\r\n", baseHeader.opcode);
  }
  return;
}


void dataTransferStateLogic() {

  switch (transferState) {
    case TRANSFER_REQUESTED_HANDSHAKE:
      Serial.println("TX_REQ");
      onTransferRequest(txRecipient);
      transferState = TRANSFER_LISTEN;
      break;
    case TRANSFER_LISTEN:
#ifdef SERVER
      tryRx(0);
#else
      tryRx(10000);
#endif
      transferState = TRANSFER_BUSY;
      break;

    // Get next datagram
    case TRANSFER_REQUEST_DATA:
      if (verifyFile() == ITERATING_SECTORS) {
        Serial.printf("==================\r\nGet datagram %u\r\n", file.header.nextSector);
        delay(500);

        getDatagram(file.header.nextSector, txRecipient);
        transferState = TRANSFER_LISTEN;
      } else {
        transferState = TRANSFER_COMPLETE;
      }
      break;
    case TRANSFER_SENDING_DATA:
      delay(500);
      sendDatagram(file.header.nextSector, txRecipient);
      transferState = TRANSFER_IDLE;
      break;
    case TRANSFER_WAIT:
      delay(3000);
      transferState = TRANSFER_REQUEST_DATA;
      break;

    case TRANSFER_COMPLETE:
      digitalWrite(LED_BLUE, false);
      // tx complete
      break;
    case TRANSFER_BUSY:
      break;
    case TRANSFER_IDLE:
    default:

#ifdef SERVER
      transferState = TRANSFER_LISTEN;
#else
      transferState = TRANSFER_IDLE;
#endif
  }
}

//=========================================================

///
///
///
void onTimeout() {
  transferState = TRANSFER_IDLE;
}

///
///
///
void TxDone() {

  transferState = TRANSFER_LISTEN;
}

///
///
///
void initTransferLogic() {

  setupRx(&callbackHandler);
  Serial.println("Transfer logic setup");
  return;
}

///
///
///
void requestTransfer(byte recipient) {
  // Build header
  TransferStartReq header;
  header.recipient = recipient;
  txRecipient = recipient;

  // Copy header
  memcpy(dataBuffer, &header, sizeof(header));

  // Send LORA
  send(dataBuffer, sizeof(header));
  transferState = TRANSFER_IDLE;
  return;
}

/// Prepare device for transmission, as requested
///
///
void onTransferRequest(byte recipient) {
  // Read file from FRAM
  readFile();
  digitalWrite(LED_BLUE, true);

  // Build header
  TransferStartResp header;
  txRecipient = recipient;
  header.recipient = recipient;
  header.checksum = file.header.checksum;
  header.sectorCount = file.header.sectorCount;
  header.finalSize = file.header.dataSize;

  // Copy header
  memcpy(dataBuffer, &header, sizeof(header));

  // Send LORA
  send(dataBuffer, sizeof(header));
  return;
}

/// Prepare device for transmission, as requested
///
///
bool onDatagramRequest(byte* rcvBuf, byte rcvBufSize) {

  DatagramReq header;

  // Validate header size
  if (rcvBufSize < sizeof(header)) {
    return false;
  }

  // copy
  memcpy(&header, rcvBuf, sizeof(header));


  // Validate header
  if (header.sector > file.header.sectorCount || header.checksum != file.header.checksum) {
    // if (header.sector > file.header.sectorCount) {
    return false;
  }

  // Update header
  file.header.nextSector = header.sector;
  Serial.printf("Sector %u will be sent.\r\n", file.header.nextSector);

  writeHeader();
  return true;
}

/// On transfer request accepted by sender
///
///
bool onTransferResponse(byte* rcvBuf, byte rcvBufSize) {

  // Validate header size
  if (rcvBufSize < sizeof(TransferStartResp)) {
    return false;
  }

  // Validate and copy
  TransferStartResp header;
  memcpy(&header, rcvBuf, sizeof(header));

  // Validate header
  if (header.sectorCount > DATA_SECTORS_MAX || header.finalSize > maxDataSize) {
    return false;
  }

  // Prepare file on FRAM
  initFileReceiver(header.checksum, header.sectorCount, header.finalSize);

  return true;
}

/// Ask for specific sector to server.
///
///
void getDatagram(byte sector, byte recipient) {

  // Build header
  DatagramReq header;
  header.recipient = recipient;
  header.sector = sector;

  // Verify if file is the same as before
  header.checksum = file.header.checksum;

  // Copy header
  memcpy(dataBuffer, &header, sizeof(header));

  // Send LORA
  send(dataBuffer, sizeof(header));
  return;
}

/// Send specific sector to server.
///
///
bool sendDatagram(byte sector, byte recipient) {
  byte sectorCount = file.header.sectorCount;
  void* sectorAddr = file.data[sector];
  double dataSize_D = static_cast<double>(file.header.dataSize);
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
///
///
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
  if (header.sector > file.header.sectorCount) {
    return false;
  }

  // Copies data and increments nextSector
  bool result = writeSector(rcvBuf + sizeof(DatagramResp), header.dataLen, header.sector);
  return result;
}
