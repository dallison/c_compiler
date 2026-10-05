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

// MOS copies the socket number (bits 0-3) here. The hardware latch is at
// PAGED_ROMSEL. Bits 4-6 of the latch choose which of the eight images in
// that socket is visible. Bit 7 is the Master ANDY overlay and is not
// part of the image select. Image 0 is the one MOS sees.
#define PAGED_ROM_ID 0x00f4
#define PAGED_ROMSEL 0xfe30

// Seven images, one socket, in this order. Image 0 is the language ROM.
#define PAGED_VIRT_LIBC 0
#define PAGED_VIRT_LIBM 1
#define PAGED_VIRT_MATH_A 2
#define PAGED_VIRT_MATH_B 3
#define PAGED_VIRT_CXX 4
#define PAGED_VIRT_STDIO 5
#define PAGED_VIRT_FLOAT 6

// How many images ensure_init brings up.
#ifndef PAGED_ROM_COUNT
#define PAGED_ROM_COUNT 7
#endif

// Title of image 0. rom_header.s places "DaveCC libc" here; the socket
// scan compares against it. Do not insert bytes ahead of that string.
#define PAGED_TITLE 0x8009

// Main RAM at PAGE. The MOS trampoline is first so a ROM can patch its
// JSR operand at a fixed address. The cross-image gate follows it.
#define PAGED_MOS_CALL 0x1f00
#define PAGED_MOS_JSR 0x1f01

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

// Main-RAM entry a paged ROM uses to call a function in another image.
// The return address of that call points into the caller's bank, which
// is switched out for the duration of the call. bbc.ld places the MOS
// trampoline at PAGED_MOS_CALL and this gate directly after it.
#define PAGED_GATE 0x1f30

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
