# Arduino UNO Smart Farm Sensor Firmware

## 1. 목적

- 대상: Arduino UNO + ESP-01 기반 센서 노드 펌웨어
- 역할: WiFi/MQTT 연결, 디바이스 식별, 센서 데이터 발행

## 2. 파일 구조

```text
firmware/
├── main.ino           # 메인 루프(setup/loop)
├── config.h           # WiFi, MQTT, 핀, Zone 설정
├── globals.h          # 전역 변수
├── device_id.h        # MAC 기반 Device ID 생성
├── wifi_manager.h     # WiFi 연결 관리
├── mqtt_client.h      # MQTT 연결/토픽 처리
├── sensor_reader.h    # 센서 읽기/전송
└── README.md
```

## 3. 필요 라이브러리

Arduino IDE Library Manager 설치 목록:

1. WiFiEsp (by bportaluri)
2. PubSubClient (by Nick O'Leary)
3. ArduinoJson v6.x (by Benoit Blanchon)
4. DHT sensor library (by Adafruit)
5. Adafruit Unified Sensor

## 4. 하드웨어 연결

### ESP-01 (WiFi)

- TX -> Arduino Pin 2 (SoftwareSerial RX)
- RX -> Arduino Pin 3 (SoftwareSerial TX)
- VCC -> Arduino 3.3V
- GND -> Arduino GND

### DHT-11 (온습도)

- VCC -> Arduino 5V
- DATA -> Arduino Pin 4
- GND -> Arduino GND

### GM5537-1 (조도)

- VCC -> Arduino 5V
- AOUT -> Arduino A0
- GND -> Arduino GND

### SZH-EK106 (토양습도)

- VCC -> Arduino 5V
- AOUT -> Arduino A1
- GND -> Arduino GND

## 5. 업로드 절차

1. Arduino IDE 실행
2. `File -> Open -> main.ino` 선택
3. 같은 폴더 `.h` 파일 자동 로드 확인
4. `Tools -> Board -> Arduino UNO` 선택
5. `Tools -> Port`에서 포트 선택
6. Upload 실행

## 6. 주요 설정

수정 파일: `firmware/config.h`

```cpp
#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PASSWORD "your-password"
#define ZONE_ID 1
```

## 7. 동작 흐름

1. 부팅 후 WiFi 연결
2. MAC 기반 Device ID 생성
3. DISCOVERING 모드에서 10초 주기 Discovery 발행
4. Registration 수신 후 REGISTERED 모드 전환
5. REGISTERED 모드에서 5초 주기 센서 데이터 전송

## 8. 트러블슈팅

### ESP-01 미인식

- 3.3V 전원 안정성 점검
- RX/TX 교차 연결 점검

### DHT-11 읽기 실패

- VCC 5V 연결 상태 점검
- DATA 핀(4번) 연결 점검

### MQTT 연결 실패

- WiFi 연결 상태 점검
- `config.h` MQTT Broker IP 점검
- `ZONE_ID` 설정 점검

### archive 코드

- `firmware/` 동작 점검용 참고 코드: `archive/PC_arduino_code.ino`
