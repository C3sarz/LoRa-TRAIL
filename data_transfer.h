#include <Arduino.h>
#include "storage_manager.h"
#include "conf.h"

enum TX_OPCODE {
  OPCODE_HANDSHAKE = 0x10,
  OPCODE_SEND,
  OPCODE_HEARTBEAT,
  OPCODE_LINK,
  OPCODE_ERROR,
};

class TX_HEADER {
  const byte SYNCWORD = 0x88;
protected:
  byte opcode;
public:
  byte sender= DEVICE_ID;
  byte recipient;
};


class DatagramReq : public TX_HEADER {
protected:
  byte opcode = OPCODE_SEND;
public:
  byte sector;
};

class DatagramResp : public TX_HEADER {
protected:
  byte opcode = OPCODE_SEND;
public:
  byte sector;
  byte dataLen = 0;
};

class TransferStartReq : public TX_HEADER {
protected:
  byte opcode = OPCODE_HANDSHAKE;
};

class TransferStartResp : public TX_HEADER {
protected:
  byte opcode = OPCODE_HANDSHAKE;
public:
  byte sectorCount;
  uint16_t finalSize;
  uint32_t checksum;
};


bool startTransfer(FileStructure payload);
void requestTransfer(byte recipient);
void onTransferRequest(byte recipient);
bool onTransferResponse(byte* rcvBuf, byte rcvBufSize);
bool sendDatagram(byte sector, byte recipient);
bool receiveDatagram(byte* rcvBuf, byte rcvBufSize);