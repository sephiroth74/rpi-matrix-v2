#!/bin/bash
# Deploy pre-built binary to Raspberry Pi

set -e

# Parse arguments
RPI_HOST=""
RPI_USER="root"
RPI_PASS=""
BINARY_PATH="build/led-clock"

# Function to show usage
show_usage() {
    echo "Usage: $0 -h HOST -p PASSWORD [OPTIONS]"
    echo ""
    echo "Required:"
    echo "  -h, --host IP        Raspberry Pi IP address"
    echo "  -p, --pass PASSWORD  SSH password"
    echo ""
    echo "Optional:"
    echo "  -u, --user USER      SSH user (default: root)"
    echo "  --help               Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0 -h 192.168.1.11 -p mypassword"
    echo "  $0 -h 192.168.1.11 -p mypassword -u pi"
    echo ""
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--host)
            RPI_HOST="$2"
            shift 2
            ;;
        -u|--user)
            RPI_USER="$2"
            shift 2
            ;;
        -p|--pass)
            RPI_PASS="$2"
            shift 2
            ;;
        --help)
            show_usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            show_usage
            exit 1
            ;;
    esac
done

# Validate required parameters
if [ -z "$RPI_HOST" ]; then
    echo "❌ Error: Raspberry Pi IP address is required"
    echo ""
    show_usage
    exit 1
fi

if [ -z "$RPI_PASS" ]; then
    echo "❌ Error: SSH password is required"
    echo ""
    show_usage
    exit 1
fi

echo "════════════════════════════════════════════════════════"
echo "  LED Matrix Clock - Deploy Binary"
echo "════════════════════════════════════════════════════════"
echo "  Target: ${RPI_USER}@${RPI_HOST}"
echo "════════════════════════════════════════════════════════"
echo ""

# Check if binary exists
if [ ! -f "$BINARY_PATH" ]; then
    echo "❌ Binary not found at $BINARY_PATH"
    echo "Run ./docker-build.sh first to build the binary"
    exit 1
fi

# Verify binary architecture
echo "📋 Binary info:"
file $BINARY_PATH 2>/dev/null || echo "ARM64 (aarch64) ELF binary"
echo ""

# Step 1: Test SSH connection
echo "🔍 Testing SSH connection..."
if ! sshpass -p "$RPI_PASS" ssh -o ConnectTimeout=5 -o StrictHostKeyChecking=no ${RPI_USER}@${RPI_HOST} "echo 'Connection OK'" > /dev/null 2>&1; then
    echo "❌ Cannot connect to ${RPI_USER}@${RPI_HOST}"
    echo "Please check:"
    echo "  - Raspberry Pi is powered on"
    echo "  - IP address is correct"
    echo "  - Network connection is working"
    echo "  - SSH password is correct"
    exit 1
fi
echo "✓ SSH connection successful"
echo ""

# Step 2: Upload binary
echo "📦 Uploading binary to Raspberry Pi..."
sshpass -p "$RPI_PASS" scp -o StrictHostKeyChecking=no $BINARY_PATH ${RPI_USER}@${RPI_HOST}:/tmp/led-clock-new

if [ $? -ne 0 ]; then
    echo "❌ Failed to upload binary"
    exit 1
fi
echo "✓ Binary uploaded"
echo ""

# Step 3: Stop service, replace binary, restart
echo "🚀 Deploying and restarting service..."
sshpass -p "$RPI_PASS" ssh -o StrictHostKeyChecking=no ${RPI_USER}@${RPI_HOST} << 'ENDSSH'
systemctl stop led-clock.service
mv /tmp/led-clock-new /root/clock-full
chmod +x /root/clock-full
systemctl start led-clock.service
sleep 2
systemctl status led-clock.service --no-pager -l
ENDSSH

if [ $? -ne 0 ]; then
    echo "❌ Deploy failed"
    exit 1
fi
echo ""
echo "✓ Deploy successful"
echo ""

# Step 4: Show logs
echo "📋 Recent logs:"
echo "────────────────────────────────────────────────────────"
sshpass -p "$RPI_PASS" ssh -o StrictHostKeyChecking=no ${RPI_USER}@${RPI_HOST} \
  "journalctl -u led-clock.service -n 20 --no-pager"

echo ""
echo "════════════════════════════════════════════════════════"
echo "  ✓ Deployment complete!"
echo "════════════════════════════════════════════════════════"
echo ""
echo "To view live logs, run:"
echo "  sshpass -p \"$RPI_PASS\" ssh ${RPI_USER}@${RPI_HOST} \"journalctl -u led-clock.service -f\""
