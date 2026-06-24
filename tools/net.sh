#!/usr/bin/env bash

# Initialize Host Side Networking for Linux/WSL

sudo ip tuntap add dev tap0 mode tap 2>/dev/null || true
sudo ip addr add 192.168.100.1/24 dev tap0 2>/dev/null || true
sudo ip link set tap0 up
sudo sysctl -w net.ipv4.ip_forward=1
sudo iptables -t nat -A POSTROUTING -s 192.168.100.0/24 -j MASQUERADE

