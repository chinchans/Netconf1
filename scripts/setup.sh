#!/bin/bash

# NETCONF Client-Server Setup Script
# For Ubuntu 20.04

set -e

echo "Setting up NETCONF Client-Server environment..."

# Update package list
echo "Updating package list..."
sudo apt update

# Install basic build tools
echo "Installing build tools..."
sudo apt install -y build-essential cmake pkg-config git

# Install OpenSSL and SSH development libraries
echo "Installing OpenSSL and SSH libraries..."
sudo apt install -y libssl-dev libssh-dev

# Install XML and XSLT libraries
echo "Installing XML and XSLT libraries..."
sudo apt install -y libxml2-dev libxslt-dev

# Install libyang (required for libnetconf2)
echo "Installing libyang..."
sudo apt install -y libyang-dev

# Try to install libnetconf2 from package manager
echo "Attempting to install libnetconf2 from package manager..."
if ! sudo apt install -y libnetconf2-dev libnetconf2; then
    echo "libnetconf2 not available in package manager, building from source..."
    
    # Build libyang from source if needed
    if [ ! -d "libyang" ]; then
        echo "Building libyang from source..."
        git clone https://github.com/CESNET/libyang.git
        cd libyang
        mkdir build && cd build
        cmake ..
        make -j$(nproc)
        sudo make install
        cd ../..
    fi
    
    # Build libnetconf2 from source
    if [ ! -d "libnetconf2" ]; then
        echo "Building libnetconf2 from source..."
        git clone https://github.com/CESNET/libnetconf2.git
        cd libnetconf2
        mkdir build && cd build
        cmake ..
        make -j$(nproc)
        sudo make install
        cd ../..
    fi
fi

# Update library cache
echo "Updating library cache..."
sudo ldconfig

# Create config directory if it doesn't exist
mkdir -p config

# Generate SSH keys for testing (if they don't exist)
if [ ! -f "config/id_rsa" ]; then
    echo "Generating SSH keys for testing..."
    ssh-keygen -t rsa -b 2048 -f config/id_rsa -N "" -C "netconf-test"
    chmod 600 config/id_rsa
    chmod 644 config/id_rsa.pub
fi

echo "Setup completed successfully!"
echo ""
echo "To build the project:"
echo "  mkdir build"
echo "  cd build"
echo "  cmake .."
echo "  make"
echo ""
echo "To run the server:"
echo "  ./netconf_server"
echo ""
echo "To run the client (in another terminal):"
echo "  ./netconf_client" 