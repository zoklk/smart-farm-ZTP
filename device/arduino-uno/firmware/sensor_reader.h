#ifndef SENSOR_READER_H
#define SENSOR_READER_H

#include <Arduino.h>
#include <DHT.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "globals.h"

// 센서 데이터 수집 및 전송
void sendSensorData() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int lightRaw = analogRead(LIGHT_PIN);
  int soilRaw = analogRead(SOIL_PIN);
  
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println(F("[Sensor] DHT read failed"));
    return;
  }
  
  // 조도: 0-1023 → 0-100%
  int light = map(lightRaw, 1023, 0, 0, 100);
  // 토양습도: 0-1023 → 0-100% (역방향)
  int soil = map(soilRaw, 1023, 0, 0, 100);
  
  StaticJsonDocument<128> doc;
  doc["id"] = deviceId;
  JsonObject d = doc.createNestedObject("d");
  d["l"] = light;
  d["t"] = temperature;
  d["h"] = humidity;
  d["s"] = soil;
  doc["sq"] = seqNumber++;

  char buffer[128];
  serializeJson(doc, buffer);

  Serial.print(F("[Sensor] "));
  mqttClient.publish(dataTopic, buffer) ? Serial.println(F("✓")) : Serial.println(F("✗"));
}

#endif
