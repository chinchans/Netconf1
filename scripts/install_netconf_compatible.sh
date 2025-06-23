#!/bin/bash

# Install compatible NETCONF libraries for Ubuntu 20.04 with OpenSSL 1.1.1f
set -e

echo "Installing compatible NETCONF libraries for O1 interface..."

# Update package list
sudo apt update

# Install basic dependencies
sudo apt install -y build-essential cmake pkg-config git
sudo apt install -y libssl-dev libssh-dev
sudo apt install -y libxml2-dev libxslt-dev
sudo apt install -y libpcre3-dev

# Install libyang 1.0.x (compatible with OpenSSL 1.1.1f)
echo "Installing libyang 1.0.x..."
if [ ! -d "libyang" ]; then
    git clone https://github.com/CESNET/libyang.git
fi
cd libyang
git checkout v1.0.225  # Use a version compatible with OpenSSL 1.1.1f
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_BUILD_TESTS=OFF ..
make -j$(nproc)
sudo make install
cd ../..

# Install libnetconf2 1.1.x (compatible with OpenSSL 1.1.1f)
echo "Installing libnetconf2 1.1.x..."
if [ ! -d "libnetconf2" ]; then
    git clone https://github.com/CESNET/libnetconf2.git
fi
cd libnetconf2
git checkout v1.1.54  # Use a version compatible with OpenSSL 1.1.1f
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_BUILD_TESTS=OFF -DENABLE_SSH=ON -DENABLE_TLS=ON ..
make -j$(nproc)
sudo make install
cd ../..

# Update library cache
sudo ldconfig

echo "NETCONF libraries installed successfully!"
echo "You can now build the O1 interface with NETCONF support." 