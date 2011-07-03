OUTPUT_FORMAT("elf32-bfin", "elf32-bfin", "elf32-bfin")
OUTPUT_ARCH(bfin)

MEMORY
{
    ram(rwx)            : ORIGIN = 0x00000000, LENGTH = 0x02000000
    l1code(x)           : ORIGIN = 0xffa00000, LENGTH = 0x10000
    l1data_a(rw)        : ORIGIN = 0xff800000, LENGTH = 0x8000
    l1data_b(rw)        : ORIGIN = 0xff904000, LENGTH = 0x4000
    scratchpad(rw)      : ORIGIN = 0xffb00000, LENGTH = 0x1000
}

SECTIONS
{
    .sdram.text     : { camera.o(.sdram.text) edit.o(.sdram.text) httpd.o(.sdram.text) i2c.o(.sdram.text) stm_m25p32.o(.sdram.text) printf.o(.sdram.text) xmodem.o(.sdram.text) gps.o(.sdram.text) myfunc.o(.sdram.text) *(.gnu.warning) } > ram
    .l1code         : { crt0.o (.text) main.o(.text) math.o(.text) malloc.o(.text) string.o(.text) srv.o(.text) uart.o(.text) colors.o(.text) jpeg.o(.text) r8x8dct.o(.text) motionvect.o(.text) uart.o(.text) setjmp.o(.text) neural.o(.text)} > l1code 
    .l1data         :
    {
        *(.data) *(.l1data)*(.l1data.a)
    } > l1data_a
    .l1data.b       : { *(.l1data.b) } > l1data_b
    .rodata         : { *(.rodata .rodata.* .gnu.linkonce.r.*) } > l1data_a
    .data           :
    {
        *(.data .data.* .gnu.linkonce.d.*) SORT(CONSTRUCTORS)
    } > l1data_a
    .ctors          :
    {
        KEEP (*crtbegin*.o(.ctors))
        KEEP (*(EXCLUDE_FILE (*crtend*.o ) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*(.ctors))
    } > l1data_a
    .dtors          :
    {
        KEEP (*crtbegin*.o(.dtors))
        KEEP (*(EXCLUDE_FILE (*crtend*.o ) .dtors))
        KEEP (*(SORT(.dtors.*)))
        KEEP (*(.dtors))
    } > l1data_a
    /* Special case: Some BSS blocks are too big to fit into L1 RAM.
       See crt0.asm:clear_bss() for BSS initialization.
     */

    .sdram.bss          : {
        __sdram_bss_start = .;
        *(.bss.sdram)
        . = ALIGN(32 / 8);
        __sdram_bss_end = .;
    } > ram

    .bss            :
    {
        /* PROVIDE(__bss_start = .); */
        __bss_start = .;
        *(.dynbss)
        *(.bss .bss.* .gnu.linkonce.b.*)
        *(COMMON)
        . = ALIGN(32 / 8);
        __bss_end = .;
        /* PROVIDE (__bss_end = .); */
    } > l1data_b
    . = 0xff903ffc;
    PROVIDE (_supervisor_stack = .);
}
