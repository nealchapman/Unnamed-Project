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

SUBDIRS = Engine Hardware/BF548 Hardware/BF548EZ-KIT_LITE Modules Util Patch
LIBS += Engine/engine.a Hardware/dsp.a Hardware/board.a Modules/modules.a Util/util.a Patch/patch.a

#LIBS += $(patsubst %,-L%,$(SUBDIRS))

TARGET = synth
SRCS = $(patsubst %,%.c,$(TARGET))
ELFS = $(SRCS:.c=.elf)
LDRS = $(SRCS:.c=.ldr)
MAPS = $(SRCS:.c=.map)
BINS = $(SRCS:.c=.bin)
OBJS = $(SRCS:.c=.o)

OBJS = $(SRCS:.c=.o)

.PHONY: subdirs $(SUBDIRS)

all: subdirs $(LDRS)

subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ all

$(TARGET).elf: $(OBJS)
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

%.ldr: %.elf
	$(LDR) -f -T $(CPU) -c $@ $<

%.bin: %.elf
	$(OBJCOPY) -Obinary $< $@

load: $(TARGET).ldr
	$(LDR) -l $< /dev/ttyS0

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
	rm -f $(TARGET).elf $(TARGET).ldr $(TARGET).map $(TARGET).bin $(OBJS)

.PHONY: all clean load
