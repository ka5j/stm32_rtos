/*
 * STM32F446RE: 512 KB flash, 128 KB SRAM (RM0390 memory map).
 * See docs/ARCHITECTURE.md, "Memory map".
 */

ENTRY(Reset_Handler)

/* The main stack starts at the top of RAM and grows down. */
_estack = ORIGIN(RAM) + LENGTH(RAM);

/* Minimum heap and stack: the link fails if .data + .bss leave less free. */
_Min_Heap_Size  = 0x200;  /* 512 bytes */
_Min_Stack_Size = 0x400;  /* 1 KB */

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

SECTIONS
{
  /* First in flash: the core reads the initial SP and reset vector from here. */
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
    *(.glue_7) *(.glue_7t) *(.vfp11_veneer) *(.v4_bx) /* linker veneers */
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

  /* Unwind tables, for any libc code that expects them. */
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

  /* IFUNC tables. The linker creates them, always empty here: resolving an
   * IFUNC needs .rel.iplt applied at startup, and Reset_Handler does not. */
  .iplt :
  {
    *(.iplt) *(.igot.plt) *(.rel.iplt)
  } > FLASH
  ASSERT(SIZEOF(.iplt) == 0, "IFUNC is unsupported: Reset_Handler does not apply .rel.iplt")

  /* Initialised data: runs from RAM, initial values stored in flash and
   * copied by Reset_Handler. */
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

  /* Zero-initialised data, cleared by Reset_Handler. */
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

  /* Reserves the minimum heap and stack (see above). */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE(end = .);
    PROVIDE(_end = .);
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } > RAM

  /* Not loaded: debug information and build notes, kept for GDB and tools.
   * Every section is named so -Wl,--orphan-handling=error can reject any
   * other one, such as .init_array, instead of placing it by heuristic. */
  .comment         0 : { *(.comment) }
  .ARM.attributes  0 : { KEEP(*(.ARM.attributes)) }
  .debug_aranges   0 : { *(.debug_aranges) }
  .debug_pubnames  0 : { *(.debug_pubnames) }
  .debug_info      0 : { *(.debug_info) }
  .debug_abbrev    0 : { *(.debug_abbrev) }
  .debug_line      0 : { *(.debug_line .debug_line.* .debug_line_end) }
  .debug_frame     0 : { *(.debug_frame) }
  .debug_str       0 : { *(.debug_str) }
  .debug_loc       0 : { *(.debug_loc) }
  .debug_macinfo   0 : { *(.debug_macinfo) }
  .debug_pubtypes  0 : { *(.debug_pubtypes) }
  .debug_ranges    0 : { *(.debug_ranges) }
  .debug_addr      0 : { *(.debug_addr) }
  .debug_line_str  0 : { *(.debug_line_str) }
  .debug_loclists  0 : { *(.debug_loclists) }
  .debug_macro     0 : { *(.debug_macro) }
  .debug_names     0 : { *(.debug_names) }
  .debug_rnglists  0 : { *(.debug_rnglists) }
  .debug_str_offsets 0 : { *(.debug_str_offsets) }
}
