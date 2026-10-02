#ifndef __vars_s
#define __vars_s

// Zero page locations.
// Make these match the output in the compiler assembly produced
// in 6502_target.c and the layout in 6502_machine.h.
//
// The C ABI uses 0x00..0x8f.  0x90..0xff is ROM and interrupt scratch.
// double has no registers of its own; it shares the float slots.

.set __b0 0x0
.set __b1 0x1
.set __b2 0x2
.set __b3 0x3
.set __b4 0x4
.set __b5 0x5
.set __i0 0x6
.set __i1 0x8
.set __i2 0xa
.set __i3 0xc
.set __i4 0xe
.set __i5 0x10
.set __i6 0x12
.set __i7 0x14
.set __l0 0x16
.set __l1 0x1a
.set __l2 0x1e
.set __l3 0x22
.set __x0 0x26
.set __x1 0x2e
.set __x2 0x36
.set __f0 0x3e
.set __f1 0x42
.set __f2 0x46
.set __f3 0x4a
.set __sp 0x4e
.set __fp 0x50
.set __result 0x52
.set __t0 0x54
.set __t1 0x55
.set __t2 0x56
.set __t3 0x57

// These are for memory copy operations.  They overlap with math temps (mt).
.set __mem_src 0x58
.set __mem_dest 0x5a
.set __mem_size 0x5c

// Math scratch space starts at 0x58 (32 bytes)
// Used for integer multiplication and division.
.set mt1 0x58     // 16 bytes
.set mt2 0x68     // 8 bytes
.set mt3 0x70     // 8 bytes

// Floating point scratch space
// For floats:
// 5 byte mantissa (x4)   20 bytes
// 1 byte exponent (x2)   2 bytes
// 1 byte sign (x2)       2 bytes
//                        24 bytes
//
// Float scratch space is 0x78-0x90 (24 bytes, end exclusive).
.set fscratch_start 0x78
.set fscratch_end 0x90

.set stack_bottom 0xc000
.set sys_exit 1
.set sys_abort 2

// Spare zero page for the ROM starts at 0x90.
.set os_scratch_start 0x90

// Scratch byte for the NMOS expansions of STZ, PHX/PHY, and (zp) indirect.
// The 65C02 side of those #ifs does not touch it.
.comm __nmos_tmp, 1

#endif
