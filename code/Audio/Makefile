CROSS = bfin-elf-
CC = $(CROSS)gcc
LD = $(CROSS)gcc
LDR = $(CROSS)ldr
OBJCOPY = $(CROSS)objcopy
CPU = bf548
CFLAGS = -g #-O2
CFLAGS += -mcpu=$(CPU)
CFLAGS += -Wall
LDFLAGS = -mcpu=$(CPU) -Wl,-Map,Audio_test.map
LIBS = -lm

TARGET = Audio_test

SRCS = Audio_test.c

OBJS = $(SRCS:.c=.o)

all: $(TARGET).ldr

$(TARGET).elf: $(OBJS)
	$(LD) $(LDFLAGS) $< $(LIBS) -o $@

%.ldr: %.elf
	$(LDR) -f -T $(CPU) -c $@ $<

%.bin: %.elf
	$(OBJCOPY) -Obinary $< $@

load: $(TARGET).ldr
	$(LDR) -l $< /dev/ttyS0

clean:
	rm -f $(TARGET).elf $(TARGET).ldr $(TARGET).map $(TARGET).bin $(OBJS)

.PHONY: all clean load
