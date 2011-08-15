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

TARGETS = patch

SRCS = $(patsubst %,%.c,$(TARGETS))
ELFS = $(SRCS:.c=.elf)
LDRS = $(SRCS:.c=.ldr)
MAPS = $(SRCS:.c=.map)
BINS = $(SRCS:.c=.bin)
OBJS = $(SRCS:.c=.o)

all: $(OBJS)

$(patsubst %,%.elf,$(TARGETS)): $(OBJS)
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

%.ldr: %.elf
	$(LDR) -f -T $(CPU) -c $@ $<

%.bin: %.elf
	$(OBJCOPY) -Obinary $< $@

load: $(patsubst %,%.ldr,$(TARGETS))
	$(LDR) -l $< /dev/ttyS0

clean:
	rm -f $(ELFS) $(LDRS) $(MAPS) $(BINS) $(OBJS)

.PHONY: all clean load
