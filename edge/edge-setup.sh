#!/bin/bash
# edge-setup.sh
# Edge Node 초기 설정 스크립트
# Ubuntu 24.04 LTS on Raspberry Pi 4

set -e

# ===============================================
# 변수 설정
# ===============================================
NODE_NUM=${1:-1}  # 노드 번호 (1, 2, 3)
NODE_IP="192.168.0.10${NODE_NUM}"
CLOUDCORE_IP="192.168.0.2"
CLOUDCORE_PORT="10000"
CLOUDCORE_TOKEN="${2:-}"  # CloudCore 토큰 (두 번째 인자)

if [ -z "$CLOUDCORE_TOKEN" ]; then
    echo "❌ 오류: CloudCore 토큰이 필요합니다."
    echo "사용법: $0 <노드번호> <CloudCore토큰>"
    echo "예시: $0 1 eyJhbGciOiJSUzI1NiIsImtpZCI6..."
    exit 1
fi

echo "================================================"
echo "   Edge Node $NODE_NUM 설정 시작"
echo "   IP: $NODE_IP"
echo "================================================"

# ===============================================
# E-01: Ubuntu 24.04 LTS 확인
# ===============================================
echo "✅ [E-01] Ubuntu 버전 확인..."
if ! grep -q "Ubuntu 24.04" /etc/os-release; then
    echo "❌ Ubuntu 24.04 LTS가 필요합니다."
    exit 1
fi
echo "   ✓ Ubuntu 24.04 LTS 확인됨"

# ===============================================
# E-02: 고정 IP 설정 (netplan)
# ===============================================
echo "✅ [E-02] 고정 IP 설정..."

# 기존 netplan 설정 백업
sudo cp /etc/netplan/*.yaml /etc/netplan/backup_$(date +%Y%m%d_%H%M%S).yaml 2>/dev/null || true

# netplan 설정 파일 생성
sudo tee /etc/netplan/01-network.yaml > /dev/null << EOF
network:
  version: 2
  renderer: networkd
  ethernets:
    eth0:
      dhcp4: no
      dhcp6: no
      addresses:
        - $NODE_IP/24
      routes:
        - to: default
          via: 192.168.0.1
      nameservers:
        addresses:
          - 8.8.8.8
          - 8.8.4.4
      optional: false
EOF

# netplan 적용
sudo netplan apply
sleep 2

echo "   ✓ 고정 IP $NODE_IP 설정 완료"

# ===============================================
# E-03: 라우터 연결 확인
# ===============================================
echo "✅ [E-03] 라우터 연결 확인..."
if ping -c 3 192.168.0.1 > /dev/null 2>&1; then
    echo "   ✓ Router 2 (192.168.0.1) 연결 확인"
else
    echo "   ⚠️  Router 2 연결 불가 - 계속 진행"
fi

# Cloud 연결 확인
if ping -c 3 $CLOUDCORE_IP > /dev/null 2>&1; then
    echo "   ✓ Cloud ($CLOUDCORE_IP) 연결 확인"
else
    echo "   ❌ Cloud 연결 불가 - 네트워크 설정 확인 필요"
fi

# ===============================================
# E-04: 시스템 패키지 업데이트
# ===============================================
echo "✅ [E-04] 시스템 패키지 업데이트..."
sudo apt-get update
sudo apt-get upgrade -y
sudo apt-get install -y \
    curl \
    wget \
    git \
    vim \
    net-tools \
    ca-certificates \
    apt-transport-https \
    software-properties-common

echo "   ✓ 시스템 업데이트 완료"

# ===============================================
# E-05: containerd 설치
# ===============================================
echo "✅ [E-05] containerd 설치..."

# containerd 설치
sudo apt-get install -y containerd

# containerd 설정
sudo mkdir -p /etc/containerd
containerd config default | sudo tee /etc/containerd/config.toml > /dev/null

# systemd cgroup 드라이버 사용
sudo sed -i 's/SystemdCgroup = false/SystemdCgroup = true/' /etc/containerd/config.toml

# containerd 재시작
sudo systemctl restart containerd
sudo systemctl enable containerd

echo "   ✓ containerd 설치 완료"

# ===============================================
# E-06: EdgeCore 설치 (keadm)
# ===============================================
echo "✅ [E-06] KubeEdge EdgeCore 설치..."

# keadm 다운로드 (ARM64)
KUBEEDGE_VERSION="v1.15.0"
wget https://github.com/kubeedge/kubeedge/releases/download/${KUBEEDGE_VERSION}/keadm-${KUBEEDGE_VERSION}-linux-arm64.tar.gz
tar -xzf keadm-${KUBEEDGE_VERSION}-linux-arm64.tar.gz
sudo mv keadm-${KUBEEDGE_VERSION}-linux-arm64/keadm/keadm /usr/local/bin/
sudo chmod +x /usr/local/bin/keadm
rm -rf keadm-${KUBEEDGE_VERSION}-linux-arm64*

# EdgeCore 설치
sudo keadm join \
    --cloudcore-ipport=${CLOUDCORE_IP}:${CLOUDCORE_PORT} \
    --token=${CLOUDCORE_TOKEN} \
    --edgenode-name=edge-node-${NODE_NUM} \
    --kubeedge-version=${KUBEEDGE_VERSION}

echo "   ✓ EdgeCore 설치 완료"

# ===============================================
# E-07: EdgeCore 서비스 확인
# ===============================================
echo "✅ [E-07] EdgeCore 서비스 확인..."
sleep 5

if systemctl is-active --quiet edgecore; then
    echo "   ✓ EdgeCore 서비스 실행 중"
else
    echo "   ❌ EdgeCore 서비스 시작 실패"
    sudo journalctl -u edgecore -n 50
    exit 1
fi

# ===============================================
# E-08: Edge-Cloud 연결 테스트
# ===============================================
echo "✅ [E-08] Edge-Cloud 연결 테스트..."

# EdgeCore 로그 확인
if sudo journalctl -u edgecore --since "1 minute ago" | grep -q "successfully"; then
    echo "   ✓ Edge-Cloud 연결 성공"
else
    echo "   ⚠️  연결 확인 중 - EdgeCore 로그 확인:"
    sudo journalctl -u edgecore -n 20
fi

# ===============================================
# 완료
# ===============================================
echo ""
echo "================================================"
echo "   ✅ Edge Node $NODE_NUM 기본 설정 완료!"
echo "================================================"
echo ""
echo "다음 단계:"
echo "  1. Cloud에서 노드 확인: kubectl get nodes"
echo "  2. MQTT Broker 설치: ./install-mqtt.sh"
echo "  3. Redis 설치: ./install-redis.sh"
echo ""

KUBEEDGE_VERSION="v1.15.0"
wget https://github.com/kubeedge/kubeedge/releases/download/${KUBEEDGE_VERSION}/keadm-${KUBEEDGE_VERSION}-linux-arm64.tar.gz
tar -xzf keadm-${KUBEEDGE_VERSION}-linux-arm64.tar.gz
sudo mv keadm-${KUBEEDGE_VERSION}-linux-arm64/keadm/keadm /usr/local/bin/
sudo chmod +x /usr/local/bin/keadm
rm -rf keadm-${KUBEEDGE_VERSION}-linux-arm64*


# # keadm 다운로드 (ARM64)
# KUBEEDGE_VERSION="v1.15.0"
# wget https://github.com/kubeedge/kubeedge/releases/download/${KUBEEDGE_VERSION}/keadm-${KUBEEDGE_VERSION}-linux-arm64.tar.gz
# tar -xzf keadm-${KUBEEDGE_VERSION}-linux-arm64.tar.gz
# sudo mv keadm-${KUBEEDGE_VERSION}-linux-arm64/keadm/keadm /usr/local/bin/
# sudo chmod +x /usr/local/bin/keadm
# rm -rf keadm-${KUBEEDGE_VERSION}-linux-arm64*

# CLOUDCORE_TOKEN=eyJhbGciOiJSUzI1NiIsImtpZCI6IndPdXVOYTVOSV9MbWFZUWJtOHcxTWV6NGY5T1k1R2V3OWF1LU1PeTFEd1kifQ.eyJhdWQiOlsiaHR0cHM6Ly9rdWJlcm5ldGVzLmRlZmF1bHQuc3ZjLmNsdXN0ZXIubG9jYWwiXSwiZXhwIjoxNzk2NzQzMDExLCJpYXQiOjE3NjUyMDcwMTEsImlzcyI6Imh0dHBzOi8va3ViZXJuZXRlcy5kZWZhdWx0LnN2Yy5jbHVzdGVyLmxvY2FsIiwianRpIjoiOThmNjU4ZGItMTlhZi00ZDA3LWJiYmItNzk4NmY5ZDdkZTNmIiwia3ViZXJuZXRlcy5pbyI6eyJuYW1lc3BhY2UiOiJrdWJlZWRnZSIsInNlcnZpY2VhY2NvdW50Ijp7Im5hbWUiOiJjbG91ZGNvcmUiLCJ1aWQiOiI5ZmJlYTQyMS1iNmQ3LTQzMzktOTEyMC02YzljNzNkYzExOGEifX0sIm5iZiI6MTc2NTIwNzAxMSwic3ViIjoic3lzdGVtOnNlcnZpY2VhY2NvdW50Omt1YmVlZGdlOmNsb3VkY29yZSJ9.nGARP5_saBr9qQAD9qYiympuuEvcTrhGZcdOzzEeWUZzB5YJ-lsWPhPVwUBnjQbACX_g3Y7_-98bH_2L9V7yugh9IsUgefeiBNUoVQHRpHyxA80Y19z-Ff-zdOHWTS1r9MaNHzdacbW1_i_OYNIOBhutDPs0Jr6ECq_EW9ZcdE_oa5wxQ4s5TiNVzkIs0UcUFFc6Arv7ANVxbvYd9o1JCRuifHXyCr2XmAq7TXNOe48jVsmRiaL7OiqhB7jlJvYSFxhlxkG11SQQRIDSsXkJooioXma2lUUqVFrmW-UgSdDWBdqG1Vp9horxVCSz9dRK8N9T0jf7lC_zIqNMELMYNQ
# # EdgeCore 설치
# sudo keadm join \
#     --cloudcore-ipport=172.17.90.89:10000 \
#     --token=${CLOUDCORE_TOKEN} \
#     --edgenode-name=edge-node-3 \
#     --kubeedge-version="v1.15.0"