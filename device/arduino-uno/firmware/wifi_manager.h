#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFiEsp.h>
#include <SoftwareSerial.h>
#include "config.h"
#include "globals.h"

// WiFi 연결
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

#endif
