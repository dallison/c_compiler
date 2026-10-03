/* Sideways libc image. Header, vectors, code, and initialized data pack
   from 0x8000. .bss is placed by the linker immediately after that image
   and must finish before 0xC000 (the MOS). LENGTH is unbounded here so the
   linker does not pad the last section out to the end of the bank; the
   build checks _end. */
MEMORY
{
  rom (rwx) : ORIGIN = 0x8000, LENGTH = 0
}
SECTIONS
{
  .rom_header : { *(.rom_header) } > rom
  .rom_vectors : { *(.rom_vectors) } > rom
  .text : {
    *(.text*)
    *(.rodata*)
    *(.data*)
    *(.preinit_array*)
    *(.init_array*)
    *(.fini_array*)
    __bss_start = .;
  } > rom
  /DISCARD/ : { *(.comment .note*) }
}
ENTRY(paged_rom_entry)
