#!/bin/bash

echo "════════════════════════════════════════════════════"
echo "  Testing Sync Timeout with Input"
echo "════════════════════════════════════════════════════"
echo ""

# Kill any existing localpdub processes
pkill -f localpdub 2>/dev/null

# Create test vault
VAULT1="/tmp/test_timeout_vault"
rm -rf "$VAULT1"
mkdir -p "$VAULT1/.localpdub"

echo "Creating new vault..."
echo -e "testpass123\ntestpass123\n9" | HOME="$VAULT1" ./build/localpdub 2>/dev/null

echo ""
echo "Testing timeout with 'all' selection..."
echo ""

# Start device, wait for timeout, then try to select all
{
    echo "testpass123"       # Enter password
    echo "y"                  # Start sync
    sleep 65                  # Wait for 60-second timeout + buffer
    echo "all"                # Try to select all devices
    sleep 2                   # Wait for crash or next prompt
    echo "q"                  # Try to quit
    echo ""                   # Confirm quit
} | HOME="$VAULT1" timeout 75 ./build/localpdub 2>&1

echo ""
echo "════════════════════════════════════════════════════"
echo "Test completed. Check for crashes above."
echo ""

# Check if process is still running
if pgrep -f "HOME=$VAULT1.*localpdub" >/dev/null; then
    echo "⚠ Warning: Process still running (possible hang)"
    pkill -f "HOME=$VAULT1.*localpdub"
else
    echo "✓ Process terminated"
fi

echo ""
echo "Cleanup test directory..."
rm -rf "$VAULT1"
echo "Done!"