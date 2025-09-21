#!/bin/bash

echo "════════════════════════════════════════════════════"
echo "  Testing Sync Deadlock Fix"
echo "════════════════════════════════════════════════════"
echo ""

# Kill any existing localpdub processes
pkill -f localpdub 2>/dev/null

# Create two test vaults with different content
VAULT1="/tmp/test_vault1"
VAULT2="/tmp/test_vault2"

rm -rf "$VAULT1" "$VAULT2"
mkdir -p "$VAULT1/.localpdub"
mkdir -p "$VAULT2/.localpdub"

# Copy existing vault to first test directory
if [ -f "$HOME/.localpdub/vault.lpd" ]; then
    cp "$HOME/.localpdub/vault.lpd" "$VAULT1/.localpdub/"
    echo "Vault 1 initialized with existing data"
else
    echo "Creating new vault 1..."
    echo -e "testpass123\ntestpass123\n9" | HOME="$VAULT1" ./build/localpdub 2>/dev/null
fi

# Create empty vault 2
echo "Creating new vault 2..."
echo -e "testpass123\ntestpass123\n9" | HOME="$VAULT2" ./build/localpdub 2>/dev/null

echo ""
echo "Starting sync test..."
echo ""
echo "Starting Device 1 sync discovery in background..."

# Start first device in sync mode (background)
{
    sleep 2
    echo "testpass123"
    echo "y"  # Start sync
    sleep 30  # Keep it running for discovery
    echo ""   # Stop discovery
    echo "q"  # Quit
    echo ""   # Confirm quit
} | HOME="$VAULT1" timeout 40 ./build/localpdub 2>&1 | sed 's/^/[Device1] /' &
PID1=$!

sleep 3

echo "Starting Device 2 sync discovery..."
echo ""

# Start second device in sync mode (foreground to see output)
{
    echo "testpass123"
    echo "y"  # Start sync
    sleep 10  # Let it discover
    echo ""   # Stop discovery
    echo "q"  # Quit
    echo ""   # Confirm quit
} | HOME="$VAULT2" timeout 20 ./build/localpdub 2>&1 | sed 's/^/[Device2] /'

# Wait for background process
wait $PID1 2>/dev/null

echo ""
echo "════════════════════════════════════════════════════"
echo "Test completed. Check above for any deadlock errors."
echo ""

# Check if processes are still running (shouldn't be)
if pgrep -f "HOME=$VAULT1.*localpdub" >/dev/null; then
    echo "⚠ Warning: Device 1 process still running (possible deadlock)"
    pkill -f "HOME=$VAULT1.*localpdub"
else
    echo "✓ Device 1 terminated correctly"
fi

if pgrep -f "HOME=$VAULT2.*localpdub" >/dev/null; then
    echo "⚠ Warning: Device 2 process still running (possible deadlock)"
    pkill -f "HOME=$VAULT2.*localpdub"
else
    echo "✓ Device 2 terminated correctly"
fi

echo ""
echo "Cleanup test directories..."
rm -rf "$VAULT1" "$VAULT2"
echo "Done!"