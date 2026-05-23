#ifndef __vars_s
#define __vars_s

// Zero page locations (must match 6502_target.c / 6502_machine.h).
// REG_BASE is the start of the unified register file; rebuild the support
// library if using a non-zero -freg-start.

.set REG_BASE 0

.set __zpr0 (REG_BASE + 0)
.set __zpr1 (REG_BASE + 1)
.set __zpr2 (REG_BASE + 2)
.set __zpr3 (REG_BASE + 3)
.set __zpr4 (REG_BASE + 4)
.set __zpr5 (REG_BASE + 5)
.set __zpr6 (REG_BASE + 6)
.set __zpr7 (REG_BASE + 7)
.set __zpr8 (REG_BASE + 8)
.set __zpr9 (REG_BASE + 9)
.set __zpr10 (REG_BASE + 10)
.set __zpr11 (REG_BASE + 11)
.set __zpr12 (REG_BASE + 12)
.set __zpr13 (REG_BASE + 13)
.set __zpr14 (REG_BASE + 14)
.set __zpr15 (REG_BASE + 15)
.set __zpr16 (REG_BASE + 16)
.set __zpr17 (REG_BASE + 17)
.set __zpr18 (REG_BASE + 18)
.set __zpr19 (REG_BASE + 19)
.set __zpr20 (REG_BASE + 20)
.set __zpr21 (REG_BASE + 21)
.set __zpr22 (REG_BASE + 22)
.set __zpr23 (REG_BASE + 23)
.set __zpr24 (REG_BASE + 24)
.set __zpr25 (REG_BASE + 25)
.set __zpr26 (REG_BASE + 26)
.set __zpr27 (REG_BASE + 27)
.set __zpr28 (REG_BASE + 28)
.set __zpr29 (REG_BASE + 29)
.set __zpr30 (REG_BASE + 30)
.set __zpr31 (REG_BASE + 31)
.set __zpr32 (REG_BASE + 32)
.set __zpr33 (REG_BASE + 33)
.set __zpr34 (REG_BASE + 34)
.set __zpr35 (REG_BASE + 35)
.set __zpr36 (REG_BASE + 36)
.set __zpr37 (REG_BASE + 37)
.set __zpr38 (REG_BASE + 38)
.set __zpr39 (REG_BASE + 39)
.set __zpr40 (REG_BASE + 40)
.set __zpr41 (REG_BASE + 41)
.set __zpr42 (REG_BASE + 42)
.set __zpr43 (REG_BASE + 43)
.set __zpr44 (REG_BASE + 44)
.set __zpr45 (REG_BASE + 45)
.set __zpr46 (REG_BASE + 46)
.set __zpr47 (REG_BASE + 47)
.set __zpr48 (REG_BASE + 48)
.set __zpr49 (REG_BASE + 49)
.set __zpr50 (REG_BASE + 50)
.set __zpr51 (REG_BASE + 51)
.set __zpr52 (REG_BASE + 52)
.set __zpr53 (REG_BASE + 53)
.set __zpr54 (REG_BASE + 54)
.set __zpr55 (REG_BASE + 55)
.set __zpr56 (REG_BASE + 56)
.set __zpr57 (REG_BASE + 57)
.set __zpr58 (REG_BASE + 58)
.set __zpr59 (REG_BASE + 59)
.set __zpr60 (REG_BASE + 60)
.set __zpr61 (REG_BASE + 61)
.set __zpr62 (REG_BASE + 62)
.set __zpr63 (REG_BASE + 63)

// Legacy logical register aliases for hand-written runtime modules.
.set __b0 __zpr0
.set __b1 __zpr1
.set __b2 __zpr2
.set __b3 __zpr3
.set __b4 __zpr4
.set __b5 __zpr5
.set __b6 __zpr6
.set __b7 __zpr7
.set __i0 __zpr8
.set __i1 __zpr10
.set __i2 __zpr12
.set __i3 __zpr14
.set __i4 __zpr16
.set __i5 __zpr18
.set __i6 __zpr20
.set __i7 __zpr22
.set __i8 __zpr24
.set __i9 __zpr26
.set __i10 __zpr28
.set __i11 __zpr30
.set __i12 __zpr32
.set __i13 __zpr34
.set __i14 __zpr36
.set __i15 __zpr38
.set __l0 __zpr40
.set __l1 __zpr44
.set __l2 __zpr48
.set __l3 __zpr52
.set __l4 __zpr56
.set __l5 __zpr60
.set __l6 __zpr56
.set __l7 __zpr60
.set __x0 __zpr32
.set __x1 __zpr40
.set __x2 __zpr48
.set __x3 __zpr56
.set __f0 __zpr36
.set __f1 __zpr40
.set __f2 __zpr44
.set __f3 __zpr48

.set __sp (REG_BASE + 64)
.set __fp (REG_BASE + 66)
.set __result (REG_BASE + 68)
.set __t0 (REG_BASE + 70)
.set __t1 (REG_BASE + 71)
.set __t2 (REG_BASE + 72)
.set __t3 (REG_BASE + 73)

.set __mem_src (REG_BASE + 74)
.set __mem_dest (REG_BASE + 76)
.set __mem_size (REG_BASE + 78)

// Math/fp scratch immediately after compiler core (64-byte reg file + 16-byte specials).
.set mt1 (REG_BASE + 80)
.set mt2 (REG_BASE + 90)
.set mt3 (REG_BASE + 96)

.set fscratch_start (REG_BASE + 102)
.set fscratch_end (REG_BASE + 126)

.set stack_bottom 0xc000
.set sys_exit 1
.set sys_abort 2

// Spare zero page above runtime layout.
.set os_scratch_start (REG_BASE + 126)

#endif
