#include <Arduino.h>

#ifndef FILE
#define FILE

/// Cannot be greater than 230
#define DATA_SECTOR_SIZE 230
#define DATA_SECTORS_MAX 130
#define DATA_HEAD 0x88


enum FILE_PROGRESS {
  ITERATING_SECTORS,
  MISSING_SECTORS,
  CHECKSUM_FAILED,
  FILE_COMPLETE,
  FILE_CORRUPT,
};

struct FileHeader {
  byte head;
  uint16_t dataSize;
  uint16_t pendingBytes;
  byte nextSector;
  byte sectorCount;
  byte sectorMask[DATA_SECTORS_MAX/8+1];
  bool isPending;
  uint32_t checksum;
};

struct FileStructure {
  FileHeader header;
  byte data[DATA_SECTORS_MAX][DATA_SECTOR_SIZE];
};

byte verifyFile();
void writeHeader();
bool writeWholeFile(byte *data, uint16_t dataSize);
bool setupStorage();
bool readFile();
bool writeDefaultFile();
void readWriteTest();
void printHeader(FileHeader header);
bool writeSector(byte *data, uint16_t dataSize, byte sector);
void initFileReceiver(uint32_t checksum, byte sectorCount, uint16_t dataSize);
void readEntireChip();

#endif