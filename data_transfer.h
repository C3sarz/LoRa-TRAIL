#include <Arduino.h>
#include "storage_manager.h"
#include "conf.h"

#define TX_SYNC_WORD 0x88
enum TX_OPCODE {
  OPCODE_HANDSHAKE_REQ = 0x10,
  OPCODE_HANDSHAKE_RESP,
  OPCODE_DATAGRAM_REQ,
  OPCODE_DATAGRAM_RESP,
  OPCODE_HEARTBEAT,
  OPCODE_LINK,
  OPCODE_ERROR,
};

class TX_HEADER {
public:
  byte SYNCWORD = TX_SYNC_WORD;
  byte opcode = 0xFF;
  byte sender = DEVICE_ID;
  byte recipient;
};


class DatagramReq : public TX_HEADER {
public:
  byte sector;
  uint32_t checksum;
  DatagramReq() {
    opcode = OPCODE_DATAGRAM_REQ;
  }
};

class DatagramResp : public TX_HEADER {
public:
  byte sector;
  byte dataLen = 0;
  DatagramResp() {
    opcode = OPCODE_DATAGRAM_RESP;
  }
};

class TransferStartReq : public TX_HEADER {
public:
  TransferStartReq() {
    opcode = OPCODE_HANDSHAKE_REQ;
  }
};

class TransferStartResp : public TX_HEADER {
public:
  byte sectorCount;
  uint16_t finalSize;
  uint32_t checksum;
  TransferStartResp() {
    opcode = OPCODE_HANDSHAKE_RESP;
  }
};


bool startTransfer(FileStructure payload);
void requestTransfer(byte recipient);
void onTransferRequest(byte recipient);
bool onTransferResponse(byte* rcvBuf, byte rcvBufSize);
void getDatagram(byte sector, byte recipient);
bool sendDatagram(byte sector, byte recipient);
bool onDatagramRequest(byte* rcvBuf, byte rcvBufSize);
bool receiveDatagram(byte* rcvBuf, byte rcvBufSize);
void dataTransferStateLogic();
void initTransferLogic();
void onRx(uint8_t* payload, uint16_t size, int16_t rssi, int8_t snr);
void onTimeout();
void TxDone();