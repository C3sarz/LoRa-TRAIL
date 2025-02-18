#include "storage_manager.h"

void mqttSetup();
void sendFileMqtt(FileStructure file);
void mqttKeepAlive();
void onMqttMessage(int messageSize);