// Shared layout for the BBC sideways libc image and its main-RAM shim.
// Included from 6502 assembly via the C preprocessor.
//
// The image is a language ROM in sideways RAM (the bank is writable, so
// .data and .bss live in the same 16 KiB window as the code):
//
//   0x8000  language ROM header (service + language entry)
//   0x8100  jump vectors, one JMP per exported function (3 bytes, append-only)
//   ...     libc code, .data, then .bss
//   0xc000  end of the sideways window (MOS follows)
//
// Vector 0 is always __paged_rom_init. Exported C functions follow in the
// order of exports.list. The shim JSRs these absolute addresses.

#ifndef paged_layout_h
#define paged_layout_h

#define PAGED_ROM_BASE 0x8000
#define PAGED_HEADER_SIZE 0x100
#define PAGED_VECTOR_BASE 0x8100
#define PAGED_VECTOR_STRIDE 3
#define PAGED_VECTOR_MAX 256
#define PAGED_ROM_END 0xc000

// MOS copies the currently selected sideways bank here. The hardware
// latch is at PAGED_ROMSEL. On the Master, bit 7 of the latch selects
// sideways RAM for reads and writes.
#define PAGED_ROM_ID 0x00f4
#define PAGED_ROMSEL 0xfe30

// Default sockets. Master 128 sideways RAM is fitted in banks 4-7.
// The second image holds the functions that do not fit in the first
// (libm, and the C++ bodies that are not header templates). Override
// with -DLIBC_ROM_SLOT / -DLIBC_ROM2_SLOT when assembling the shim.
#ifndef LIBC_ROM_SLOT
#define LIBC_ROM_SLOT 4
#endif
#ifndef LIBC_ROM2_SLOT
#define LIBC_ROM2_SLOT 5
#endif
#ifndef LIBC_ROM3_SLOT
#define LIBC_ROM3_SLOT 6
#endif
#ifndef LIBC_ROM4_SLOT
#define LIBC_ROM4_SLOT 7
#endif
#ifndef LIBC_ROM5_SLOT
#define LIBC_ROM5_SLOT 8
#endif
#ifndef LIBC_ROM6_SLOT
#define LIBC_ROM6_SLOT 9
#endif
#ifndef LIBC_ROM7_SLOT
#define LIBC_ROM7_SLOT 10
#endif
// How many images ensure_init brings up. The build script sets this to
// the number of images it actually linked.
#ifndef PAGED_ROM_COUNT
#define PAGED_ROM_COUNT 7
#endif

// Standard streams live in the MOS cassette/RS423 output buffer
// (&0900-&09FF), which a disc system leaves idle. The bodies are main
// RAM, so every sideways bank can dereference them. Keep these in sync
// with the same names in libc/include/stdio.h.
#define PAGED_STDIN_FILE 0x0900
#define PAGED_STDOUT_FILE 0x0920
#define PAGED_STDERR_FILE 0x0940
#define PAGED_STDIN_BUF 0x0960
#define PAGED_STDOUT_BUF 0x09a0
#define PAGED_STDIN_PTR 0x09e0
#define PAGED_STDOUT_PTR 0x09e2
#define PAGED_STDERR_PTR 0x09e4

// Main-RAM entry a paged ROM uses to call a function in the other paged
// ROM. It has to live here: the return address of that call points into
// the caller's bank, which is switched out for the duration of the call.
// bbc.ld places this at the start of the user image, which is PAGE &1F00.
#define PAGED_GATE 0x1f00

// OSBYTE 0 returns this value (in X) for MOS 3.20 and later. Those
// machines need ROMSEL bit 7 set while the libc bank is selected.
#define PAGED_MASTER_OS 3

#define PAGED_OSBYTE 0xfff4
#define PAGED_OSWORD 0xfff1
#define PAGED_OSCLI 0xfff7
#define PAGED_OSWRCH 0xffee
#define PAGED_OSNEWL 0xffe7

// Mailbox in the user program (main RAM). The ROM init reads the heap
// bounds. The stream slots are reserved and unused; the live pointers
// are PAGED_STDIN_PTR and the two words after it.
//   +0  heap start (user program _end)
//   +2  heap limit (__bbc_stack_cap, or 0 to use mode 7 HIMEM &7C00)
//   +4  reserved
//   +6  reserved
//   +8  reserved
#define PAGED_MBOX_HEAP 0
#define PAGED_MBOX_LIMIT 2
#define PAGED_MBOX_STDIN 4
#define PAGED_MBOX_STDOUT 6
#define PAGED_MBOX_STDERR 8

#endif
