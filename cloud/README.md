# Cloud Layer Guide

## 1. 목적

- Cloud 계층 배포/운영 파일 집합
- CloudCore, 모니터링 스택, Edge 대상 워크로드 매니페스트 관리

## 2. 디렉터리 구성

| 경로 | 설명 |
|---|---|
| `cloudcore/` | CloudCore + Monitoring 배포 파일/스크립트 |
| `edge-manifests/` | Edge 노드 대상 워크로드(Device Registration, Data Validator, 테스트 리소스) |
| `helm/` | `cilium`, `rook-ceph` 로컬 차트 |
| `utils/` | 운영 복구 스크립트/유틸 파일 |

상세 배포 내용: `cloud/cloudcore/README.md`

## 3. 권장 배포 순서

### Step 1. CloudCore + Monitoring

신규 클러스터:

1. `cloud/cloudcore/README.md` 기준 선행 조건 확인
2. `kubeedge/`, `monitoring/` 매니페스트 배포

CloudCore가 `keadm init`으로 이미 설치된 경우:

```bash
cd cloud/cloudcore
sudo ./deploy-cloud.sh
```

### Step 2. Edge 대상 워크로드

Device Registration (`registeration/` 디렉터리명 주의):

```bash
kubectl apply -f cloud/edge-manifests/registeration/device-registration-0.yaml
kubectl apply -f cloud/edge-manifests/registeration/device-registration-1.yaml
kubectl apply -f cloud/edge-manifests/registeration/device-registration-2.yaml
kubectl apply -f cloud/edge-manifests/registeration/device-registration-3.yaml
```

Data Validator:

```bash
kubectl apply -f cloud/edge-manifests/data-validator/secret.yaml
kubectl apply -f cloud/edge-manifests/data-validator/deployment.yaml
kubectl apply -f cloud/edge-manifests/data-validator/deployment2.yaml
kubectl apply -f cloud/edge-manifests/data-validator/deployment3.yaml
kubectl apply -f cloud/edge-manifests/data-validator/service.yaml
```

## 4. 적용 전 체크리스트

- `nodeSelector` 호스트명: `edge-server1`, `edge-002`, `edge-003`
- `MQTT_BROKER`, `ALERT_SERVER_URL`
- `INFLUX_URL`, `INFLUX_ORG`, `INFLUX_BUCKET`
- `secret.yaml` Influx 토큰
- `hostNetwork: true` 필요 여부

## 5. 운영 확인

```bash
kubectl get pods -A -o wide
kubectl get svc -n kubeedge
kubectl get svc -n monitoring
kubectl get devices -A
```

로그 확인:

```bash
kubectl logs -n kubeedge -l app=device-registration --tail=100
kubectl logs -n kubeedge -l app=data-validator --tail=100
```

## 6. 운영 유틸리티

복구 스크립트: `cloud/utils/remove_finalizer.sh`

```bash
cd cloud/utils
chmod +x remove_finalizer.sh
./remove_finalizer.sh
```

주의:

- `kubeedge` 네임스페이스 강제 정리 수행
- 후속 `keadm init` 재실행 포함
- 실행 전 현재 클러스터 영향 범위 확인 필수

## 7. 참고

- CloudCore/모니터링 상세: `cloud/cloudcore/README.md`
- Helm 차트: 버전 고정/템플릿 참고용 로컬 보관본
