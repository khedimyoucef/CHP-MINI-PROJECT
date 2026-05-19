#!/bin/bash
set -e

echo "=== 1. Setting up SSH Keys ==="
if [ ! -f ~/.ssh/id_rsa ]; then
    ssh-keygen -t rsa -N "" -f ~/.ssh/id_rsa
fi
cat ~/.ssh/id_rsa.pub >> ~/.ssh/authorized_keys
chmod 600 ~/.ssh/authorized_keys

echo "=== 2. Configuring LTSP ==="
# Ensure IP is set correctly in INI format
sudo sh -c 'echo "[server]" > /etc/ltsp/ltsp.conf'
sudo sh -c 'echo "IP=\"192.168.56.1\"" >> /etc/ltsp/ltsp.conf'

# Generate dnsmasq configuration for LTSP
sudo ltsp dnsmasq -d0

# Start and enable dnsmasq
sudo systemctl restart dnsmasq
sudo systemctl enable dnsmasq

echo "=== 3. Building LTSP Network Image (This will take 10-15 minutes) ==="
sudo ltsp image /

echo "=== 4. Finalizing PXE Boot ==="
sudo ltsp initrd
sudo ltsp ipxe

echo "=== LTSP Setup Complete! You can now boot slave1 and slave2 ==="
