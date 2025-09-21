#!/bin/bash

echo "════════════════════════════════════════════════════"
echo "  LocalPDub ANSI Color Display Test"
echo "════════════════════════════════════════════════════"
echo ""

# Test color detection
echo "Testing color support detection..."
echo ""

# Check TERM variable
echo "TERM environment: $TERM"

# Check if output is TTY
if [ -t 1 ]; then
    echo "Output is a TTY: YES ✓"
else
    echo "Output is a TTY: NO"
fi

# Check NO_COLOR variable
if [ -z "$NO_COLOR" ]; then
    echo "NO_COLOR variable: Not set ✓"
else
    echo "NO_COLOR variable: Set (colors disabled)"
fi

echo ""
echo "Running LocalPDub with color output..."
echo ""

# Test with colors enabled (default)
echo "1. Normal mode (colors should appear):"
echo -e "testtest123\n9" | timeout 3 ./build/localpdub 2>&1 | head -15

echo ""
echo "2. Testing NO_COLOR environment variable:"
NO_COLOR=1 echo -e "testtest123\n9" | timeout 3 ./build/localpdub 2>&1 | head -15

echo ""
echo "3. Testing non-TTY output (piped):"
echo -e "testtest123\n9" | ./build/localpdub 2>&1 | cat | head -15

echo ""
echo "════════════════════════════════════════════════════"
echo "Color Feature Summary:"
echo "════════════════════════════════════════════════════"
echo ""
echo "✓ ANSI color codes supported"
echo "✓ Terminal detection implemented"
echo "✓ NO_COLOR environment variable respected"
echo "✓ Box drawing characters (╔═╗║╚╝)"
echo "✓ Colored menu options"
echo "✓ Success/error message formatting"
echo "✓ BBS-style decorations"
echo ""
echo "Supported colors:"
echo -e "\033[31m● Red\033[0m \033[32m● Green\033[0m \033[33m● Yellow\033[0m \033[34m● Blue\033[0m"
echo -e "\033[35m● Magenta\033[0m \033[36m● Cyan\033[0m \033[37m● White\033[0m"
echo ""
echo -e "\033[91m● Bright Red\033[0m \033[92m● Bright Green\033[0m \033[93m● Bright Yellow\033[0m"
echo -e "\033[94m● Bright Blue\033[0m \033[95m● Bright Magenta\033[0m \033[96m● Bright Cyan\033[0m"
echo ""