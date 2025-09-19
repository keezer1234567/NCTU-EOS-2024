#!/bin/sh

set -x #prints each command and its arguments to the terminal before executing it
# set -e #Exit immediately if a command exits with a non-zero status

rmmod -f my_dev
insmod my_dev.ko

./writer CHENHUNGXIANG & #run in subshell
./reader 192.168.50.147 8888 /dev/my_dev
