CROSS = bfin-elf-
CC = $(CROSS)gcc
LDR = $(CROSS)ldr
OBJCOPY = $(CROSS)objcopy
CPU = bf548
CFLAGS ?= -g -O2
CFLAGS += -mcpu=$(CPU)
CFLAGS += -Wall
CFLAGS += -Wl,-Map,blink.map

all: blink.ldr

blink.ldr: blink
	$(LDR) -f -T $(CPU) -c $@ $<

blink.bin:
	$(OBJCOPY) -Obinary blink blink.bin

load: blink.ldr
	$(LDR) -l $< /dev/ttyS0

clean:
	rm -f *.o blink blink.ldr blink.map blink.bin

.PHONY: all clean load
