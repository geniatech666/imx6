#!/bin/sh

sleep 3

WIFI_MODULE_PATH=$(find /lib/modules -name "bcmdhd.ko")
rk_wifi_init /dev/ttymxc2  $WIFI_MODULE_PATH bootmod=auto

sleep 2
rfkill unblock all
sleep 2
ifconfig wlan0 up
hciconfig hci0 up

: exit 0
