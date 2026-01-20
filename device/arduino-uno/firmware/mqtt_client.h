#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFiEsp.h>
#include "config.h"
#include "globals.h"
#include "wifi_manager.h"

// MQTT 메시지 콜백
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  static unsigned long lastProcessTime = 0;
  unsigned long now = millis();
  
  if (now - lastProcessTime < 100) {
    Serial.println(F("[MQTT] Too fast, ignoring"));
    return;
  }
  lastProcessTime = now;

  if (strcmp(topic, configTopic) == 0) {
    StaticJsonDocument<64> doc;
    DeserializationError error = deserializeJson(doc, payload, length);

    if (error) {
      Serial.println(F("[MQTT] JSON parse error"));
      return;
    }

    if (doc.containsKey("status")) {
      const char* status = doc["status"];
      
      if (strcmp(status, "registered") == 0) {
        Serial.println(F("[Status] DISCOVERING -> REGISTERED"));
        deviceState = REGISTERED;
        lastSensorDataTime = millis() - SENSOR_DATA_INTERVAL;
      }
    }
  }
}

// MQTT 연결
void connectMQTT() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  const char* broker = MQTT_BROKERS[ZONE_ID - 1];

  char clientId[25];
  sprintf(clientId, "%s-%04X", deviceId, random(0xFFFF));

  mqttClient.setServer(broker, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);

  if (mqttClient.connect(clientId)) {
    mqttClient.subscribe(configTopic, 1);
    delay(500);
    mqttClient.loop();
    Serial.println(F("[MQTT] Ready"));
    
  } else {
    Serial.print(F("[MQTT] Failed, rc="));
    Serial.println(mqttClient.state());
    delay(2000);
  }
}

// Discovery 메시지 발송
void sendDiscovery() {
  StaticJsonDocument<100> doc;
  doc["Id"] = deviceId;
  doc["zone"] = ZONE_ID;
  doc["type"] = "UNO-FARM-V1";
  
  char buffer[100];
  size_t len = serializeJson(doc, buffer);
  
  char discoveryTopic[30];
  sprintf(discoveryTopic, "edge/zone%d/discovery", ZONE_ID);

  delay(100);
  bool result = mqttClient.publish(discoveryTopic, buffer, len);
  delay(100);
  
  Serial.println(result ? F("[Discovery] ✓ OK") : F("[Discovery] ✗ FAILED"));
}

#endif
