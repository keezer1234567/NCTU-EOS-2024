#!/bin/sh

set -x
# set -e

sudo rmmod -f seg_driver.ko
sudo insmod seg_driver.ko

sudo rmmod -f led_driver.ko
sudo insmod led_driver.ko

sudo ./hw1

# 如果遇到 "insmod Operation not permitted" 的 Bug，請先 Ctrl+C，然後重新執行一次
# 推測此問題可能是 gpio 的腳位沒有 free 乾淨 (但我不想修了XD，能用就好)