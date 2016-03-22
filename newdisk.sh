dd of=disk.bin if=/dev/zero count=1048576 bs=1
rm device/disk.bin
mv disk.bin device
