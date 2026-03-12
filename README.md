# Smart Farm IoT System with Zero-Touch Provisioning

## 프로젝트 개요

- 목표: KubeEdge 기반 3계층 스마트팜 IoT 운영 자동화
- 데이터 흐름: Arduino 센서 -> Raspberry Pi Edge -> Kubernetes Cloud
- 핵심 주제: ZTP(Zero-Touch Provisioning), Edge 데이터 검증, 중앙 모니터링
- 작성: 20215105 변진철, 20215140 윤진현
- 개발 보조 도구: Claude
- 문서 작성 보조 도구: Claude, ChatGPT

---

## 아키텍처

<img src="docs/images/architecture.png" width="750">

| 계층 | 구성 | 역할 |
|---|---|---|
| Cloud Layer | K8s Cluster (KubeEdge CloudCore, InfluxDB, Grafana) | 중앙 모니터링, 집계 데이터 저장/시각화 |
| Edge Layer | Raspberry Pi x 3 (device-registration, data-validator) | 디바이스 등록, 데이터 집계/검증/전송 |
| Device Layer | Arduino UNO + ESP-01 x 7 (DHT11, SZH-EK106, CDS) | 센서 데이터 수집, MQTT 발행 |

---

## 시스템 워크플로우

<img src="docs/images/system_workflow.png" width="750">

- 단계 1(빨간 박스): ZTP 등록 파이프라인
- 단계 2(파란 박스): 정상 운영 데이터 파이프라인

---

## 핵심 기능

### 1) Zero-Touch Provisioning (ZTP)

- 센서 교체 시 수동 등록 절차 최소화
- 전원 인가 후 자동 Discovery/Registration 흐름

<img src="docs/images/ZTP_scenario.png" width="500">

```text
Arduino 부팅
  -> ESP-01 MAC 기반 Device ID 생성
  -> Discovery 토픽 발행 (edge/zone{N}/discovery)
  -> Edge device-registration 감지
  -> Cloud K8s API로 Device CRD 생성
  -> Registration 응답 전송 (edge/zone{N}/status/{deviceId})
  -> 센서 데이터 전송 시작 (edge/zone{N}/data/{deviceId})
```

**ZTP 로그 - Arduino**

<img src="docs/images/ZTP_log_arduino.png" width="750">

**ZTP 로그 - Pi**

<img src="docs/images/ZTP_log_pi.png" width="750">

### 2) Edge Automation (데이터 파이프라인/자동 대응)

- 5초 주기 Raw 데이터 수집
- 1분 단위 집계(12개 샘플 평균)
- 센서 유효성 검증 및 임계치 기반 제어 로그 생성
- 집계 데이터 InfluxDB 전송

<img src="docs/images/data_pipeline.png" width="500">

```text
Arduino -> 5초 주기 Raw data
  -> Pi 버퍼 12개 누적(1분)
  -> 평균값 산출(노이즈 완화, 12:1 압축)
  -> 범위 검증(temperature/humidity/light/soil)
  -> 임계치 이탈 시 Flask Control Log 기록
  -> Cloud InfluxDB 저장
```

> Arduino UNO 저장 공간과 ESP-01 대역폭 제약으로 직접 제어 대신 Flask 기반 Control Log 방식 사용

**Control Log (Pi -> Arduino 명령)**

<img src="docs/images/control_log.png" width="750">

### 3) Central Monitoring (Grafana)

- Zone별 활성 디바이스 현황
- 온도/습도 Time Series 시각화
- InfluxDB 1분 집계값(avg/min/max) 대시보드 연계

**Grafana Central Dashboard**

<img src="docs/images/grafana_dashboard.png" width="750">

**InfluxDB Data Explorer**

<img src="docs/images/influxdb.png" width="750">

---

## 하드웨어 구성

**Device Layer — Arduino UNO + ESP-01**

<img src="docs/images/hardware_arduino.png" width="600">

**Edge Layer — Raspberry Pi x 3**

<img src="docs/images/hardware_pi.png" width="600">

---

## MQTT 토픽 구조

| 토픽 | 방향 | 용도 |
|---|---|---|
| `edge/zone{N}/discovery` | Arduino -> Pi | 디바이스 Discovery |
| `edge/zone{N}/status/{deviceId}` | Pi -> Arduino | Registration 응답 |
| `edge/zone{N}/data/{deviceId}` | Arduino -> Pi | 센서 데이터 전송 |

- 예시: Zone 1, `E0980613D2FC`
- Discovery: `edge/zone1/discovery`
- Status: `edge/zone1/status/E0980613D2FC`
- Data: `edge/zone1/data/E0980613D2FC`

---

## 레포지토리 구조

```text
smart-farm-ZTP/
├── device/
│   └── arduino-uno/             # Arduino 펌웨어 (WiFi, MQTT, 센서 드라이버)
├── edge/
│   ├── alert_server/            # Flask 기반 Control Log 서버
│   ├── sys-backup/              # Pi 네트워크/EdgeCore/MQTT 백업 설정
│   ├── edge-setup.sh            # Pi 초기 세팅 스크립트
│   └── mosquitto.conf           # Mosquitto 설정
└── cloud/
    ├── cloudcore/               # CloudCore 매니페스트/배포 스크립트
    │   ├── kubeedge/            # CloudCore, Device CRD/Registration
    │   └── monitoring/          # InfluxDB, Grafana, Prometheus, AlertManager
    ├── edge-manifests/
    │   ├── registeration/       # ZTP device-registration 매니페스트
    │   └── data-validator/      # 집계/검증/Influx 전송 매니페스트
    └── helm/                    # Cilium, Rook-Ceph 차트
```

- `registeration`, `data-validator` 위치: KubeEdge 워크로드 관리 구조 반영

레이어별 상세 문서:
- [Device Layer](device/arduino-uno/README.md)
- [Edge Layer](edge/README.md)
- [Cloud Layer](cloud/README.md)

---

## 기술 스택

| 영역 | 기술 |
|---|---|
| Device | Arduino UNO, ESP-01(ESP8266), DHT-11, GM5537-1, SZH-EK106 |
| Edge | Raspberry Pi 4, Python 3.12, Mosquitto MQTT, Flask, KubeEdge EdgeCore |
| Cloud | Kubernetes, KubeEdge CloudCore, InfluxDB 2.7, Grafana 10.x |
| 통신 | MQTT(QoS 1), HTTP(REST) |

---

## 알려진 한계

- KubeEdge DeviceTwin/EventBus 일부 기능 미활용(CNI 이슈)
- Device CRD 생성 중심, Status 동기화 로직 미완료
- Arduino UNO/ESP-01 제약으로 직접 제어 대신 Control Log 방식 채택
