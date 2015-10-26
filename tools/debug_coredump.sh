#!/bin/bash
#
# This tool offerts a dead simple way to doing post mortem analysis
# of an ESP8266 crash.
#
# All you need is a file containing a dump of the serial log
# containing the core dump snippet:
#
# -------- Core Dump --------
# [very long line]
# -------- End Core Dump --------
#
# And run:
#
#     tools/debug_coredump.sh <my_log_file>
#
# OSX and Windows users: the log file should be under your Home directory
# otherwise docker-machine (or boot2docker) won't make it available to your
# docker VM.

LOG=minicom.log
BIN=esp_gtw

if [ -z "${LOG}" ]; then
    echo "usage: $0 console_log_file"
fi

# run core server in background and connect xt-gdb to it
# exec docker run --rm -it -v /${V7DIR}:/cesanta -v ${LOG}:/var/log/esp-console.log ${SDK} /bin/bash -c 

./serve_core.py  --irom ../Debug/${BIN}-0x02010.bin --iram ../Debug/${BIN}-0x00000.bin --rom rom.bin ${LOG}  
#/home/lieven/esp-open-sdk/xtensa-lx106-elf/bin/xtensa-lx106-elf-gdb ../Debug/esp_gtw -ex 'target remote 127.0.0.1:1234' -ex 'set confirm off' -ex 'add-symbol-file tools/romsyms 0x40000000'
