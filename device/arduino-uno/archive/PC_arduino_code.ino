#include <WiFiEsp.h>
#include <WiFiEspClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <DHT.h>

// ========================================
// 설정
// ========================================
#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PASSWORD "your-password"
#define ZONE_ID 1

#define DHT_PIN 4
#define DHT_TYPE DHT11
#define LIGHT_PIN A0
#define SOIL_PIN A1

const char* MQTT_BROKERS[] = {
  "192.168.0.101",
  "192.168.0.102",
  "192.168.0.103"
};

const int MQTT_PORT = 1883;

// ========================================
// 하드웨어
// ========================================
SoftwareSerial espSerial(2, 3);
WiFiEspClient espClient;
PubSubClient mqttClient(espClient);
DHT dht(DHT_PIN, DHT_TYPE);

// ========================================
// 상태
// ========================================
enum DeviceState {
  DISCOVERING,
  REGISTERED
};

DeviceState deviceState = DISCOVERING;

unsigned long lastDiscoveryTime = 0;
unsigned long lastSensorDataTime = 0;
const unsigned long DISCOVERY_INTERVAL = 10000;
unsigned long sensorDataInterval = 5000;
unsigned int seqNumber = 0;

char deviceId[20];
char configTopic[50];
char dataTopic[50];


// ========================================
// WiFi 연결
// ========================================
void connectWiFi() {
  for (int i = 0; i < 3; i++) {
    WiFi.init(&espSerial);
    delay(1000);
    
    if (WiFi.status() != WL_NO_SHIELD) {
      break;
    }
    delay(2000);
  }
  
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println(F(" FAILED (ESP-01 not found)"));
    while (1);
  }
  
  int status = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int attempts = 0;
  
  while (status != WL_CONNECTED && attempts < 20) {
    delay(1000);
    status = WiFi.status();
    attempts++;
  }
  
  if (status == WL_CONNECTED) {
    Serial.print(F(" OK ("));
    Serial.print(WiFi.localIP());
    Serial.println(F(")"));
  } else {
    Serial.println(F(" FAILED"));
    delay(2000);
    connectWiFi();
  }
}

// ========================================
// MQTT 콜백
// ========================================
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
        lastSensorDataTime = millis() - sensorDataInterval;
      }
    }
  }
}

// ========================================
// MQTT 연결
// ========================================
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

// ========================================
// Discovery
// ========================================
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

// ========================================
// 센서 데이터
// ========================================
void sendSensorData() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int lightRaw = analogRead(LIGHT_PIN);
  int soilRaw = analogRead(SOIL_PIN);
  
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println(F("[Sensor] DHT read failed"));
    return;
  }
  
  // 토양습도: 0-1023 → 0-100% (역방향)
  int light = map(lightRaw, 1023, 0, 0, 100);
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

// ========================================
// Device ID 생성 (MAC Address 기반)
// ========================================
String getMacAddress() {
  String macAddr = "";
  
  // AT+CIFSR 명령으로 MAC 주소 조회
  espSerial.println(F("AT+CIFSR"));
  delay(500);
  
  // 응답 읽기 (최대 3초 대기)
  unsigned long startTime = millis();
  String response = "";
  
  while (millis() - startTime < 3000) {
    while (espSerial.available()) {
      char c = espSerial.read();
      response += c;
      
      // STAMAC 라인 찾기
      if (response.indexOf(F("STAMAC")) != -1) {
        int startIdx = response.indexOf('"', response.indexOf(F("STAMAC")));
        int endIdx = response.indexOf('"', startIdx + 1);
        
        if (startIdx != -1 && endIdx != -1) {
          String rawMac = response.substring(startIdx + 1, endIdx);

          for (unsigned int i = 0; i < rawMac.length(); i++) {
            char c = rawMac.charAt(i);
            if (c != ':') {
              macAddr += (char)toupper(c);
            }
          }
          return macAddr;
        }
      }
    }
  }
  
  return "";
}

void generateDeviceId() {
  Serial.println(F("=== Generating Device ID from MAC Address ==="));
  
  String macAddr = getMacAddress();
  
  if (macAddr.length() == 12) {
    macAddr.toCharArray(deviceId, sizeof(deviceId));
    Serial.print(F("Device ID (MAC-based): "));
    Serial.println(deviceId);
  } else {
    Serial.println(F("ERROR: Failed to get MAC address."));
    Serial.println(F("Please check ESP-01 connection and try again."));
    deviceId[0] = '\0';
  }
}


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
  
  generateDeviceId();  // MAC기반 ID 생성
  // Device ID 검증
  if (deviceId[0] == '\0' || strlen(deviceId) != 12) {
    Serial.println(F("FATAL: Invalid Device ID. System halted."));
    while(1) {
      delay(1000); // 시스템 정지
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
      if (now - lastSensorDataTime >= sensorDataInterval) {
        sendSensorData();
        lastSensorDataTime = now;
      }
      break;
  }
  
  delay(10);
}