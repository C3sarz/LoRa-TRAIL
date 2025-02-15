#include <Arduino.h>
#include "storage_manager.h"

enum TX_OPCODE{
  OPCODE_START = 0x11,
  OPCODE_SEND,
  OPCODE_END,
  OPCODE_REPEAT,
  OPCODE_HEARTBEAT,
  OPCODE_LINK,
};

struct TX_HEADER {
  TX_OPCODE opcode;
  uint32_t txID;
  byte sector;
  byte sectorCount;
  byte dataLen;
};
struct TX_PAYLOAD {
  TX_HEADER header;
  byte data[DATA_SECTOR_SIZE];
};

bool startTransfer(FileStructure payload);
void sendDatagram();