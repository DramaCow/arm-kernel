#!/bin/bash
od -A d -t $1 -j $(($2*512)) -N 512 device/disk.bin | head
