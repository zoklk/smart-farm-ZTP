#ifndef DEVICE_ID_H
#define DEVICE_ID_H

#include <Arduino.h>
#include <SoftwareSerial.h>
#include "globals.h"

// MAC Address 조회
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

// Device ID 생성 (MAC 기반)
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

#endif
