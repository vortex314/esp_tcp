pwd
# mv minicom1.log minicom2.log
# mv minicom.log minicom1.log
USB=$2
TTY=/dev/tty$USB
LOG=$USB.log
FLASH="./esptool.py --p $TTY -b 576000 write_flash  -ff 40m -fm dio -fs 8m "
rm $LOG
PROJECT=$1
echo "---------------" $PROJECT "------------------------"
./reset $TTY
../tools/esptool.py --port $TTY  read_mac
../tools/esptool.py --port $TTY  read_flash  0x3F8000 0x100 dump.bin 
od --endian=little -X -c dump.bin > $LOG
# ../tools/esptool.py --port $TTY  erase_flash
../tools/esptool.py elf2image ../Debug/$PROJECT
./esptool2 -debug -bin -boot2 -1024 -dio -40 ../Debug/$PROJECT $PROJECT.bin .text .data .rodata # was 4096
./esptool.py --p $TTY -b 576000 write_flash  -ff 40m -fm dio -fs 32m \
	0x00000 rboot.bin 0x02000 $PROJECT.bin \
    0x3FC000 esp_init_data_default.bin 0x3FE000 blank.bin  
#    0x3F8000 blank4.bin \
# 	0x7C000 esp_init_data_default.bin 0x7E000 blank.bin \
#	0xFC000 esp_init_data_default.bin 0xFE000 blank.bin \
# $FLASH 0x00000 ../Debug/esp_tcp-0x00000.bin  0x02010 ../Debug/esp_tcp-0x02010.bin 0xFC000 esp_init_data_default.bin 0xFE000 blank.bin 
# $FLASH 0x02010 ../Debug/esp_tcp-0x02010.bin 
# $FLASH 0xFC000 esp_init_data_default.bin 
# $FLASH 0xFE000 blank.bin 
# $FLASH 0x7C000 esp_init_data_default.bin 
# $FLASH 0x7E000 blank.bin 
 # was 32m, # esp_init_data_default.bin, was 7C and 7E 8m 
./reset $TTY
minicom  -D $TTY -C $LOG
# 
