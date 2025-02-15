#include "lora.h"
#include "conf.h"

// For working with the transceiver
#include "LoRaWan-Arduino.h"  // Click here to get the library: http://librarymanager/All#SX126x
#include "mbed.h"
#include "rtos.h"
rtos::Semaphore cadInProgress;
bool channelBusy = true;

// Interfaces
extern volatile bool rxReady;
void * onDatagram(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
void * onDataRxTimeout();
RX_HANDLERS* callbackHandler = NULL;

#ifdef DEBUG
unsigned long int cadTime = 0;
#endif

// LoRa chip variables
static RadioEvents_t RadioEvents;

void loraSetup() {

  // Initialize LoRa chip.
  lora_rak11300_init();

  // Initialize the Radio callbacks
  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  RadioEvents.RxDone = onRxDone;
  RadioEvents.RxTimeout = onRxTimeout;
  RadioEvents.RxError = onRxError;
  RadioEvents.CadDone = OnCadDone;

  // Initialize the Radio
  Radio.Init(&RadioEvents);

  // Set Radio channel (operating frequency)
  Radio.SetChannel(RF_FREQUENCY);

  // Set Radio TX configuration
  Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                    LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                    true, 0, 0, LORA_IQ_INVERSION_ON, TX_TIMEOUT_VALUE);

  // Set Radio RX configuration
  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                    true, true, 0, 0, LORA_IQ_INVERSION_ON, true);

// Ready to go
#ifdef DEBUG
  Serial.println("=================================================================");
  Serial.println("LoRa Node: Initialization completed");
  Serial.println("=================================================================");
#endif
}

void setupRx(RX_HANDLERS* handlers){
  callbackHandler = handlers;
}

void tryRx(uint16_t timeout){
  Radio.Rx(timeout);
}

/** @brief Function to be executed on Radio Tx Done event
 */
void OnTxDone(void) {
#ifdef DEBUG
  Serial.println("OnTxDone");
#endif
}

/**@brief Function to be executed on Radio Tx Timeout event
 */
void OnTxTimeout(void) {
#ifdef DEBUG
  Serial.println("OnTxTimeout");
#endif
}

void onRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr){
  if(callbackHandler != NULL){
    callbackHandler->RxDone(payload, size, rssi, snr);
  }
}

void onRxError(){
  Serial.println("RX ERROR");
}

void onRxTimeout() {
  if(callbackHandler != NULL){
    callbackHandler->RxTimeout();
  }
}

/** @brief Function to transmit a message
 */
void send(byte* data, byte dataLen) {

  // CAD to ensure channel not active. Wait until channel not busy.
  // https://news.rakwireless.com/channel-activity-detection-ensuring-your-lora-r-packets-are-sent
  // https://forum.rakwireless.com/t/how-to-calculate-the-best-cad-settings/13984/5
  channelBusy = true;  // assume channel is busy
  while (channelBusy)  // wait for channel to be not busy before sending a message
  {
#ifdef DEBUG
    cadTime = millis();  // mark the time CAD process starts
#endif
    cadInProgress.release();  // reset the in-progress flag
    Radio.Standby();          // put radio on standby
    Radio.SetCadParams(LORA_CAD_08_SYMBOL, LORA_SPREADING_FACTOR + 13, 10, LORA_CAD_RX, 0);
    unsigned long startTime = millis();
    unsigned long stopTime = random(300);
    while (millis() - startTime < stopTime)
      ;
    while (!cadInProgress.try_acquire())
      ;  // aquire CAD semaphore

// Start CAD thread
// (It does seem CAD process runs as an independent thread)
#ifdef DEBUG
    Serial.println("Starting CAD");
#endif
    Radio.StartCad();

    // Wait unti OnCadDone finishes
    while (!cadInProgress.try_acquire())
      ;
    if (channelBusy) {
#ifdef DEBUG
      Serial.println("Channel Busy");
#endif
    }
#ifdef DEBUG
    else
      Serial.println("Channel not busy");
#endif
  }

  Radio.Send(data, dataLen);
}

/**
   @brief CadDone callback: is the channel busy?
*/
void OnCadDone(bool cadResult) {
#ifdef DEBUG
  time_t duration = millis() - cadTime;
  if (cadResult)  // true = busy / Channel Activity Detected; false = not busy / channel activity not detected
  {
    Serial.printf("CAD returned channel busy after %ldms\n", duration);
  } else {
    Serial.printf("CAD returned channel free after %ldms\n", duration);
  }
#endif

  channelBusy = cadResult;

#ifdef DEBUG
  osStatus status = cadInProgress.release();
  switch (status) {
    case osOK:
      Serial.println("token has been correctly released");
      break;
    case osErrorResource:
      Serial.println("maximum token count has been reached");
      break;
    case osErrorParameter:
      Serial.println("semaphore internal error");
      break;
    default:
      Serial.println("unrecognized semaphore error");
      break;
  }
#endif
#ifndef DEBUG
  cadInProgress.release();
#endif
}
