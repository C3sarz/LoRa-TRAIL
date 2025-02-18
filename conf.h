
#ifndef CONF
#define CONF
// #define SERVER
#define DEVICE_ID 0x01;

enum DATA_TRANSFER_STATE{
  TRANSFER_REQUESTED_HANDSHAKE,
  TRANSFER_RESPONDED_HANDSHAKE,
  TRANSFER_LISTEN,
  TRANSFER_BUSY,
  TRANSFER_REQUEST_DATA,
  TRANSFER_SENDING_DATA,
  TRANSFER_WAIT,
  TRANSFER_IDLE,
  TRANSFER_COMPLETE,
};
#endif