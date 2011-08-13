CROSS = bfin-elf-
CC = $(CROSS)gcc
LD = $(CROSS)gcc
LDR = $(CROSS)ldr
OBJCOPY = $(CROSS)objcopy
CPU = bf548
CFLAGS = -g -O2
CFLAGS += -mcpu=$(CPU)
CFLAGS += -Wall
CFLAGS += -D DO_CYCLE_COUNTS
LDFLAGS = -mcpu=$(CPU) -Wl,-Map,$(TARGET).map
LIBS = -lm -lbfdsp
#LIBS = -lm /opt/uClinux/bfin-elf/bfin-elf/lib/libbfdsp.a

SUBDIRS = dir1 dir2

TARGET = source

SRCS = $(TARGET).c

OBJS = $(SRCS:.c=.o)

all: $(TARGET).ldr
	$(SUBDIRS)

$(TARGET).elf: $(OBJS)
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

%.ldr: %.elf
	$(LDR) -f -T $(CPU) -c $@ $<

%.bin: %.elf
	$(OBJCOPY) -Obinary $< $@

load: $(TARGET).ldr
	$(LDR) -l $< /dev/ttyS0

clean:
	rm -f $(TARGET).elf $(TARGET).ldr $(TARGET).map $(TARGET).bin $(OBJS)

$(SUBDIRS):
	$(MAKE) -C $@

source:dir1
source:dir2

.PHONY: all clean load subdirs $(SUBDIRS)
