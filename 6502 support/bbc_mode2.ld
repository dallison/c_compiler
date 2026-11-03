/* Shadow screen on a Master. Mode 130 is 16-colour mode 2. Mode 129 is
   4-colour mode 1. Both keep the picture in shadow RAM and HIMEM at
   &8000. mode() reads that HIMEM and parks the stack there.
   On a Model B the same picture occupies &3000..&7FFF and this image
   does not fit under it.
   Until mode() runs, the machine is in mode 7. The cap keeps the
   software stack below the teletext screen at &7C00. The sideways shim
   still starts at &1F00, which is PAGE with DFS.
   .text.blitter is first, after the shim, so a paint routine can bank
   shadow RAM over &3000 and keep executing. It must stay below &3000. */
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
  .text : {
    *(.paged_mos)
    *(.paged_gate)
    *(.text.blitter)
    *(.text* .rodata* .davecc_stacktrace)
  } > ram
  .data : { *(.data* .preinit_array* .init_array* .fini_array*) } > ram
  .bss : { *(.bss* COMMON) } > ram
}
__bbc_stack_cap = 0x7c00;
ENTRY(_start)
