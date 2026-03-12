# KubeEdge CloudCore Deployment Guide

## 1. 범위

- 대상 디렉터리: `cloud/cloudcore/`
- 목적: CloudCore + Monitoring 스택 + Device CRD/Registration 배포
- 기준 버전:
  - KubeEdge CloudCore `v1.15.0`
  - InfluxDB `v2.7`
  - Grafana `v10.x`
  - Prometheus `v2.45`
  - AlertManager `v0.26`

## 2. 아키텍처 요약

```text
Cloud Kubernetes Cluster (192.168.0.2)
├── Namespace: kubeedge
│   ├── CloudCore (:10000)
│   ├── Device Registration
│   └── Device CRDs
└── Namespace: monitoring
    ├── InfluxDB (:8086)
    ├── Prometheus (:9090 / NodePort 30090)
    ├── AlertManager (:9093 / NodePort 30093)
    └── Grafana (:3000 / NodePort 30300)
```

## 3. 파일 구조

| 경로 | 설명 |
|---|---|
| `deploy-cloud.sh` | 모니터링 + Device CRD/Registration 자동 배포 스크립트 |
| `kubeedge/00-namespace.yaml` | `kubeedge`, `monitoring` 네임스페이스 |
| `kubeedge/01-cloudcore.yaml` | CloudCore 기본 매니페스트 |
| `kubeedge/01-cloudcore-fixed.yaml` | CloudCore selector 보정 버전 |
| `kubeedge/06-device-registration.yaml` | Device Registration 배포 |
| `kubeedge/07-device-crd.yaml` | DeviceModel/Device CRD |
| `monitoring/02-influxdb.yaml` | InfluxDB |
| `monitoring/03-grafana.yaml` | Grafana |
| `monitoring/04-prometheus.yaml` | Prometheus |
| `monitoring/05-alertmanager.yaml` | AlertManager |

## 4. 사전 조건

### 4.1 클러스터

- Kubernetes 1.28+
- `kubectl` 접근 가능 상태
- Cloud 노드 IP 예시: `192.168.0.2`

### 4.2 스토리지

hostPath 경로 준비:

```bash
sudo mkdir -p /data/kubeedge/cloudcore
sudo mkdir -p /data/influxdb
sudo mkdir -p /data/grafana
sudo mkdir -p /data/prometheus
sudo mkdir -p /data/alertmanager
sudo chmod 755 /data/*
```

필요 용량(권장): 약 82Gi

### 4.3 KubeEdge CRD

```bash
kubectl apply -f https://raw.githubusercontent.com/kubeedge/kubeedge/release-1.15/build/crds/devices/devices_v1alpha2_device.yaml
kubectl apply -f https://raw.githubusercontent.com/kubeedge/kubeedge/release-1.15/build/crds/devices/devices_v1alpha2_devicemodel.yaml
kubectl apply -f https://raw.githubusercontent.com/kubeedge/kubeedge/release-1.15/build/crds/reliablesyncs/cluster_objectsync_v1alpha1.yaml
kubectl apply -f https://raw.githubusercontent.com/kubeedge/kubeedge/release-1.15/build/crds/reliablesyncs/objectsync_v1alpha1.yaml
```

### 4.4 네트워크

- CloudCore WebSocket: `:10000`
- Edge 외부 경유 환경 예시: `172.17.90.89:10000 -> 192.168.0.2:10000`

## 5. 배포 절차

### 5.1 자동 배포 (CloudCore 기설치 환경)

```bash
cd cloud/cloudcore
sudo ./deploy-cloud.sh
```

스크립트 수행 항목:

1. CloudCore 존재 확인
2. PV 디렉터리 생성
3. InfluxDB/Grafana/Prometheus/AlertManager 배포
4. Device CRD/Registration 배포

참고:

- 현재 `deploy-cloud.sh`는 상대경로 기준 매니페스트 호출 구조
- 실행 경로/파일 경로 불일치 시 스크립트 내부 `kubectl apply -f` 경로 보정 필요

### 5.2 수동 배포

```bash
cd cloud/cloudcore

# Namespace
kubectl apply -f kubeedge/00-namespace.yaml

# CloudCore (신규 구축 시)
kubectl apply -f kubeedge/01-cloudcore-fixed.yaml
kubectl wait --for=condition=ready pod -l app=cloudcore -n kubeedge --timeout=300s

# Monitoring
kubectl apply -f monitoring/02-influxdb.yaml
kubectl apply -f monitoring/03-grafana.yaml
kubectl apply -f monitoring/04-prometheus.yaml
kubectl apply -f monitoring/05-alertmanager.yaml

# Device CRD + Registration
kubectl apply -f kubeedge/07-device-crd.yaml
kubectl apply -f kubeedge/06-device-registration.yaml
```

## 6. 검증

### 6.1 Pod 상태

```bash
kubectl get pods -n kubeedge -o wide
kubectl get pods -n monitoring -o wide
```

### 6.2 Service

```bash
kubectl get svc -n kubeedge
kubectl get svc -n monitoring
```

### 6.3 PV/PVC

```bash
kubectl get pv
kubectl get pvc -A
```

기대 상태: PVC `Bound`

### 6.4 CloudCore 로그

```bash
kubectl logs -n kubeedge -l app=cloudcore --tail=50
```

## 7. 접속 정보

- Grafana: `http://192.168.0.2:30300` (`admin/admin`)
- Prometheus: `http://192.168.0.2:30090`
- AlertManager: `http://192.168.0.2:30093`
- InfluxDB: `http://192.168.0.2:8086` (내부 접근 중심)

## 8. Edge 연결 준비

토큰 발급:

```bash
keadm gettoken --kube-config=/root/.kube/config
# 또는
kubectl create token cloudcore -n kubeedge --duration=87600h
```

포트포워딩 예시:

```text
External: 172.17.90.89:10000
Internal: 192.168.0.2:10000
Protocol: TCP
```

## 9. 트러블슈팅

### 9.1 PVC Pending

- 확인: `kubectl get pv`, `kubectl get pvc -A`
- 조치: `/data/*` 경로/권한 점검, PV 재적용

### 9.2 CloudCore Pod 비정상

- 원인 후보: CRD 미설치, 인증서/hostPath 문제
- 조치: CRD 재적용, `/etc/kubeedge/{ca,certs}` 점검

### 9.3 Device Registration MQTT 연결 실패

- 원인 후보: Edge MQTT 미구성
- 조치: Edge MQTT 구성 후 재확인
- 로그: `kubectl logs -n kubeedge -l app=device-registration`

### 9.4 InfluxDB 초기화 실패

```bash
sudo chown -R 1000:1000 /data/influxdb
kubectl delete pod -n kubeedge -l app=influxdb
```

### 9.5 Grafana 데이터소스 실패

```bash
kubectl get svc -n kubeedge influxdb
kubectl get svc -n monitoring prometheus
kubectl rollout restart deployment grafana -n monitoring
```

## 10. 참고 링크

- KubeEdge: https://kubeedge.io/docs/
- InfluxDB: https://docs.influxdata.com/influxdb/
- Grafana: https://grafana.com/docs/
- Prometheus: https://prometheus.io/docs/
