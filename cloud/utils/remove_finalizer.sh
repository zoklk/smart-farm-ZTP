#!/bin/bash
# complete-recovery.sh

set -e

NAMESPACE="kubeedge"
CLOUD_IP="192.168.0.2"
KUBEEDGE_VERSION="1.15.0"

echo "========================================="
echo "Complete CloudCore Recovery"
echo "========================================="
echo ""

# Step 1: Terminating Namespace 강제 삭제
echo "[Step 1/4] Forcing deletion of terminating namespace..."

if kubectl get namespace $NAMESPACE &>/dev/null; then
    PHASE=$(kubectl get namespace $NAMESPACE -o jsonpath='{.status.phase}')
    
    if [ "$PHASE" == "Terminating" ]; then
        echo "  Namespace is Terminating. Forcing deletion..."
        
        # spec.finalizers 제거
        kubectl get namespace $NAMESPACE -o json \
          | jq '.spec.finalizers = []' \
          | kubectl replace --raw "/api/v1/namespaces/$NAMESPACE/finalize" -f - \
          || echo "  spec.finalizers removal failed, trying alternative..."
        
        # metadata.finalizers도 제거
        kubectl patch namespace $NAMESPACE -p '{"metadata":{"finalizers":null}}' --type=merge \
          || echo "  metadata.finalizers removal failed"
        
        # 확인
        sleep 3
        if kubectl get namespace $NAMESPACE &>/dev/null; then
            echo "  ❌ Failed to delete namespace. Manual intervention required."
            echo "  Try: kubectl delete namespace $NAMESPACE --grace-period=0 --force"
            exit 1
        else
            echo "  ✅ Namespace deleted successfully"
        fi
    else
        echo "  Namespace exists but not terminating. Deleting normally..."
        kubectl delete namespace $NAMESPACE --timeout=30s || true
    fi
else
    echo "  Namespace does not exist (already deleted)"
fi

echo ""
sleep 2

# Step 2: 새 Namespace 생성
echo "[Step 2/4] Creating new kubeedge namespace..."
kubectl create namespace $NAMESPACE
echo "  ✅ Namespace created"
echo ""

# Step 3: CloudCore 설치
echo "[Step 3/4] Installing CloudCore..."
keadm init --advertise-address=$CLOUD_IP --kubeedge-version=$KUBEEDGE_VERSION
echo "  ✅ CloudCore installed"
echo ""

# Step 4: 상태 확인
echo "[Step 4/4] Verifying installation..."
sleep 10

kubectl wait --for=condition=Ready pod -l app=cloudcore -n $NAMESPACE --timeout=120s
echo "  ✅ CloudCore is running"
echo ""

# Device CRD 설치
echo "Installing Device CRDs..."
kubectl apply -f https://raw.githubusercontent.com/kubeedge/kubeedge/release-1.15/build/crds/devices/devices_v1alpha2_device.yaml
kubectl apply -f https://raw.githubusercontent.com/kubeedge/kubeedge/release-1.15/build/crds/devices/devices_v1alpha2_devicemodel.yaml
echo "  ✅ Device CRDs installed"
echo ""

# 토큰 생성
echo "Generating Edge token..."
TOKEN=$(keadm gettoken)
TOKEN_FILE="/tmp/edge-token-$(date +%Y%m%d-%H%M%S).txt"
echo "$TOKEN" > $TOKEN_FILE
echo ""
echo "========================================="
echo "Edge Token:"
echo "$TOKEN"
echo "========================================="
echo "Token saved to: $TOKEN_FILE"
echo ""

echo "========================================="
echo "Recovery Complete!"
echo "========================================="
echo ""
echo "Next: Reconnect Edge nodes with the token above"
echo ""
