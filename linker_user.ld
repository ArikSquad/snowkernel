ENTRY(_start)
SECTIONS
{
  . = 0x00400000;
  .text : { *(.text*) *(.rodata*) }
  .data : { *(.data*) }
  .bss  : { *(.bss*) *(COMMON) }
  /DISCARD/ : { *(.eh_frame*) *(.comment*) }
}
