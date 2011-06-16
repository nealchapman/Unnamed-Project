CROSS = bfin-elf-
CC = $(CROSS)gcc
LDR = $(CROSS)ldr
OBJCOPY = $(CROSS)objcopy
CPU = bf548
CFLAGS = -g -O2
CFLAGS += -mcpu=$(CPU)
CFLAGS += -Wall
CFLAGS += -Wl,-Map,Audio_test.map
LIBS = -lm

all: Audio_test.ldr 


Audio_test: Audio_test.c
	$(CC) -g -O2 $(CFLAGS) $< $(LIBS) -o $@

Audio_test.ldr: Audio_test
	$(LDR) -f -T $(CPU) -c $@ $<

Audio_test.bin:
	$(OBJCOPY) -Obinary Audio_test Audio_test.bin

load: Audio_test.ldr
	$(LDR) -l $< /dev/ttyS0

clean:
	rm -f *.o Audio_test Audio_test.ldr Audio_test.map Audio_test.bin

.PHONY: all clean load
