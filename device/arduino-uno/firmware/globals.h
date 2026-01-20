#ifndef GLOBALS_H
#define GLOBALS_H

#include <WiFiEsp.h>
#include <WiFiEspClient.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>
#include <DHT.h>
#include "config.h"

// 하드웨어 객체 (main.ino에서 정의)
extern SoftwareSerial espSerial;
extern WiFiEspClient espClient;
extern PubSubClient mqttClient;
extern DHT dht;

// 디바이스 상태 변수
extern DeviceState deviceState;
extern unsigned long lastDiscoveryTime;
extern unsigned long lastSensorDataTime;
extern unsigned int seqNumber;

// 디바이스 정보
extern char deviceId[20];
extern char configTopic[50];
extern char dataTopic[50];

#endif
