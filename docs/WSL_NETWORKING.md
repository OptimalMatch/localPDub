# WSL2 Networking Guide for LocalPDub

## The Problem
WSL2 uses a virtual network with NAT, which prevents UDP broadcast discovery from working between WSL2 and machines on your physical network.

- WSL2 typically uses subnet: `172.x.x.x/20`
- Physical network uses: `192.168.x.x/24`

## Solutions

### Option 1: Port Forwarding (Quick Fix)
Forward ports from Windows host to WSL2:

```powershell
# Run as Administrator in PowerShell
$wslIP = wsl hostname -I | %{ $_.Split(' ')[0] }

# Forward discovery ports (UDP 51820-51829)
for ($i=51820; $i -le 51829; $i++) {
    netsh interface portproxy add v4tov4 listenport=$i listenaddress=0.0.0.0 connectport=$i connectaddress=$wslIP
}
```

### Option 2: Bridge Mode (Permanent Solution)
1. Create `C:\Users\%USERNAME%\.wslconfig`:
```ini
[wsl2]
networkingMode=bridged
vmSwitch=WSLBridge
dhcp=true
```

2. Create Hyper-V External Switch:
   - Open Hyper-V Manager
   - Actions → Virtual Switch Manager
   - Create External switch named "WSLBridge"
   - Connect to your physical network adapter

3. Restart WSL:
```powershell
wsl --shutdown
```

### Option 3: WSL1 (Native Networking)
Convert your distro to WSL1 for native networking:
```powershell
wsl --set-version Ubuntu 1
```

### Option 4: Direct IP Connection (Feature Request)
We could add a feature to specify IP directly:
```bash
localpdub sync --connect 192.168.5.51:51820
```

## Checking Your Network
```bash
# In WSL2
ip addr show eth0

# In Windows
ipconfig /all

# Test connectivity
ping -c 1 192.168.5.51  # Will likely fail from WSL2
```

## Workaround for Now
Run LocalPDub on Windows directly instead of in WSL2, or use WSL1 which shares the host network.