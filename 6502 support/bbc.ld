/* Model B with DFS 2.23 and ADFS 1.30: PAGE is &1F00. A program linked at
   the DFS-only value &1900 overwrites the ADFS workspace at &1E00, and the
   next Break reports "Bad sum". Mode 7's screen starts at &7C00. */
PHDRS
{
  interp PT_INTERP;
}
MEMORY
{
  ram (rwx) : ORIGIN = 0x1f00, LENGTH = 0
}
SECTIONS
{
  .text : { *(.text* .rodata* .davecc_stacktrace) } > ram
  .data : { *(.data* .preinit_array* .init_array* .fini_array*) } > ram
  .bss : { *(.bss* COMMON) } > ram
}
__bbc_stack_cap = 0;
ENTRY(_start)
