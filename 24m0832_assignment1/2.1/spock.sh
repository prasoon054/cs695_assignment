#!/bin/bash

echo "Compiling kernel module..."
make

echo "Loading kernel module..."
sudo insmod ioctl_driver.ko

echo "Creating device file..."
sudo mknod /dev/ioctl_device c $(awk "\$2==\"ioctl_device\" {print \$1}" /proc/devices) 0

echo "Compiling user-space application..."
gcc -o ioctl_test ioctl_test.c

echo "Running user-space application..."
sudo ./ioctl_test

echo "Removing device file and kernel module..."
sudo rm /dev/ioctl_device
sudo rmmod ioctl_driver

make clean
echo "Done!"
