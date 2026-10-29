ENTRY(efi_main)
SECTIONS
{
    . = 0x100000;
    .text : { *(.text) }
    .data : { *(.data) }
    .rodata : { *(.rodata) }
    .bss : { *(.bss) }
    /DISCARD/ : { *(.eh_frame) *(.comment) }
}