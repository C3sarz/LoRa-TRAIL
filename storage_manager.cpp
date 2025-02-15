#include <sys/_stdint.h>
#include "storage_manager.h"
#include "RAK1500x_MB85RC.h"
#include <Arduino_CRC32.h>
#include "testFile.cpp"

#define DEV_ADDR MB85RC_DEV_ADDRESS
#define WP_PING WB_IO1  // SlotA installation.

RAK_MB85RC MB85RC;
Arduino_CRC32 crc32;
FileStructure localFile;

const uint32_t fileSize = sizeof(FileStructure);
const uint32_t maxDataSize = fileSize - sizeof(FileHeader);

bool setupStorage() {

  // Wisblock module setup
  pinMode(WB_IO2, OUTPUT);
  pinMode(WP_PING, OUTPUT);
  digitalWrite(WB_IO2, HIGH);
  digitalWrite(WP_PING, LOW);

  // Init FRAM module comms
  Wire.begin();
  while (MB85RC.begin(Wire, DEV_ADDR) == false) {
    Serial.println("MB85RC is not connected.");
    while (1) {
      delay(1000);
    }
  }

  // Check FRAM model
  switch (MB85RC.getDeviceType()) {
    case MB85RC256:
      Serial.print("RAK15003(MB85RC256) ");
      break;
    case MB85RC512:
      Serial.print("RAK15003(MB85RC512) ");
      break;
    case MB85RC1:
      Serial.print("RAK15005(MB85RC1M) ");
      break;
    default:
      break;
  }

  // Good to go
  Serial.printf("FRAM initialized. Capacity: %u\r\n", MB85RC.getDeviceCapacity());
  return true;
}

/// Print file header details.
void printHeader(FileHeader header) {
  Serial.printf("==Header==\r\nSize: %u\r\nSectors: %u\r\nChecksum: %u\r\n", header.dataSize, header.sectorCount, header.checksum);
  Serial.printf("pendingBytes: %u\r\nlastIndex: %u\r\n", header.pendingBytes, header.lastIndex);
}


// Read file stored in FRAM
bool readFile() {
  const byte readBufferSize = DATA_SECTOR_SIZE;
  byte readBuffer[readBufferSize];
  FileHeader header;

  MB85RC.read(0, (uint8_t *)(&header), sizeof(FileHeader));
  printHeader(header);

  // Verify header
  if (localFile.header.head != DATA_HEAD
      && localFile.header.dataSize > maxDataSize) {
    return false;
  }

  // Copy header
  memcpy(&(localFile.header), &header, sizeof(FileHeader));

  for (uint i = 0; i < header.sectorCount; i++) {
    MB85RC.read(sizeof(FileHeader) + i * header.sectorCount, (uint8_t *)(readBuffer), readBufferSize);
  }

  if (verifyFile()) {
    Serial.println("File read successful!");
    return true;
  }

  return false;
}

bool writeDefaultFile() {
  Serial.println("Writing default file...");

  // Load test file
  extern const byte testFile[];
  extern const uint16_t testFileSize;

  bool result = writeFileSender((byte *)testFile, testFileSize);

  return result;
}

void initFileReceiver(uint32_t checksum, byte sectorCount, uint16_t dataSize) {
  // Sanity checks
  if (dataSize > maxDataSize) {
    return;
  }

  // Clear mask
  for (uint i = 0; i < (DATA_SECTORS_MAX / 8 + 1); i++) {
    localFile.header.sectorMask[i] &= 0;
  }

  // // Calculate header values
  // byte sectorCount = (byte)std::ceil(((double)dataSize) / (double)DATA_SECTOR_SIZE);

  // Create header
  localFile.header.dataSize = dataSize;
  localFile.header.pendingBytes = dataSize;
  localFile.header.head = DATA_HEAD;
  localFile.header.isPending = true;
  localFile.header.lastIndex = 0;
  localFile.header.sectorCount = sectorCount;
  localFile.header.checksum = checksum;

  // Write to chip
  MB85RC.write(0, (uint8_t *)&localFile, fileSize);
  return;
}

/// Write sector into storage.
bool writeSector(byte *data, uint16_t dataSize, byte sector) {

  // Sanity checks
  if (dataSize > DATA_SECTOR_SIZE || sector > DATA_SECTORS_MAX || localFile.header.head != DATA_HEAD) {
    return false;
  }

  // == Update Sector Mask
  // Submask in array
  byte subMaskIndex = sector / 8;

  // Selected bit in submask (bit in the array element)
  byte subMaskBit = (sector - 1) % 8;

  // Move bit to location and update mask.
  byte subMask = localFile.header.sectorMask[subMaskIndex] | (1 >> subMaskBit);
  localFile.header.sectorMask[subMaskIndex] = subMask;

  // == Modify header
  double dataSize_D = static_cast<double>(localFile.header.dataSize);
  uint16_t remainingBytes = static_cast<uint16_t>(dataSize_D - dataSize_D * (sector / (double)localFile.header.sectorCount));
  localFile.header.lastIndex = sector;
  localFile.header.pendingBytes = remainingBytes;

  // == Copy data
  memcpy(localFile.data[sector], data, dataSize);

  // Write to chip
  MB85RC.write(0, (uint8_t *)&localFile, fileSize);
  Serial.printf("Sector written to storage!\r\nSize: %u\r\nSector: %u\r\nRemaining: %u bytes\r\n", dataSize, sector, remainingBytes);
  return true;
}

// Write whole file into storage.
bool writeFileSender(byte *data, uint16_t dataSize) {

  // Sanity checks
  if (dataSize > maxDataSize) {
    return false;
  }

  // Copy data
  memcpy(localFile.data, data, dataSize);

  // Calculate header values
  byte sectorCount = (byte)std::ceil(((double)dataSize) / (double)DATA_SECTOR_SIZE);

  // Modify header
  localFile.header.dataSize = dataSize;
  localFile.header.pendingBytes = dataSize;
  localFile.header.head = DATA_HEAD;
  localFile.header.isPending = true;
  localFile.header.lastIndex = 0;
  localFile.header.sectorCount = sectorCount;


  uint32_t calc_checksum = crc32.calc((uint8_t *)(&localFile.data), localFile.header.dataSize);
  localFile.header.checksum = calc_checksum;

  // Write to chip
  MB85RC.write(0, (uint8_t *)&localFile, fileSize);

  if (verifyFile()) {
    Serial.printf("File written to storage!\r\nSize: %u\r\nSectors: %u\r\nChecksum: %u\r\n", dataSize, sectorCount, calc_checksum);
    return true;
  }

  return false;
}

bool verifyFile() {
  bool result = false;
  uint32_t calc_checksum = 0;

  // Verify FILE
  if (localFile.header.head == DATA_HEAD
      && localFile.header.dataSize <= maxDataSize) {
    uint32_t checksum = localFile.header.checksum;
    calc_checksum = crc32.calc((uint8_t *)localFile.data, localFile.header.dataSize);

    if (calc_checksum == checksum) {
      result = true;
    }
  }
  Serial.printf("Verify result: %u, checksum: %u\r\n", result, calc_checksum);
  // printHeader(file.header);
  return result;
}




// From RAK docs
void readWriteTest() {
  char writeBuf[16] = ">>Test RAK1500X";
  char readBuf[16] = { 0 };
  uint32_t successCount = 0;
  uint32_t failCount = 0;
  uint32_t productSize = MB85RC.getDeviceCapacity();
  float progress = 0;
  time_t interval = millis();

  for (uint32_t i = 0; i < productSize; i += sizeof(writeBuf)) {
    MB85RC.write(i, (uint8_t *)writeBuf, sizeof(writeBuf));
    MB85RC.read(i, (uint8_t *)readBuf, sizeof(readBuf));
    if (memcmp(writeBuf, readBuf, sizeof(readBuf)) == 0) {
      successCount++;
    } else {
      failCount++;
    }
    if ((millis() - interval) > 1000) {
      interval = millis();
      Serial.printf("Test progress: %5.2f%% , successCount: %ld , failCount:%ld \n", progress, successCount, failCount);
    }
    progress = (float)(i + sizeof(writeBuf)) * 100 / productSize;
    memset(readBuf, '0', sizeof(readBuf));
    delay(1);
  }
  Serial.printf("Test progress: %5.2f%% , successCount: %ld , failCount:%ld \n", progress, successCount, failCount);
}