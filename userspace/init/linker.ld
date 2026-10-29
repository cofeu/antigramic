ENTRY(_start)
SECTIONS
{
    . = 0x400000;
    .text : { *(.text*) }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss : {
        *(.bss*)
        *(COMMON)
        _heap_start = .;
        . += 16M;
        _heap_end = .;
    }
}