/* Mode 4 HIMEM is &5800. Keep the image above PAGE (&1F00) so a program
   that returns to BASIC does not replace the BASIC program. The ELF headers
   are loaded a few hundred bytes below the origin; &3000 still leaves that
   below the screen and clear of PAGE. Do not use this script for mode 2:
   that screen starts at &3000. */
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
/* Start the software stack below the mode 4 screen even when *RUN happens in
   mode 7 (HIMEM &7C00); main's frame must not sit in screen memory. */
__bbc_stack_cap = 0x5800;
ENTRY(_start)
