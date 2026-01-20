# Arduino UNO Smart Farm Sensor Firmware

## 1. 파일 구조
```
firmware/
├── main.ino           # 메인 프로그램 (setup, loop)
├── config.h           # WiFi, MQTT, 핀 설정
├── globals.h          # 전역 변수 선언
├── device_id.h        # MAC 기반 Device ID 생성
├── wifi_manager.h     # WiFi 연결 관리
├── mqtt_client.h      # MQTT 연결 및 메시지 처리
├── sensor_reader.h    # 센서 데이터 수집 및 전송
└── README.md
```


## 2. 필요 라이브러리
Arduino IDE의 라이브러리 매니저에서 설치:
1. **WiFiEsp** (by bportaluri)
2. **PubSubClient** (by Nick O'Leary)
3. **ArduinoJson** (by Benoit Blanchon) - v6.x
4. **DHT sensor library** (by Adafruit)
5. **Adafruit Unified Sensor** (DHT 라이브러리 의존성)


## 3. 하드웨어 연결
### ESP-01 (WiFi Module)
- TX → Arduino Pin 2 (SoftwareSerial RX)
- RX → Arduino Pin 3 (SoftwareSerial TX)
- VCC → Arduino 3.3V
- GND → Arduino GND

### DHT-11 (온습도 센서)
- VCC → Arduino 5V
- DATA → Arduino Pin 4
- GND → Arduino GND

### GM5537-1 (조도 센서)
- VCC → Arduino 5V
- AOUT → Arduino A0
- GND → Arduino GND

### SZH-EK106 (토양습도 센서)
- VCC → Arduino 5V
- AOUT → Arduino A1
- GND → Arduino GND


## 4. 업로드 방법
### Arduino IDE
1. Arduino IDE 실행
2. `File` → `Open` → `main.ino` 선택
3. 같은 폴더의 모든 .h 파일이 자동으로 탭에 로드됨
4. `Tools` → `Board` → `Arduino UNO` 선택
5. `Tools` → `Port` → 연결된 포트 선택
6. Upload 버튼 클릭 (→)


## 5. 설정 변경
`config.h` 파일에서 다음 항목 수정 가능:

```cpp
#define WIFI_SSID "your-wifi-ssid"      // WiFi 이름
#define WIFI_PASSWORD "your-password"    // WiFi 비밀번호
#define ZONE_ID 1                        // Zone 번호 (1, 2, 3)
```


## 6. 동작 원리
1. **부팅** → WiFi 연결 → MAC 주소 기반 Device ID 생성
2. **DISCOVERING 모드** → 10초마다 Discovery 메시지 발송
3. **Pi에서 Registration 수신** → REGISTERED 모드 전환
4. **REGISTERED 모드** → 5초마다 센서 데이터 전송


## 7. 트러블슈팅
### ESP-01이 인식 안 됨
- 전원 공급 확인 (9V 배터리 → 3.3V 레귤레이터)
- RX/TX 연결 확인 (교차 연결)

### DHT-11 읽기 실패
- VCC가 5V에 연결되었는지 확인
- 데이터 핀이 Pin 4에 연결되었는지 확인

### MQTT 연결 실패
- WiFi 연결 상태 확인
- MQTT Broker IP 주소 확인 (config.h)
- Zone ID가 올바른지 확인

### archive의 raw code
- firmware의 코드가 원활히 동작하지 않을 경우 참고용
