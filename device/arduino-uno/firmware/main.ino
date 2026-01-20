// ========================================
// Arduino Smart Farm Sensor - Main
// ========================================

#include "globals.h"
#include "config.h"
#include "device_id.h"
#include "wifi_manager.h"
#include "mqtt_client.h"
#include "sensor_reader.h"

// ========================================
// 전역 변수 정의
// ========================================
SoftwareSerial espSerial(2, 3);
WiFiEspClient espClient;
PubSubClient mqttClient(espClient);
DHT dht(DHT_PIN, DHT_TYPE);

DeviceState deviceState = DISCOVERING;
unsigned long lastDiscoveryTime = 0;
unsigned long lastSensorDataTime = 0;
unsigned int seqNumber = 0;

char deviceId[20];
char configTopic[50];
char dataTopic[50];

// ========================================
// Setup
// ========================================
void setup() {
  Serial.begin(9600);
  while (!Serial);
  delay(1000);
  
  Serial.println(F("\n=== Arduino Smart Farm Sensor ===\n"));
  
  espSerial.begin(9600);
  espSerial.setTimeout(5000);  
  dht.begin();
  
  connectWiFi();
  generateDeviceId();
  
  // Device ID 검증
  if (deviceId[0] == '\0' || strlen(deviceId) != 12) {
    Serial.println(F("FATAL: Invalid Device ID. System halted."));
    while(1) {
      delay(1000);
    }
  }

  sprintf(configTopic, "edge/zone%d/status/%s", ZONE_ID, deviceId);
  sprintf(dataTopic, "edge/zone%d/data/%s", ZONE_ID, deviceId);
  
  Serial.print(F("[Device] ID: "));
  Serial.println(deviceId);
  
  connectMQTT();
  
  Serial.println(F("\n=== Setup Complete ===\n"));
}

// ========================================
// Loop
// ========================================
void loop() {
  if (!mqttClient.connected()) {
    static unsigned long lastReconnect = 0;
    if (millis() - lastReconnect > 5000) {
      lastReconnect = millis();
      Serial.println(F("[Loop] Reconnecting..."));
      connectMQTT();
    }
  } else {
    mqttClient.loop();
  }
  
  unsigned long now = millis();
  
  switch (deviceState) {
    case DISCOVERING:
      if (now - lastDiscoveryTime >= DISCOVERY_INTERVAL) {
        sendDiscovery();
        lastDiscoveryTime = now;
      }
      break;
      
    case REGISTERED:
      if (now - lastSensorDataTime >= SENSOR_DATA_INTERVAL) {
        sendSensorData();
        lastSensorDataTime = now;
      }
      break;
  }
  
  delay(10);
}
