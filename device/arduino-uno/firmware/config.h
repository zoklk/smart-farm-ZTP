#ifndef CONFIG_H
#define CONFIG_H

// WiFi 설정
#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PASSWORD "your-password"
#define ZONE_ID 1

// 핀 설정
#define DHT_PIN 4
#define DHT_TYPE DHT11
#define LIGHT_PIN A0
#define SOIL_PIN A1

// MQTT 설정
const char* MQTT_BROKERS[] = {
  "192.168.0.101",
  "192.168.0.102",
  "192.168.0.103"
};
const int MQTT_PORT = 1883;

// 타이밍 설정
const unsigned long DISCOVERY_INTERVAL = 10000;  // 10초
const unsigned long SENSOR_DATA_INTERVAL = 5000; // 5초

// 디바이스 상태
enum DeviceState {
  DISCOVERING,
  REGISTERED
};

#endif
