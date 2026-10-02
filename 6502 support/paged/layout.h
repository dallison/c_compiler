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

// Default socket. Master 128 sideways RAM is fitted in banks 4-7.
// Override with -DLIBC_ROM_SLOT=n when assembling the shim.
#ifndef LIBC_ROM_SLOT
#define LIBC_ROM_SLOT 4
#endif

// OSBYTE 0 returns this value (in X) for MOS 3.20 and later. Those
// machines need ROMSEL bit 7 set while the libc bank is selected.
#define PAGED_MASTER_OS 3

#define PAGED_OSBYTE 0xfff4
#define PAGED_OSWORD 0xfff1
#define PAGED_OSCLI 0xfff7
#define PAGED_OSWRCH 0xffee
#define PAGED_OSNEWL 0xffe7

// Mailbox in the user program (main RAM). The ROM init reads the first
// four bytes and writes the standard-stream pointers into the next six.
//   +0  heap start (user program _end)
//   +2  heap limit (__bbc_stack_cap, or 0 to use mode 7 HIMEM &7C00)
//   +4  stdin
//   +6  stdout
//   +8  stderr
#define PAGED_MBOX_HEAP 0
#define PAGED_MBOX_LIMIT 2
#define PAGED_MBOX_STDIN 4
#define PAGED_MBOX_STDOUT 6
#define PAGED_MBOX_STDERR 8

#endif
