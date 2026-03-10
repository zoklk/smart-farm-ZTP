#!/bin/bash
#
# KubeEdge Cloud Infrastructure Deployment (without CloudCore)
# Purpose: Deploy monitoring and data storage components only
# Note: Assumes CloudCore is already installed via keadm
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

wait_for_pod() {
    local namespace=$1
    local label=$2
    local timeout=${3:-300}
    
    print_status "Waiting for pod with label '$label' in namespace '$namespace'..."
    kubectl wait --for=condition=ready pod \
        -l "$label" \
        -n "$namespace" \
        --timeout="${timeout}s" 2>/dev/null || {
        print_warning "Timeout waiting for pod, checking status..."
        kubectl get pods -n "$namespace" -l "$label"
        return 1
    }
    print_status "Pod is ready!"
    return 0
}

create_pv_directories() {
    print_status "Creating PV directories..."
    
    local directories=(
        "/data/influxdb"
        "/data/grafana"
        "/data/prometheus"
        "/data/alertmanager"
    )
    
    for dir in "${directories[@]}"; do
        if [ ! -d "$dir" ]; then
            print_status "Creating: $dir"
            sudo mkdir -p "$dir"
            sudo chmod 755 "$dir"
        else
            print_status "Already exists: $dir"
        fi
    done
}

main() {
    print_status "KubeEdge Cloud Infrastructure Deployment (Simplified)"
    print_status "======================================================"
    echo ""
    
    # Check kubectl
    if ! command -v kubectl &> /dev/null; then
        print_error "kubectl not found"
        exit 1
    fi
    
    # Check cluster
    if ! kubectl cluster-info &> /dev/null; then
        print_error "Cannot connect to Kubernetes cluster"
        exit 1
    fi
    
    print_status "Connected to Kubernetes cluster"
    echo ""
    
    # Check CloudCore
    print_status "Checking CloudCore installation..."
    if kubectl get deployment cloudcore -n kubeedge &>/dev/null; then
        print_status "✓ CloudCore found (keadm installation)"
        kubectl get pods -n kubeedge -l k8s-app=kubeedge
    else
        print_error "CloudCore not found!"
        print_error "Please install CloudCore first:"
        print_error "  keadm init --advertise-address=192.168.0.2 --kubeedge-version=1.15.0"
        exit 1
    fi
    echo ""
    
    # Create PV directories
    create_pv_directories
    echo ""
    
    # Deploy InfluxDB
    print_status "Deploying InfluxDB..."
    kubectl apply -f 02-influxdb.yaml
    wait_for_pod "kubeedge" "app=influxdb" 300
    echo ""
    
    # Deploy Grafana
    print_status "Deploying Grafana..."
    kubectl apply -f 03-grafana.yaml
    wait_for_pod "monitoring" "app=grafana" 180
    echo ""
    
    # Deploy Prometheus
    print_status "Deploying Prometheus..."
    kubectl apply -f 04-prometheus.yaml
    wait_for_pod "monitoring" "app=prometheus" 180
    echo ""
    
    # Deploy AlertManager
    print_status "Deploying AlertManager..."
    kubectl apply -f 05-alertmanager.yaml
    wait_for_pod "monitoring" "app=alertmanager" 120
    echo ""
    
    # Deploy Device CRD
    print_status "Deploying Device CRD..."
    kubectl apply -f 07-device-crd.yaml
    sleep 2
    echo ""
    
    # Deploy Device Registration Service
    print_status "Deploying Device Registration Service..."
    kubectl apply -f 06-device-registration.yaml
    wait_for_pod "kubeedge" "app=device-registration" 120 || \
        print_warning "Device Registration may need MQTT broker (will work after Edge setup)"
    echo ""
    
    # Summary
    print_status "======================================================"
    print_status "Deployment Complete!"
    print_status "======================================================"
    echo ""
    
    print_status "Pod Status:"
    kubectl get pods -n kubeedge
    kubectl get pods -n monitoring
    echo ""
    
    print_status "Service Status:"
    kubectl get svc -n kubeedge
    kubectl get svc -n monitoring
    echo ""
    
    print_status "Access URLs:"
    echo "  - Grafana:      http://192.168.0.2:30300 (admin/admin)"
    echo "  - Prometheus:   http://192.168.0.2:30090"
    echo "  - AlertManager: http://192.168.0.2:30093"
    echo "  - CloudCore:    ws://192.168.0.2:10000"
    echo ""
    
    print_status "Next Steps:"
    echo "  1. Get CloudCore token: kubectl create token cloudcore -n kubeedge --duration=87600h"
    echo "  2. Configure Edge nodes"
    echo "  3. Update Device Registration Service MQTT address"
    echo ""
}

main "$@"