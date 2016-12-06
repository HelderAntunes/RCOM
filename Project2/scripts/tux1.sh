#!/bin/bash

ifconfig eth0 up
ifconfig eth0 172.16.30.1/24
echo 0 > /proc/sys/net/ipv4/icmp_echo_ignore_broadcasts
route add -net 172.16.31.0/24 gw 172.16.30.254
route add default gw 172.16.30.254
