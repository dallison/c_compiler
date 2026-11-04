/* Like bbc_mode2.ld, but linked at &E00 for a post-*TAPE image.
   DFS still loads a small reloc stub at PAGE (&1F00); the stub selects
   TAPE, copies this image down to &E00, and jumps to _start.
   Shadow mode 1 keeps HIMEM at &8000. The blitter must stay below &3000. */
PHDRS
{
  interp PT_INTERP;
}
MEMORY
{
  ram (rwx) : ORIGIN = 0x0e00, LENGTH = 0
}
SECTIONS
{
  .text : {
    *(.paged_mos)
    *(.paged_gate)
    /* Blitter + sprite bitmaps (games/sprites.s). Must stay below &3000:
       soft stack grows down from &7C00 and shreds high .rodata under COROs.
       daveld ignores a separate .rodata.sprites wildcard — park in blitter. */
    *(.text.blitter)
    *(.text* .rodata* .davecc_stacktrace)
  } > ram
  .data : { *(.data* .preinit_array* .init_array* .fini_array*) } > ram
  .bss : { *(.bss* COMMON) } > ram
}
/* Stay at &7C00 until mode(129) re-parks at HIMEM. Mode 7 screen owns
   &7C00..&7FFF — a stack top of &8000 corrupts the screen and the stack. */
__bbc_stack_cap = 0x7c00;
ENTRY(_start)
