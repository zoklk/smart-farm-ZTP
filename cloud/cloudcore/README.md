# KubeEdge IoT Cloud Infrastructure Deployment Guide

## 📋 개요

이 디렉토리는 KubeEdge IoT 인프라의 Cloud 계층을 배포하기 위한 Kubernetes 매니페스트와 스크립트를 포함합니다. 이 디렉토리는 상세 계획에 대한 내용으로 구성되어, 실제 배포환경에 맞추어 수정이 필요합니다.

**배포 대상:**
- KubeEdge CloudCore (v1.15.0)
- InfluxDB (v2.7) - 시계열 데이터베이스
- Grafana (v10.2) - 시각화
- Prometheus (v2.45) - 메트릭 수집
- AlertManager (v0.26) - 알림 관리
- Device Registration Service - MQTT 기반 Device 자동 등록

---

## 🏗️ 아키텍처

```
Cloud Kubernetes Cluster (192.168.0.2)
├── Namespace: kubeedge
│   ├── CloudCore (WebSocket :10000)
│   ├── Device Registration Service
│   └── Device CRDs
└── Namespace: monitoring
    ├── InfluxDB (:8086)
    ├── Prometheus (:9090, NodePort :30090)
    ├── AlertManager (:9093, NodePort :30093)
    └── Grafana (:3000, NodePort :30300)
```

---

## 📦 파일 목록

| 파일 | 설명 | 컴포넌트 |
|------|------|----------|
| `00-namespace.yaml` | Namespace 정의 | kubeedge, monitoring |
| `01-cloudcore.yaml` | KubeEdge CloudCore | CloudCore + PV/PVC |
| `02-influxdb.yaml` | InfluxDB StatefulSet | InfluxDB + PV/PVC |
| `03-grafana.yaml` | Grafana Deployment | Grafana + PV/PVC + Datasources |
| `04-prometheus.yaml` | Prometheus StatefulSet | Prometheus + PV/PVC + Config |
| `05-alertmanager.yaml` | AlertManager Deployment | AlertManager + PV/PVC + Config |
| `06-device-registration.yaml` | Device Registration Service | Python MQTT Consumer |
| `07-device-crd.yaml` | Device CRD 정의 | DeviceModel, Device |
| `deploy-cloud.sh` | 자동 배포 스크립트 | 전체 배포 자동화 |
| `README.md` | 이 파일 | 배포 가이드 |

---

## 🚀 사전 요구사항

### 1. Kubernetes Cluster
- **버전**: 1.28+
- **노드**: Single-node (192.168.0.2)
- **접근**: kubectl 설정 완료

### 2. Storage
- **타입**: hostPath (수동 PV 할당)
- **경로**: `/data/*` 디렉토리 사용
- **필요 용량**:
  - CloudCore: 5Gi
  - InfluxDB: 50Gi
  - Grafana: 5Gi
  - Prometheus: 20Gi
  - AlertManager: 2Gi
  - **총합**: ~82Gi

### 3. KubeEdge CRDs
KubeEdge CRD가 미리 설치되어 있어야 합니다:

```bash
# Device CRDs 설치
kubectl apply -f https://raw.githubusercontent.com/kubeedge/kubeedge/release-1.15/build/crds/devices/devices_v1alpha2_device.yaml
kubectl apply -f https://raw.githubusercontent.com/kubeedge/kubeedge/release-1.15/build/crds/devices/devices_v1alpha2_devicemodel.yaml

# ReliableSync CRDs 설치 (선택적)
kubectl apply -f https://raw.githubusercontent.com/kubeedge/kubeedge/release-1.15/build/crds/reliablesyncs/cluster_objectsync_v1alpha1.yaml
kubectl apply -f https://raw.githubusercontent.com/kubeedge/kubeedge/release-1.15/build/crds/reliablesyncs/objectsync_v1alpha1.yaml
```

### 4. 네트워크
- **Cloud IP**: 192.168.0.2
- **CloudCore WebSocket**: :10000
- **Port-forwarding**: Router1 172.17.90.89:10000 → 192.168.0.2:10000 (Edge 연결용)

---

## 📝 배포 방법

### 방법 1: 자동 배포 (추천)

```bash
# 1. 디렉토리 이동
cd /path/to/cloud-manifests

# 2. 배포 스크립트 실행
sudo ./deploy-cloud.sh
```

**스크립트 동작:**
1. PV 디렉토리 자동 생성 (`/data/*`)
2. Namespace 생성
3. 모든 컴포넌트 순차 배포
4. Pod 상태 대기 및 확인
5. 최종 상태 리포트

### 방법 2: 수동 배포

```bash
# 1. PV 디렉토리 생성
sudo mkdir -p /data/kubeedge/cloudcore
sudo mkdir -p /data/influxdb
sudo mkdir -p /data/grafana
sudo mkdir -p /data/prometheus
sudo mkdir -p /data/alertmanager
sudo chmod 755 /data/*

# 2. Namespace 생성
kubectl apply -f 00-namespace.yaml

# 3. KubeEdge CRDs 설치 (위 사전 요구사항 참조)

# 4. CloudCore 배포
kubectl apply -f 01-cloudcore.yaml
kubectl wait --for=condition=ready pod -l app=cloudcore -n kubeedge --timeout=300s

# 5. InfluxDB 배포
kubectl apply -f 02-influxdb.yaml
kubectl wait --for=condition=ready pod -l app=influxdb -n kubeedge --timeout=300s

# 6. Grafana 배포
kubectl apply -f 03-grafana.yaml
kubectl wait --for=condition=ready pod -l app=grafana -n monitoring --timeout=180s

# 7. Prometheus 배포
kubectl apply -f 04-prometheus.yaml
kubectl wait --for=condition=ready pod -l app=prometheus -n monitoring --timeout=180s

# 8. AlertManager 배포
kubectl apply -f 05-alertmanager.yaml
kubectl wait --for=condition=ready pod -l app=alertmanager -n monitoring --timeout=120s

# 9. Device CRD 배포
kubectl apply -f 07-device-crd.yaml

# 10. Device Registration Service 배포
kubectl apply -f 06-device-registration.yaml
```

---

## ✅ 배포 확인

### 1. Pod 상태 확인

```bash
# kubeedge namespace
kubectl get pods -n kubeedge -o wide

# monitoring namespace
kubectl get pods -n monitoring -o wide
```

**예상 결과:**
```
NAMESPACE   NAME                                  READY   STATUS    RESTARTS   AGE
kubeedge    cloudcore-xxxxxxxx-xxxxx              1/1     Running   0          5m
kubeedge    influxdb-0                            1/1     Running   0          4m
kubeedge    device-registration-xxxxxxxx-xxxxx    1/1     Running   0          1m
monitoring  grafana-xxxxxxxx-xxxxx                1/1     Running   0          3m
monitoring  prometheus-0                          1/1     Running   0          3m
monitoring  alertmanager-xxxxxxxx-xxxxx           1/1     Running   0          2m
```

### 2. Service 확인

```bash
kubectl get svc -n kubeedge
kubectl get svc -n monitoring
```

### 3. PV/PVC 확인

```bash
kubectl get pv
kubectl get pvc -A
```

모든 PVC가 `Bound` 상태여야 합니다.

### 4. CloudCore 로그 확인

```bash
kubectl logs -n kubeedge -l app=cloudcore --tail=50
```

다음 메시지가 보이면 정상:
```
I1208 10:00:00.000000       1 server.go:xxx] Starting CloudHub
I1208 10:00:00.000000       1 server.go:xxx] CloudHub is listening on :10000
```

---

## 🌐 접속 URL

### Grafana
- **URL**: http://192.168.0.2:30300
- **계정**: admin / admin
- **첫 로그인 후 비밀번호 변경 권장**

### Prometheus
- **URL**: http://192.168.0.2:30090
- **인증**: 없음

### AlertManager
- **URL**: http://192.168.0.2:30093
- **인증**: 없음

### InfluxDB
- **URL**: http://192.168.0.2:8086 (ClusterIP, 내부 접근만)
- **계정**: admin / adminpassword
- **Organization**: kubeedge
- **Bucket**: sensor_data
- **Token**: `my-super-secret-admin-token`

---

## 🔐 Edge 노드 연결 준비

### CloudCore Token 가져오기

Edge 노드가 CloudCore에 연결하려면 토큰이 필요합니다:

```bash
# 토큰 가져오기 (keadm을 사용한 경우)
keadm gettoken --kube-config=/root/.kube/config

# 또는 직접 생성
kubectl create token cloudcore -n kubeedge --duration=87600h
```

### Port-forwarding 설정 (Router)

Edge 노드가 CloudCore에 접속하려면 다음 포트포워딩이 필요합니다:

**Router 1:**
```
External: 172.17.90.89:10000
Internal: 192.168.0.2:10000
Protocol: TCP
```

---

## 🐛 트러블슈팅

### 1. PVC가 Pending 상태

**원인**: PV가 생성되지 않음

**해결:**
```bash
# PV 상태 확인
kubectl get pv

# PV 디렉토리 확인
ls -la /data/

# 수동으로 PV 생성
kubectl apply -f 02-influxdb.yaml  # (PV 부분만 다시 적용)
```

### 2. CloudCore Pod가 시작하지 않음

**원인**: KubeEdge CRD 미설치

**해결:**
```bash
# CRD 설치 확인
kubectl get crd | grep kubeedge

# CRD 설치
kubectl apply -f https://raw.githubusercontent.com/kubeedge/kubeedge/release-1.15/build/crds/devices/devices_v1alpha2_device.yaml
kubectl apply -f https://raw.githubusercontent.com/kubeedge/kubeedge/release-1.15/build/crds/devices/devices_v1alpha2_devicemodel.yaml
```

### 3. Device Registration Service가 MQTT 연결 실패

**원인**: Edge MQTT Broker가 아직 구성되지 않음

**해결:**
- 이것은 정상입니다. Edge 노드 구성 후 자동으로 연결됩니다.
- 로그 확인: `kubectl logs -n kubeedge -l app=device-registration`

### 4. InfluxDB 초기화 실패

**원인**: 권한 문제 또는 디렉토리 부족

**해결:**
```bash
# 디렉토리 권한 확인
sudo ls -la /data/influxdb

# 권한 수정
sudo chown -R 1000:1000 /data/influxdb

# Pod 재시작
kubectl delete pod -n kubeedge -l app=influxdb
```

### 5. Grafana가 Datasource 연결 실패

**원인**: Prometheus 또는 InfluxDB가 준비되지 않음

**해결:**
```bash
# 서비스 확인
kubectl get svc -n kubeedge influxdb
kubectl get svc -n monitoring prometheus

# Grafana Pod 재시작
kubectl rollout restart deployment grafana -n monitoring
```

---

## 📊 다음 단계

### C-11 ~ C-17: 데이터 저장 및 시각화 (Week 5-6)

**Edge 노드 연결 후 수행:**

1. **InfluxDB 설정**
   ```bash
   # InfluxDB CLI 접속
   kubectl exec -it -n kubeedge influxdb-0 -- influx
   
   # Retention Policy 설정 (C-12)
   # Continuous Query 설정 (C-13)
   ```

2. **Grafana Dashboard 생성 (C-14 ~ C-17)**
   - Overview Dashboard
   - Device Metrics Dashboard
   - Edge Performance Dashboard
   - Anomaly Dashboard

### C-18 ~ C-21: 모니터링 및 알림 (Week 6-7)

**Edge 노드 메트릭 수집 후 수행:**

1. **Prometheus Scrape Config 업데이트 (C-18)**
   - Edge node-exporter 추가
   - MQTT exporter 추가
   - Redis exporter 추가

2. **Alert Rules 작성 (C-19)**
   - Device/Edge/Cloud 레벨별 알림 규칙

3. **AlertManager 설정 (C-20)**
   - Slack Webhook 추가
   - Email 설정

4. **Grafana Datasource 연동 (C-21)**
   - 이미 설정되어 있음, 확인만 필요

---

## 📝 참고 자료

- [KubeEdge 공식 문서](https://kubeedge.io/docs/)
- [InfluxDB Documentation](https://docs.influxdata.com/influxdb/)
- [Grafana Documentation](https://grafana.com/docs/)
- [Prometheus Documentation](https://prometheus.io/docs/)

---

## 🔄 업데이트 이력

| 날짜 | 버전 | 변경 내용 |
|------|------|----------|
| 2025-12-08 | 1.0 | 초기 Cloud 인프라 배포 |

---

**문서 버전**: v1.0  
**마지막 업데이트**: 2025-12-08
