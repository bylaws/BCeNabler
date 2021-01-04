#!/bin/sh

aarch64-linux-gnu-gcc -c patch.s -o patch.o
aarch64-linux-gnu-objcopy -O binary -j .text patch.o patch.bin

# Must be smaller than a single page
bin2c -o patch_code.h -n PatchRawData patch.bin

rm patch.o
rm patch.bin
