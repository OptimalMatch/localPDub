#!/bin/bash

# Setup two test vaults with different entries for sync testing

echo "Setting up test vaults for sync testing..."

# Clean up old test vaults
rm -rf /tmp/localpdub_vault1 /tmp/localpdub_vault2

# Create directories
mkdir -p /tmp/localpdub_vault1/.localpdub
mkdir -p /tmp/localpdub_vault2/.localpdub

# Create Vault 1 with some entries
echo "Creating Vault 1 with test entries..."
HOME=/tmp/localpdub_vault1 ./build/localpdub <<EOF
testtest123
testtest123
3
GitHub Account
john.doe
MySecurePass123!
https://github.com
john@example.com
Personal GitHub account
n
3
Gmail Account
john.doe@gmail.com
GmailPass456#
https://gmail.com
john.doe@gmail.com
Primary email account
n
3
Vault1-Only Entry
vault1user
vault1password
https://vault1.com
vault1@test.com
This entry exists only in vault 1
n
9
EOF

echo ""
echo "Creating Vault 2 with different entries..."
HOME=/tmp/localpdub_vault2 ./build/localpdub <<EOF
testtest123
testtest123
3
GitHub Account
john.doe
DifferentGitHubPass789!
https://github.com
john@example.com
Personal GitHub account - updated password
n
3
Twitter Account
@johndoe
TwitterPass321
https://twitter.com
john@twitter.com
Twitter social media
n
3
Vault2-Only Entry
vault2user
vault2password
https://vault2.com
vault2@test.com
This entry exists only in vault 2
n
9
EOF

echo ""
echo "Test vaults created successfully!"
echo ""
echo "Vault 1 contents:"
HOME=/tmp/localpdub_vault1 ./build/localpdub <<EOF | grep -A 30 "Password Entries"
testtest123
1
9
EOF

echo ""
echo "Vault 2 contents:"
HOME=/tmp/localpdub_vault2 ./build/localpdub <<EOF | grep -A 30 "Password Entries"
testtest123
1
9
EOF

echo ""
echo "════════════════════════════════════════════════════"
echo "Test vaults ready for sync testing!"
echo "════════════════════════════════════════════════════"
echo ""
echo "To test sync, run in two separate terminals:"
echo ""
echo "Terminal 1:"
echo "  HOME=/tmp/localpdub_vault1 ./build/localpdub"
echo "  Password: testtest123"
echo "  Select option 8 for sync"
echo ""
echo "Terminal 2:"
echo "  HOME=/tmp/localpdub_vault2 ./build/localpdub"
echo "  Password: testtest123"
echo "  Select option 8 for sync"
echo ""
echo "Expected behavior:"
echo "  - Vault 1 should discover Vault 2 and vice versa"
echo "  - GitHub Account will have a conflict (different passwords)"
echo "  - Gmail Account will sync to Vault 2"
echo "  - Twitter Account will sync to Vault 1"
echo "  - Vault-specific entries will sync to the other vault"
echo ""