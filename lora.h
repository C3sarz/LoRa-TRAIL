#include <Arduino.h>


// Define LoRa parameters
#define RF_FREQUENCY 915000000   // Hz
#define TX_OUTPUT_POWER 22       // dBm
#define LORA_BANDWIDTH 0         // [0: 125 kHz, 1: 250 kHz, 2: 500 kHz, 3: Reserved]
#define LORA_SPREADING_FACTOR 7  // [SF7..SF12]
#define LORA_CODINGRATE 1        // [1: 4/5, 2: 4/6,  3: 4/7,  4: 4/8]
#define LORA_PREAMBLE_LENGTH 8   // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT 0    // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false
#define RX_TIMEOUT_VALUE 0
#define TX_TIMEOUT_VALUE 7000

struct RX_HANDLERS{
  void (*RxDone)(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
  void (*RxTimeout)();
  void (*TxDone)();
};

// Forward function declarations
void setupRF();
void OnTxDone(void);
void OnTxTimeout(void);
void send(byte* data, byte dataLen);
void OnCadDone(bool cadResult);
void onRxDone(uint8_t * payload, uint16_t size, int16_t rssi, int8_t snr);
void onRxTimeout();
void onRxError();
void tryRx(uint16_t timeout);
void setupRx(RX_HANDLERS* handlers);