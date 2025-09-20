#!/bin/bash

# Simulate two LocalPDub instances syncing with each other
# This creates two separate vault directories and runs two instances

echo "════════════════════════════════════════════════════"
echo "  LocalPDub Dual Instance Sync Simulation"
echo "════════════════════════════════════════════════════"
echo ""

# Set up two separate vault directories
VAULT1_DIR="/tmp/localpdub_vault1"
VAULT2_DIR="/tmp/localpdub_vault2"

echo "Setting up test environment..."

# Clean up any existing test directories
rm -rf "$VAULT1_DIR" "$VAULT2_DIR"

# Create directories
mkdir -p "$VAULT1_DIR/.localpdub"
mkdir -p "$VAULT2_DIR/.localpdub"

# Function to create a vault with test data
create_test_vault() {
    local vault_dir="$1"
    local vault_name="$2"
    local extra_entry="$3"

    echo "Creating $vault_name..."

    HOME="$vault_dir" timeout 10 ./build/localpdub 2>/dev/null <<EOF
testtest123
testtest123
3
Test Entry 1
user1@test.com

https://test1.com
user1@test.com
Common entry in both vaults
n
$extra_entry
9
EOF
}

# Create first vault with unique entry
{
    echo ""  # Skip first entry creation
    echo "3" # Add another entry
    echo "Vault1 Unique"
    echo "vault1user"
    echo "vault1pass123"
    echo "https://vault1only.com"
    echo "vault1@test.com"
    echo "This entry is only in vault 1"
    echo "n"
} > /tmp/vault1_extra.txt

# Create second vault with different unique entry
{
    echo ""  # Skip first entry creation
    echo "3" # Add another entry
    echo "Vault2 Unique"
    echo "vault2user"
    echo "vault2pass456"
    echo "https://vault2only.com"
    echo "vault2@test.com"
    echo "This entry is only in vault 2"
    echo "n"
} > /tmp/vault2_extra.txt

# Create both vaults
create_test_vault "$VAULT1_DIR" "Vault 1" "$(cat /tmp/vault1_extra.txt)"
create_test_vault "$VAULT2_DIR" "Vault 2" "$(cat /tmp/vault2_extra.txt)"

echo ""
echo "Initial vault contents:"
echo ""

# Show initial contents
echo "Vault 1 entries:"
HOME="$VAULT1_DIR" timeout 5 ./build/localpdub 2>/dev/null <<EOF | grep -A 20 "Password Entries"
testtest123
1
9
EOF

echo ""
echo "Vault 2 entries:"
HOME="$VAULT2_DIR" timeout 5 ./build/localpdub 2>/dev/null <<EOF | grep -A 20 "Password Entries"
testtest123
1
9
EOF

echo ""
echo "════════════════════════════════════════════════════"
echo "Starting sync simulation..."
echo "════════════════════════════════════════════════════"
echo ""

# Install screen if not available
if ! command -v screen >/dev/null 2>&1; then
    echo "Installing screen for multi-instance testing..."
    sudo apt-get install -y screen >/dev/null 2>&1
fi

if command -v screen >/dev/null 2>&1; then
    echo "Starting Instance 1 in discovery mode..."

    # Start first instance in screen with sync mode
    screen -dmS vault1 bash -c "
        cd $PWD
        HOME=$VAULT1_DIR ./build/localpdub <<EOF
testtest123
8
EOF
    "

    sleep 3

    echo "Starting Instance 2 in discovery mode..."

    # Start second instance in screen with sync mode
    screen -dmS vault2 bash -c "
        cd $PWD
        HOME=$VAULT2_DIR ./build/localpdub <<EOF
testtest123
8
EOF
    "

    echo ""
    echo "Both instances are running in discovery mode."
    echo ""

    # Check if screens are running
    echo "Active LocalPDub sessions:"
    screen -ls | grep -E "vault[12]"

    echo ""
    echo "Waiting 10 seconds for discovery..."
    sleep 10

    # Clean up screens
    echo ""
    echo "Stopping instances..."
    screen -S vault1 -X quit 2>/dev/null
    screen -S vault2 -X quit 2>/dev/null

    echo ""
    echo "════════════════════════════════════════════════════"
    echo "Manual Sync Instructions:"
    echo "════════════════════════════════════════════════════"
    echo ""
    echo "To manually test sync between instances:"
    echo ""
    echo "Terminal 1:"
    echo "  HOME=$VAULT1_DIR ./build/localpdub"
    echo "  [Enter password: testtest123]"
    echo "  [Select option 8 for sync]"
    echo ""
    echo "Terminal 2:"
    echo "  HOME=$VAULT2_DIR ./build/localpdub"
    echo "  [Enter password: testtest123]"
    echo "  [Select option 8 for sync]"
    echo ""
    echo "Both instances should discover each other within a few seconds."
    echo ""
else
    echo "Screen is not available. Manual testing required."
    echo ""
    echo "To test sync, open two terminals and run:"
    echo ""
    echo "Terminal 1:"
    echo "  HOME=$VAULT1_DIR ./build/localpdub"
    echo ""
    echo "Terminal 2:"
    echo "  HOME=$VAULT2_DIR ./build/localpdub"
    echo ""
fi

# Test network broadcast directly
echo "════════════════════════════════════════════════════"
echo "Testing network broadcast functionality..."
echo "════════════════════════════════════════════════════"
echo ""

# Start a UDP listener to check if broadcasts are working
timeout 5 nc -lu 51820 > /tmp/broadcast_test.txt 2>&1 &
NC_PID=$!

sleep 1

# Try to trigger a broadcast by starting sync mode briefly
HOME="$VAULT1_DIR" timeout 3 ./build/localpdub 2>/dev/null <<EOF &
testtest123
8
EOF

sleep 2

# Check if any broadcast was received
if [ -s /tmp/broadcast_test.txt ]; then
    echo "✓ UDP broadcasts are being sent"
    echo "Broadcast data received (first 100 chars):"
    head -c 100 /tmp/broadcast_test.txt
    echo ""
else
    echo "⚠ No UDP broadcasts detected"
    echo "This may be due to:"
    echo "  - Firewall blocking UDP port 51820"
    echo "  - Network configuration issues"
    echo "  - Broadcast not reaching loopback interface"
fi

# Clean up
kill $NC_PID 2>/dev/null
rm -f /tmp/broadcast_test.txt /tmp/vault1_extra.txt /tmp/vault2_extra.txt

echo ""
echo "════════════════════════════════════════════════════"
echo "Test Complete"
echo "════════════════════════════════════════════════════"
echo ""
echo "Test vaults created at:"
echo "  $VAULT1_DIR"
echo "  $VAULT2_DIR"
echo ""
echo "To clean up test vaults:"
echo "  rm -rf $VAULT1_DIR $VAULT2_DIR"
echo ""