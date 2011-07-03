CSRCS   = main.c init.c srv.c uart.c camera.c jpeg.c i2c.c stm_m25p32.c httpd.c\
  xmodem.c colors.c edit.c malloc.c string.c printf.c myfunc.c neural.c gps.c sdcard.c \
  picoc/picoc.c picoc/table.c picoc/lex.c picoc/parse.c picoc/expression.c \
  picoc/heap.c picoc/type.c picoc/variable.c picoc/clibrary.c \
  picoc/platform/library_surveyor.c picoc/platform.c picoc/platform/platform_surveyor.c \
  picoc/include.c 
 
ASMSRCS = r8x8dct.asm math.asm setjmp.asm motionvect.asm crt0.asm

CPUDEFINES = -D__ADSPBF537__ -D__ADSPLPBLACKFIN__ -DSURVEYOR_HOST
INCLUDES = -I../include
ASMFLAGS = -x assembler-with-cpp $(CPUDEFINES) $(INCLUDES)
CFLAGS = $(INCLUDES) -g -O2 -Wall -fomit-frame-pointer -fno-builtin-printf -fno-builtin-puts \
	-fno-builtin-strchr -fno-builtin-strcpy -fno-builtin-strlen -fno-builtin-strdup \
	-fno-builtin-memset -fno-builtin-memcpy -fno-builtin-isdigit -fno-builtin-cos -fno-builtin-sin \
	-fno-builtin-tan -fno-builtin-acos -fno-builtin-asin -fno-builtin-atan -mfast-fp
CFLAGS += $(CPUDEFINES)

OBJS = $(CSRCS:%.c=%.o) $(ASMSRCS:%.asm=%.o)

AS = bfin-elf-as
CC = bfin-elf-gcc
LD = bfin-elf-ld
LDR = bfin-elf-ldr

all:  srv1.bin srv1.ldr srv1_115k.bin srv1_115k.ldr

# Special rules for 115K baud version of main.c (main.c will default to 2304000 baud)
main_115k.o: main.c
	$(CC) -D__SRV_UART0_BAUDRATE_115200 -c -o main_115k.o main.c $(CFLAGS)
	mv main_115k.o main.o

%.o: %.asm
	$(CC) $(ASMFLAGS) -c -o $@ $<

srv1.bin: $(OBJS) main.o
	$(LD) -T srv1.x -Map srv1.map -O binary -o srv1.bin $(OBJS) -lbffastfp

srv1_115k.bin: $(OBJS) main_115k.o
	$(LD) -T srv1.x -Map srv1_115k.map -O binary -o srv1_115k.bin $(OBJS) -lbffastfp

srv1.ldr: srv1.bin init.o
	$(LDR) -f -T BF537 -c srv1.ldr --initcode init.o srv1.bin
    
srv1_115k.ldr: srv1.bin init.o
	$(LDR) -f -T BF537 -c srv1_115k.ldr --initcode init.o srv1_115k.bin

clean:
	rm -f *.o *.s *.bin *.ldr picoc/*.o picoc/platform/*.o

