#!/bin/bash

# MAC address của Raspberry Pi (Windows hiển thị với dấu gạch ngang)
MAC="b8-27-eb-db-99-c4"

# Gọi lệnh arp -a của Windows từ WSL
OUTPUT=$(powershell.exe -Command "arp -a")

# Tìm dòng chứa MAC
LINE=$(echo "$OUTPUT" | grep -i $MAC)

if [ -n "$LINE" ]; then
    # Cột đầu tiên là IP
    IP=$(echo $LINE | awk '{print $1}')
    echo "Found Raspberry Pi at $IP"
    # SSH vào Raspberry Pi
    ssh root@$IP
else
    echo "Raspberry Pi not found in ARP table"
fi

