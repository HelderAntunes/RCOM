#!/bin/bash

ifconfig eth0 up
ifconfig eth0 172.16.30.254/24
ifconfig eth2 up
ifconfig eth2 172.16.31.253/24
echo 1 > /proc/sys/net/ipv4/ip_forward
echo 0 > /proc/sys/net/ipv4/icmp_echo_ignore_broadcasts
route add default gw 172.16.31.254