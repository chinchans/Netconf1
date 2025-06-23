# O1 Interface with NETCONF Protocol

This project implements an O1 (Open Interface) system that uses NETCONF protocol for communication, specifically designed to work with OpenSSL 1.1.1f on Ubuntu 20.04.

## What is O1 Interface?

O1 (Open Interface) is a standardized interface for network device management that typically uses NETCONF protocol for:
- **Configuration Management**: Setting interface parameters, status, etc.
- **Operational Data**: Retrieving interface statistics, status, etc.
- **Distributed Tracing**: Including TraceID and SpanID for request tracking

## Features

- ✅ **NETCONF Protocol**: Full NETCONF implementation with SSH transport
- ✅ **O1 Interface Management**: Get/set interface configurations
- ✅ **Distributed Tracing**: TraceID and SpanID support
- ✅ **YANG Data Modeling**: Structured data definitions
- ✅ **OpenSSL 1.1.1f Compatible**: Works with Ubuntu 20.04
- ✅ **SSH Authentication**: Secure communication

## Dependencies

### System Dependencies (Ubuntu 20.04)
```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config git
sudo apt install -y libssl-dev libssh-dev
sudo apt install -y libxml2-dev libxslt-dev
sudo apt install -y libpcre3-dev
```

### NETCONF Libraries (Compatible Versions)
The project uses specific versions of libnetconf2 and libyang that work with OpenSSL 1.1.1f:

- **libyang v1.0.225** (compatible with OpenSSL 1.1.1f)
- **libnetconf2 v1.1.54** (compatible with OpenSSL 1.1.1f)

## Installation

### Step 1: Install compatible NETCONF libraries
```bash
chmod +x scripts/install_netconf_compatible.sh
./scripts/install_netconf_compatible.sh
```

### Step 2: Build the O1 interface
```bash
mkdir build_netconf
cd build_netconf
cmake -f ../CMakeLists_netconf.txt ..
make
```

### Step 3: Generate SSH keys (if needed)
```bash
mkdir -p config
ssh-keygen -t rsa -b 2048 -f config/id_rsa -N "" -C "o1-netconf"
chmod 600 config/id_rsa
chmod 644 config/id_rsa.pub
```

## Usage

### Step 1: Start the O1 NETCONF Server
```bash
cd build_netconf
./o1_netconf_server
```

You should see:
```
O1 Interface NETCONF Server
Starting server on port 830
NETCONF initialized for O1 interface server
O1 NETCONF server listening on port 830
Press Ctrl+C to stop the server
```

### Step 2: Run the O1 NETCONF Client
In another terminal:
```bash
cd build_netconf
./o1_netconf_client
```

You should see:
```
O1 Interface NETCONF Client
Connecting to 127.0.0.1:830 as admin
NETCONF initialized for O1 interface
Generated O1 interface data:
O1 Interface Data:
  TraceID:       a1b2c3d4e5f678901234567890123456
  SpanID:        a1b2c3d4e5f67890
  Interface:     eth0
  Operation:     get
  Status:        up
NETCONF session established successfully
O1 get-config sent for interface eth0
Received NETCONF response
O1 edit-config sent for interface eth0
Received NETCONF response
Successfully completed O1 interface operations via NETCONF
O1 NETCONF client completed successfully
```

## O1 Interface Operations

### 1. Get Configuration (get-config)
Retrieves the current configuration of an O1 interface:

```xml
<rpc xmlns="urn:ietf:params:xml:ns:netconf:base:1.0" message-id="1">
  <get-config>
    <source>
      <running/>
    </source>
    <filter type="subtree">
      <o1-interface xmlns="urn:example:o1-interface">
        <name>eth0</name>
      </o1-interface>
    </filter>
  </get-config>
</rpc>
```

### 2. Edit Configuration (edit-config)
Sets the configuration of an O1 interface with tracing data:

```xml
<rpc xmlns="urn:ietf:params:xml:ns:netconf:base:1.0" message-id="2">
  <edit-config>
    <target>
      <running/>
    </target>
    <config>
      <o1-interface xmlns="urn:example:o1-interface">
        <name>eth0</name>
        <status>up</status>
        <tracing>
          <traceid>a1b2c3d4e5f678901234567890123456</traceid>
          <spanid>a1b2c3d4e5f67890</spanid>
        </tracing>
      </o1-interface>
    </config>
  </edit-config>
</rpc>
```

## YANG Model

The project includes a comprehensive YANG model (`config/o1-interface.yang`) that defines:

- **Interface Configuration**: Name, status, description
- **Tracing Data**: TraceID, SpanID, timestamp
- **Statistics**: Packets in/out, bytes in/out
- **RPC Operations**: get-interface-status, set-interface-status

## Customization

### Change Interface Name
```bash
# Modify the interface name in the source code
# Default is "eth0", you can change it to any interface name
```

### Change Port
```bash
# Server on different port
./o1_netconf_server 9000

# Client connecting to different port
./o1_netconf_client 127.0.0.1 9000
```

### Change Authentication
```bash
# Use different username/password
./o1_netconf_client 127.0.0.1 830 myuser mypassword
```

## Troubleshooting

### 1. "libnetconf2 not found"
```bash
# Make sure you ran the installation script
./scripts/install_netconf_compatible.sh
```

### 2. "OpenSSL version conflict"
```bash
# The compatible versions should work with OpenSSL 1.1.1f
# If still having issues, check your OpenSSL version:
openssl version
```

### 3. "SSH connection failed"
```bash
# Generate SSH keys
ssh-keygen -t rsa -b 2048 -f config/id_rsa -N ""
```

### 4. "Port 830 already in use"
```bash
# Kill existing process or use different port
sudo netstat -tlnp | grep 830
sudo kill <PID>
```

## Architecture

```
┌─────────────────┐    NETCONF/SSH    ┌─────────────────┐
│   O1 Client     │◄─────────────────►│   O1 Server     │
│                 │                   │                 │
│ • Generate      │                   │ • Accept        │
│   TraceID/SpanID│                   │   connections   │
│ • Send get-config│                  │ • Process       │
│ • Send edit-config│                 │   NETCONF msgs  │
│ • Parse responses│                  │ • Send replies  │
└─────────────────┘                   └─────────────────┘
```

## Files Structure

```
├── src/
│   ├── o1_netconf_client.c    # O1 NETCONF client
│   ├── o1_netconf_server.c    # O1 NETCONF server
│   └── ...
├── config/
│   ├── o1-interface.yang      # YANG data model
│   └── ...
├── scripts/
│   └── install_netconf_compatible.sh  # Installation script
├── CMakeLists_netconf.txt     # Build configuration
└── README_O1_NETCONF.md       # This file
```

This implementation provides a complete O1 interface solution using NETCONF protocol, compatible with your existing OpenSSL 1.1.1f installation on Ubuntu 20.04. 