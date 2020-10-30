#!/bin/bash

PCI_ID=$(lspci | grep -i xilinx | awk '{print $1}')

echo $PCI_ID

echo /sys/bus/pci/devices/0000\:$PCI_ID/remove

echo 1 > /sys/bus/pci/devices/0000\:$PCI_ID/remove
echo 1 > /sys/bus/pci/rescan
