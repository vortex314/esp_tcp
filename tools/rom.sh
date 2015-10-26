
./esptool.py --port /dev/ttyUSB0 read_flash  0x2000 0x100 out2000.bin
./esptool.py --port /dev/ttyUSB0 read_flash  0x1000 0x100 out1000.bin
echo "===> 0x1000 "
od -t x1 out1000.bin
echo "===> 0x2000 "
od -t x1 out2000.bin
