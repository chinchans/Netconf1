# NETCONF Client-Server with TraceID and SpanID

This project implements a NETCONF client and server that can exchange traceid and spanid character arrays for distributed tracing.

## Dependencies

### System Dependencies (Ubuntu 20.04)
```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config
sudo apt install -y libssl-dev libssh-dev
sudo apt install -y libxml2-dev libxslt-dev
sudo apt install -y libnetconf2-dev libnetconf2
sudo apt install -y libyang-dev
```

### Manual Installation of libnetconf2 (if not available in apt)
```bash
# Install libyang first
git clone https://github.com/CESNET/libyang.git
cd libyang
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install

# Install libnetconf2
git clone https://github.com/CESNET/libnetconf2.git
cd libnetconf2
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
```

## Building the Project
```bash
mkdir build
cd build
cmake ..
make
```

## Running the Application

1. Start the server:
```bash
./netconf_server
```

2. In another terminal, run the client:
```bash
./netconf_client
```

## Features
- NETCONF over SSH communication
- Sends traceid and spanid as character arrays
- Supports basic NETCONF operations (get-config, edit-config)
- Includes proper error handling and logging

## Security Notes
- The server uses a self-signed certificate for demonstration
- In production, use proper certificates and authentication
- Default username: admin, password: admin123 