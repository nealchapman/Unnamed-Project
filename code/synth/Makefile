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

TARGET = synth

SRCS = $(TARGET).c

OBJS = $(SRCS:.c=.o)

#all: $(TARGET).ldr
all:
	cd Engine && make clean all
	cd Hardware/BF548 && make clean all
	cd Hardware/BF548EZ-KIT_LITE && make clean all
	cd Modules && make clean all
	cd Util && make clean all
	cd Patch && make clean all
	make $(TARGET).ldr
	
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

.PHONY: all clean load
