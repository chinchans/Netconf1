# Simple Tracing Client-Server

This is a simplified version that works with OpenSSL 1.1.1f (available in Ubuntu 20.04) without requiring libnetconf2.

## What it does

- **Client**: Generates random TraceID (32 hex chars) and SpanID (16 hex chars) and sends them to the server
- **Server**: Receives the tracing data, processes it, and sends back a confirmation

## Dependencies

Only requires OpenSSL 1.1.1f (which is already in Ubuntu 20.04):

```bash
sudo apt update
sudo apt install -y build-essential libssl-dev
```

## Quick Start

### Step 1: Install dependencies
```bash
make install-deps
```

### Step 2: Build the programs
```bash
make
```

### Step 3: Run the server (in one terminal)
```bash
./simple_server
```

### Step 4: Run the client (in another terminal)
```bash
./simple_client
```

## Example Output

**Server output:**
```
Simple Tracing Server
Starting server on port 8443
OpenSSL initialized
Server listening on port 8443
Press Ctrl+C to stop the server
Client connected from 127.0.0.1:12345
New client connected
Received data (123 bytes):
{
  "type": "tracing_data",
  "traceid": "a1b2c3d4e5f678901234567890123456",
  "spanid": "a1b2c3d4e5f67890",
  "timestamp": 1703123456
}
Received Tracing Data:
  TraceID: a1b2c3d4e5f678901234567890123456
  SpanID:  a1b2c3d4e5f67890
Processing tracing data...
Sent response (156 bytes): {"status": "success", ...}
Client connection closed
```

**Client output:**
```
Simple Tracing Client
Connecting to 127.0.0.1:8443
OpenSSL initialized
Generated tracing data:
Tracing Data:
  TraceID: a1b2c3d4e5f678901234567890123456
  SpanID:  a1b2c3d4e5f67890
Connected to server successfully
Sent 123 bytes to server
Message sent:
{
  "type": "tracing_data",
  "traceid": "a1b2c3d4e5f678901234567890123456",
  "spanid": "a1b2c3d4e5f67890",
  "timestamp": 1703123456
}
Received response (156 bytes):
{
  "status": "success",
  "message": "Tracing data received and processed",
  "received_traceid": "a1b2c3d4e5f678901234567890123456",
  "received_spanid": "a1b2c3d4e5f67890",
  "timestamp": 1703123456
}
Successfully sent tracing data to server
Client completed successfully
```

## Customization

### Change port
```bash
# Server on different port
./simple_server 9000

# Client connecting to different port
./simple_client 127.0.0.1 9000
```

### Test everything at once
```bash
make test
```

## How it works

1. **Client** generates random TraceID and SpanID using OpenSSL's random number generator
2. **Client** sends data as JSON over TCP socket
3. **Server** receives and parses the JSON data
4. **Server** processes the tracing data and sends back a confirmation
5. **Client** receives the confirmation

## Files

- `src/simple_client.c` - Client that sends tracing data
- `src/simple_server.c` - Server that receives and processes tracing data
- `Makefile` - Build configuration
- `README_SIMPLE.md` - This file

## Troubleshooting

1. **"Permission denied"**: Make sure you have build tools installed
2. **"Port already in use"**: Change the port or kill the existing process
3. **"Connection refused"**: Make sure the server is running first

This simplified version avoids the complex NETCONF dependencies and works directly with your existing OpenSSL 1.1.1f installation. 