/* Model B, DFS PAGE at &1900, mode 7 screen at &7C00. */
PHDRS
{
  interp PT_INTERP;
}
MEMORY
{
  ram (rwx) : ORIGIN = 0x1900, LENGTH = 0
}
SECTIONS
{
  .text : { *(.text* .rodata* .davecc_stacktrace) } > ram
  .data : { *(.data* .preinit_array* .init_array* .fini_array*) } > ram
  .bss : { *(.bss* COMMON) } > ram
}
ENTRY(_start)
