#!/bin/bash

# ITASK: Your code here

# Create the virtual bridge and name it br0 and bring the interface up
sudo ip link add br0 type bridge

# Create the tap and name it tap0
# sudo ip tuntap add dev tap0 mode tap
sudo ip tuntap add dev tap0 mode tap user "$USER"

# Bring up the interface in promiscuous mode
sudo ip link set tap0 up promisc on

# Make tap0 a slave of br0
sudo ip link set tap0 master br0

# Give bridge br0 an IP address of 192.168.123.1
sudo ip addr add 192.168.123.1/24 broadcast 192.168.123.255 dev br0

# Make sure everything is up
sudo ip link set dev br0 up
sudo ip link set dev tap0 up

#sudo qemu-system-x86_64 -boot d -m 4G \
#	-device e1000,netdev=network0,mac=52:54:00:8c:d0:7e \
#	-netdev tap,id=network0,ifname=tap0,script=no,downscript=no \
#	-cdrom archlinux-2021.12.01-x86_64.iso

