/* Mode 7: the screen starts at &7C00 and the software stack grows down from
   BASIC HIMEM. PAGE depends on the filing systems fitted (&1900 with DFS,
   &1F00 with DFS and ADFS, &2100 seen with Econet), so keep the image at
   &3000 like bbc_mode4.ld. The ELF headers are loaded a few hundred bytes
   below the origin. */
PHDRS
{
  interp PT_INTERP;
}
MEMORY
{
  ram (rwx) : ORIGIN = 0x3000, LENGTH = 0
}
SECTIONS
{
  .text : { *(.text* .rodata* .davecc_stacktrace) } > ram
  .data : { *(.data* .preinit_array* .init_array* .fini_array*) } > ram
  .bss : { *(.bss* COMMON) } > ram
}
__bbc_stack_cap = 0;
ENTRY(_start)
