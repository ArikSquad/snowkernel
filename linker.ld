ENTRY(_start)
SECTIONS
{
  . = 1M;

  /* be sure that multiboot header is within the first 8KB of the file */
  .multiboot :
  {
    KEEP(*(.multiboot))
  }

  .text : ALIGN(16)
  {
    KEEP(*(.text.boot))
    *(.text*)
  }

  .rodata : ALIGN(16) { *(.rodata*) }
  .data : ALIGN(16) { *(.data*) }
  .bss : ALIGN(16) {
    __bss_start = .;
    *(COMMON) *(.bss*)
    __bss_end = .;
  }
}
