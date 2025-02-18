#include "send_mqtt.h"
#include <SPI.h>
#include <RAK13800_W5100S.h>    // Click to install library: http://librarymanager/All#RAKwireless_W5100S
#include <ArduinoMqttClient.h>  // Click to install library: http://librarymanager/All#ArduinoMqttClient
#include "storage_manager.h"

#define USER_NAME ""  // Provide a username and password for authentication.
#define PASSWORD ""
#define SERVER_PORT 1883  //  Define the server port.

IPAddress ip(192, 168, 0, 88);       // Set IP address,dependent on your local network.
IPAddress server(192, 168, 0, 242);  // Set the server IP address.
EthernetClient client;
MqttClient mqttClient(client);
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };  // Set the MAC address, do not repeat in a network.

int port = 61612;
const char topic[] = "server/file";

void mqttSetup() {
  pinMode(WB_IO2, OUTPUT);
  digitalWrite(WB_IO2, HIGH);  // Enable power supply.

  pinMode(WB_IO3, OUTPUT);
  digitalWrite(WB_IO3, LOW);  // Reset Time.
  delay(100);
  digitalWrite(WB_IO3, HIGH);  // Reset Time.

  // Open serial communications and wait for port to open:
  time_t timeout = millis();
  Serial.begin(115200);
  while (!Serial) {
    if ((millis() - timeout) < 5000) {
      delay(100);
    } else {
      break;
    }
  }
  Serial.println("RAK13800 Ethernet MQTT Publish example.");

  Ethernet.init(SS);        // Set CS pin.
  Ethernet.begin(mac, ip);  // Start the Ethernet connection:

  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    while (true) {
      delay(1);
    }
  }
  while (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
    delay(500);
  }

  delay(1000);  // Give the Ethernet shield a second to initialize:

  mqttClient.setId("SENDER");
  mqttClient.setUsernamePassword(USER_NAME, PASSWORD);

  Serial.print("Attempting to connect to the MQTT broker...");

  if (!mqttClient.connect(server, SERVER_PORT)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    delay(100);
    // while (true)
    // {
    //   delay(1);
    // }
  }

  Serial.println("Connected to the MQTT broker!");
  Serial.println();
}

void mqttKeepAlive() {
  mqttClient.poll();
}


void sendFileMqtt(FileStructure file) {



  for (uint i = 0; i < file.header.sectorCount; i++) {

    // double dataSize_D = static_cast<double>(file.header.dataSize);
    // uint16_t remainingBytes = static_cast<uint16_t>(dataSize_D - dataSize_D * (i / (double)file.header.sectorCount));
    mqttClient.beginMessage(topic);
    Serial.printf("Sending data to server. DataLen: %u\r\n",file.header.dataSize - i*DATA_SECTOR_SIZE);
    mqttClient.write((uint8_t*)(file.data[i]), DATA_SECTOR_SIZE);
    mqttClient.endMessage();
  }

  // mqttClient
}

// void loop()
// {
//   // Call poll() regularly to allow the library to send MQTT keep alives which.
//   // Avoids being disconnected by the broker.
//   mqttClient.poll();

//   unsigned long currentMillis = millis();   // Avoid having delays in loop, we'll use the strategy from BlinkWithoutDelay.

//   if (currentMillis - previousMillis >= interval)
//   {
//     // Save the last time a message was sent.
//     previousMillis = currentMillis;

//     Serial.print("Sending message to topic: ");
//     Serial.println(topic);
//     Serial.print("hello ");
//     Serial.println(count);

//     // Send message, the Print interface can be used to set the message contents.
//     mqttClient.beginMessage(topic);
//     mqttClient.print("hello ");
//     mqttClient.print(count);  //  Can be changed to the sensor data if you want.
//     mqttClient.endMessage();

//     Serial.println();
//     count++;
//   }
// }
