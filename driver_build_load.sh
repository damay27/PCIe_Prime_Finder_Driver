#!/bin/bash

DRIVER_NAME=prime_finder
DEVICE_FILE_NAME=prime_finder

./device_refresh.sh

rmmod $DRIVER_NAME
make
insmod $DRIVER_NAME.ko

MAJOR_NUMBER=`cat /proc/devices | grep $DRIVER_NAME | awk '{print $1}'`

mknod /dev/$DEVICE_FILE_NAME c $MAJOR_NUMBER 0
