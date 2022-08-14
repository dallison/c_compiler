#ifndef __vars_s
#define __vars_s

// Zero page locations.
// Make these match the output in the compiler assembly produced
// in 6502_target.c.

// Zero pages address 0x00..0xba are used by the C runtime.
// 0xbb..0xff are available for OS.
// 0x44(68) bytes for OS.
.set __b0 0x0
.set __b1 0x1
.set __b2 0x2
.set __b3 0x3
.set __b4 0x4
.set __b5 0x5
.set __b6 0x6
.set __b7 0x7
.set __i0 0x8
.set __i1 0xa
.set __i2 0xc
.set __i3 0xe
.set __i4 0x10
.set __i5 0x12
.set __i6 0x14
.set __i7 0x16
.set __i8 0x18
.set __i9 0x1a
.set __i10 0x1c
.set __i11 0x1e
.set __i12 0x20
.set __i13 0x22
.set __i14 0x24
.set __i15 0x26
.set __l0 0x28
.set __l1 0x2c
.set __l2 0x30
.set __l3 0x34
.set __l4 0x38
.set __l5 0x3c
.set __l6 0x40
.set __l7 0x44
.set __x0 0x48
.set __x1 0x50
.set __x2 0x58
.set __x3 0x60
.set __f0 0x68
.set __d0 0x68
.set __f1 0x6c
.set __d1 0x6c
.set __f2 0x70
.set __d2 0x70
.set __f3 0x74
.set __d3 0x74
.set __sp 0x78
.set __fp 0x7a
.set __result 0x7c
.set __t0 0x7e
.set __t1 0x7f
.set __t2 0x80
.set __t3 0x81

// These are for memory copy operations.  They overlap with math temps (mt).
.set __mem_src 0x82
.set __mem_dest 0x84
.set __mem_size 0x86

// Math scratch space starts at 0x82 (32 bytes)
// Used for integer multiplication and division.
.set mt1 0x82     // 16 bytes
.set mt2 0x92     // 8 bytes
.set mt3 0x9a     // 8 bytes

// Floating point scratch space
// For floats:
// 5 byte mantissa (x4)   20 bytes
// 1 byte exponent (x2)   2 bytes
// 1 byte sign (x2)       2 bytes
//                        24 bytes
//

// Float scratch space is 0xa2-0xba (24 bytes)
.set fscratch_start 0xa2
.set fscratch_end 0xba

// Double isn't worth it on 6502.  It takes too much memory and
// is too slow.

.set stack_bottom 0xc000
.set sys_exit 1
.set sys_abort 2

// Spare zero page is from 0xbb..0xff
.set os_scratch_start 0xbb

#endif

