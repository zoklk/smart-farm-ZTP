# Edge Layer Guide

## 1. 목적

- 대상: Raspberry Pi 기반 Edge 노드
- 역할: EdgeCore 연결, 로컬 MQTT 브로커 운영, 알림 로그 서버 운영

## 2. 디렉터리 구성

| 경로 | 설명 |
|---|---|
| `edge-setup.sh` | Edge 노드 초기 설정 자동화 (Ubuntu 24.04, containerd, keadm join) |
| `mosquitto.conf` | 개발/테스트용 MQTT Broker 설정 |
| `alert_server/` | Flask 기반 알림 로그 수집/표시 서버 |
| `sys-backup/` | netplan, edgecore, CNI 백업 파일 |
| `mk-docker-opts.sh` | flannel 기반 Docker 옵션 생성 스크립트(레거시 참고) |

## 3. 사전 조건

- OS: Ubuntu 24.04 LTS (ARM64)
- CloudCore 접근 가능 상태
- CloudCore 토큰 확보
- `sudo` 권한 계정

토큰 생성 예시(Cloud):

```bash
kubectl create token cloudcore -n kubeedge --duration=87600h
```

## 4. Edge 초기 설정

실행 스크립트: `edge-setup.sh`

작업 항목:

1. OS 버전 확인
2. 고정 IP(netplan) 적용
3. Cloud 네트워크 연결 점검
4. 패키지 업데이트
5. containerd 설치/설정
6. keadm 설치 및 `keadm join`
7. edgecore 서비스 상태 점검

실행:

```bash
cd edge
chmod +x edge-setup.sh
./edge-setup.sh <NODE_NUM> <CLOUDCORE_TOKEN>
```

예시:

```bash
./edge-setup.sh 1 eyJhbGciOi...
```

기본값:

- `NODE_NUM=1` -> `NODE_IP=192.168.0.101`
- `CLOUDCORE_IP=192.168.0.2`
- `CLOUDCORE_PORT=10000`

환경 대역이 다르면 `edge-setup.sh` 상단 변수 직접 수정.

## 5. MQTT Broker 구성

`mosquitto.conf` 특성:

- `listener 1883`
- `allow_anonymous true` (개발/테스트)
- persistence 활성화

적용:

```bash
sudo apt-get update
sudo apt-get install -y mosquitto mosquitto-clients
sudo cp mosquitto.conf /etc/mosquitto/conf.d/smart-farm.conf
sudo systemctl restart mosquitto
sudo systemctl enable mosquitto
```

확인:

```bash
sudo systemctl status mosquitto
ss -lntp | grep 1883
```

## 6. Alert Server 실행

위치: `alert_server/`

```bash
cd alert_server
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
python3 alert_server.py
```

확인 URL:

- `http://<edge-ip>:8080/`
- `http://<edge-ip>:8080/health`

## 7. 운영 점검

Edge 노드:

```bash
sudo systemctl status edgecore
sudo journalctl -u edgecore -n 100 --no-pager
```

Cloud 노드:

```bash
kubectl get nodes -o wide
kubectl get pods -n kubeedge -o wide
```

## 8. 주의 사항

- `sys-backup/`는 복구/비교 목적 파일
- 백업 파일 직접 재적용 전 IP/호스트명 차이 확인
- 토큰/인증정보/실환경 IP의 저장소 직접 노출 금지
