#!/bin/bash

# Test LocalPDub CLI by piping commands
# This assumes vault already exists with password: testtest123

echo "Testing LocalPDub CLI..."

# Create a sequence of commands to pipe into the application
{
    echo "testtest123"     # Master password
    sleep 0.5

    echo "3"               # Add new entry
    echo "Test Website"    # Title
    echo "user@test.com"   # Username
    echo ""                # Generate password
    echo "https://test.com" # URL
    echo "user@test.com"   # Email
    echo "Test notes"      # Notes
    echo "n"               # No custom fields

    sleep 0.5

    echo "1"               # List entries

    sleep 0.5

    echo "3"               # Add another entry
    echo "Bank Account"
    echo "john.doe"
    echo "MySecurePass123!"
    echo "https://mybank.com"
    echo "john@example.com"
    echo "Main bank account"
    echo "y"               # Add custom fields
    echo "Account Number"
    echo "123456789"
    echo "Branch Code"
    echo "001"
    echo "done"

    sleep 0.5

    echo "1"               # List entries again

    sleep 0.5

    echo "7"               # Generate password
    echo "24"              # Length
    echo "y"               # Uppercase
    echo "y"               # Lowercase
    echo "y"               # Numbers
    echo "y"               # Symbols
    echo "n"               # Don't copy

    sleep 0.5

    echo "8"               # Save and exit
} | ./build/localpdub

echo "Test complete!"