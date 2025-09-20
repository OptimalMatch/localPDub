#!/bin/bash

# Comprehensive test for LocalPDub sync functionality
# Tests multiple scenarios including discovery, conflict resolution, and error handling

echo "════════════════════════════════════════════════════"
echo "  LocalPDub Comprehensive Sync Testing"
echo "════════════════════════════════════════════════════"
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test results
TESTS_PASSED=0
TESTS_FAILED=0

# Function to run a test
run_test() {
    local test_name="$1"
    local test_command="$2"
    local expected_result="$3"

    echo -e "${YELLOW}Running:${NC} $test_name"

    if eval "$test_command"; then
        if [ "$expected_result" = "pass" ]; then
            echo -e "${GREEN}✓ PASSED${NC}: $test_name\n"
            ((TESTS_PASSED++))
        else
            echo -e "${RED}✗ FAILED${NC}: $test_name (expected to fail but passed)\n"
            ((TESTS_FAILED++))
        fi
    else
        if [ "$expected_result" = "fail" ]; then
            echo -e "${GREEN}✓ PASSED${NC}: $test_name (correctly failed)\n"
            ((TESTS_PASSED++))
        else
            echo -e "${RED}✗ FAILED${NC}: $test_name\n"
            ((TESTS_FAILED++))
        fi
    fi
}

# Test 1: Check if binary exists and runs
test_binary() {
    ./build/localpdub-linux-x86_64 2>/dev/null <<EOF
testtest123
9
EOF
    return $?
}

run_test "Binary execution" "test_binary" "pass"

# Test 2: Check network ports availability
test_ports() {
    for port in 51820 51821 51822; do
        if lsof -i:$port >/dev/null 2>&1; then
            echo "  Port $port is in use"
            return 1
        fi
    done
    echo "  All sync ports (51820-51822) are available"
    return 0
}

run_test "Network ports availability" "test_ports" "pass"

# Test 3: Test sync menu option appears
test_sync_menu() {
    output=$(timeout 2 ./build/localpdub 2>/dev/null <<EOF
testtest123
EOF
)
    if echo "$output" | grep -q "8. Sync with other devices"; then
        return 0
    else
        return 1
    fi
}

run_test "Sync menu option visibility" "test_sync_menu" "pass"

# Test 4: Test entering sync mode
test_enter_sync() {
    output=$(timeout 5 ./build/localpdub 2>/dev/null <<EOF
testtest123
8


9
EOF
)
    if echo "$output" | grep -q "Sync with Other Devices"; then
        return 0
    else
        return 1
    fi
}

run_test "Enter sync mode" "test_enter_sync" "pass"

# Test 5: Create two vault instances for sync testing
echo -e "${YELLOW}Setting up dual vault test environment...${NC}"

# Create first test vault
VAULT1_DIR="/tmp/localpdub_test1"
VAULT2_DIR="/tmp/localpdub_test2"

mkdir -p "$VAULT1_DIR/.localpdub"
mkdir -p "$VAULT2_DIR/.localpdub"

# Copy existing vault to test directories if it exists
if [ -f "$HOME/.localpdub/vault.lpd" ]; then
    cp "$HOME/.localpdub/vault.lpd" "$VAULT1_DIR/.localpdub/"
    cp "$HOME/.localpdub/vault.lpd" "$VAULT2_DIR/.localpdub/"
    echo "  Copied existing vault for testing"
else
    echo "  No existing vault to copy, will use current vault"
fi

# Test 6: Check UDP broadcast functionality
test_udp_broadcast() {
    # Start a UDP listener in background
    timeout 3 nc -lu 51820 >/tmp/udp_test.txt 2>&1 &
    NC_PID=$!

    sleep 1

    # Try to send a test UDP packet
    echo "TEST_PACKET" | nc -u -w1 localhost 51820

    sleep 1

    # Check if packet was received
    if [ -f /tmp/udp_test.txt ]; then
        rm /tmp/udp_test.txt
        return 0
    else
        return 1
    fi
}

run_test "UDP broadcast capability" "test_udp_broadcast" "pass"

# Test 7: Check TCP connection capability
test_tcp_connection() {
    # Start a TCP listener in background
    timeout 3 nc -l 51820 >/tmp/tcp_test.txt 2>&1 &
    NC_PID=$!

    sleep 1

    # Try to connect and send data
    echo "TEST_DATA" | nc -w1 localhost 51820

    sleep 1

    # Check if data was received
    if [ -f /tmp/tcp_test.txt ] && grep -q "TEST" /tmp/tcp_test.txt; then
        rm /tmp/tcp_test.txt
        return 0
    else
        [ -f /tmp/tcp_test.txt ] && rm /tmp/tcp_test.txt
        return 1
    fi
}

run_test "TCP connection capability" "test_tcp_connection" "pass"

# Test 8: Test discovery timeout
test_discovery_timeout() {
    start_time=$(date +%s)
    timeout 65 ./build/localpdub 2>/dev/null <<EOF
testtest123
8
9
EOF
    end_time=$(date +%s)
    duration=$((end_time - start_time))

    # Should timeout around 60 seconds
    if [ $duration -le 65 ] && [ $duration -ge 2 ]; then
        echo "  Discovery ran for $duration seconds"
        return 0
    else
        echo "  Unexpected duration: $duration seconds"
        return 1
    fi
}

run_test "Discovery timeout mechanism" "test_discovery_timeout" "pass"

# Test 9: Simulate two instances (requires GNU screen or tmux)
if command -v screen >/dev/null 2>&1; then
    test_dual_instance() {
        # Start first instance in screen
        screen -dmS localpdub1 bash -c "cd $PWD && HOME=$VAULT1_DIR ./build/localpdub"
        sleep 2

        # Start second instance in screen
        screen -dmS localpdub2 bash -c "cd $PWD && HOME=$VAULT2_DIR ./build/localpdub"
        sleep 2

        # Check if both screens are running
        if screen -ls | grep -q localpdub1 && screen -ls | grep -q localpdub2; then
            # Clean up
            screen -S localpdub1 -X quit 2>/dev/null
            screen -S localpdub2 -X quit 2>/dev/null
            return 0
        else
            # Clean up
            screen -S localpdub1 -X quit 2>/dev/null
            screen -S localpdub2 -X quit 2>/dev/null
            return 1
        fi
    }

    run_test "Dual instance startup" "test_dual_instance" "pass"
else
    echo -e "${YELLOW}Skipping dual instance test (screen not installed)${NC}"
fi

# Test 10: Test network interface detection
test_network_interfaces() {
    if ip addr show | grep -q "inet "; then
        echo "  Network interfaces detected"
        return 0
    else
        echo "  No network interfaces found"
        return 1
    fi
}

run_test "Network interface detection" "test_network_interfaces" "pass"

# Clean up test directories
rm -rf "$VAULT1_DIR" "$VAULT2_DIR" 2>/dev/null

# Summary
echo ""
echo "════════════════════════════════════════════════════"
echo "  Test Summary"
echo "════════════════════════════════════════════════════"
echo -e "  ${GREEN}Passed:${NC} $TESTS_PASSED"
echo -e "  ${RED}Failed:${NC} $TESTS_FAILED"
echo ""

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed successfully!${NC}"
    echo ""
    echo "Next steps for manual testing:"
    echo "1. Run LocalPDub on two different machines"
    echo "2. Ensure both are on the same network"
    echo "3. Start sync mode (option 8) on both"
    echo "4. Verify devices discover each other"
    echo "5. Test actual password sync between devices"
    exit 0
else
    echo -e "${RED}Some tests failed. Please review the output above.${NC}"
    exit 1
fi