/*
 * Linker Script - STM32F446RE (Nucleo-F446RE)
 * Cortex-M4F, 512K Flash, 128K SRAM
 *
 * Base addresses per RM0390 memory map.
 */

ENTRY(Reset_Handler)

/* Top of stack = end of RAM (stack grows downward from here) */
_estack = ORIGIN(RAM) + LENGTH(RAM);

/* Minimum reserved space for heap and stack. The ._user_heap_stack
 * section below forces the linker to fail at link time if .data + .bss
 * + these reservations don't fit in RAM, instead of silently linking
 * and corrupting memory at runtime. Adjust these if you know your
 * actual heap/stack needs. */
_Min_Heap_Size  = 0x200;  /* 512 bytes */
_Min_Stack_Size = 0x400;  /* 1 KB */

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

SECTIONS
{
  /* Vector table must be the very first thing in flash - the CPU
   * hardware reads word 0 (initial SP) and word 1 (reset handler
   * address) directly from here on reset, before any code runs. */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } > FLASH

  /* Program code */
  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    KEEP(*(.init))
    KEEP(*(.fini))
    . = ALIGN(4);
    _etext = .;
  } > FLASH

  /* const data - read-only, stays in flash */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } > FLASH

  /* Stack unwind info - harmless to include, some libc calls expect it */
  .ARM.extab :
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } > FLASH

  .ARM :
  {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } > FLASH

  /* Initialized globals: runtime address in RAM (so they're writable),
   * but initial values are stored in flash (RAM is garbage on power-up).*/
  _sidata = LOADADDR(.data);

  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } > RAM AT> FLASH

  /* Zero-initialized globals: no flash storage needed, just a RAM range
   * the startup file memsets to zero using [_sbss, _ebss) before main(). */
  .bss :
  {
    . = ALIGN(4);
    _sbss = .;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
  } > RAM

  /* Reserves room for heap + stack and makes the linker error out if
   * .data + .bss have left less than this much RAM free. */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE(end = .);
    PROVIDE(_end = .);
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } > RAM
}
