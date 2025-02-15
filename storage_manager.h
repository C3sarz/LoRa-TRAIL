#include <Arduino.h>

#ifndef FILE
#define FILE

#define DATA_SECTOR_SIZE 230
#define DATA_SECTORS_MAX 130
#define DATA_HEAD 0x88

struct FileHeader {
  byte head;
  uint16_t dataSize;
  uint16_t pendingBytes;
  byte lastIndex;
  byte sectorCount;
  byte sectorMask[DATA_SECTORS_MAX/8+1];
  bool isPending;
  uint32_t checksum;
};

struct FileStructure {
  FileHeader header;
  byte data[DATA_SECTORS_MAX][DATA_SECTOR_SIZE];
};

bool verifyFile();
bool writeFileSender(byte *data, uint16_t dataSize);
bool setupStorage();
bool readFile();
bool writeDefaultFile();
void readWriteTest();
void printHeader(FileHeader header);

#endif