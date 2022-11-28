#ifndef __fp_s
#define __fp_s

#include "vars.s"

.text

// A good tutorial for the floating point algorithms:
// https://www.rfwireless-world.com/Tutorials/floating-point-tutorial.html
// Floating point calculator:
// https://www.h-schmidt.net/FloatConverter/IEEE754.html

.global __i1tof
.global __i2tof
.global __i4tof
.global __i8tof
 
.global __ui1tof
.global __ui2tof
.global __ui4tof
.global __ui8tof
 
.global __ftoi1
.global __ftoi2
.global __ftoi4
.global __ftoi8
.global __ftoui1
.global __ftoui2
.global __ftoui4
.global __ftoui8

.global __cmpeqf
.global __cmpnef
.global __cmpltf
.global __cmpgef
 
.global __fadd
.global __fsub
.global __fneg

.global __fmul
.global __fdiv

.global __fassemble
.global __fnormalize
.global __fround
.global __funpackA
.global __funpackB
.global __fzero
.global __fzero_mantissa
.global __fcheckA0
.global __fcheckB0
.global __fres0
.global __fresA
.global __fresB
.global __frshiftA
.global __frshiftB
.global __fnegmantissa
.global __fmantissa_is_zero
.global __fnan
.global __finf
.global __fisnanA
.global __fisinfA
.global __fisnanB
.global __fisinfB

.global __unpackIEEE754
.global __packIEEE754

// 4-byte Floating point format (IEEE 754 single precision)
// +---+--------+-----------------------------------+
// | S |   EXP  |      MANTISSA                     |
// +---+--------+-----------------------------------+
// EXP: 8 bit exponent
// MANTISSA: 23 bits
// S is the sign.

// The exponent is 2^n with a bias of 127.  So a value
// of 127 means 2^0.
// The mantissa has an implicit 1 prefix, so it 1.m where
// m is the value of the mantissa.
//
// Mantissa scratch format.
// For a IEEE754 single precision mantissa ABC (24 bits including implicit 1)
//
// 40    32    24    16    8     0   bit
// +-----+-----+-----+-----+-----+
// |  0  |  A  |  B  |  C  |  0  |
// +-----+-----+-----+-----+-----+
//    4     3     2     1     0      byte

// The 24 bits of the IEEE754 mantissa are in bytes 1, 2 and 3
// of the mantissa scratch space.
// Byte 0 is used to increase precision when shifting right to add
// Byte 4 is used to detect overflow.

// In the functions, A refers to the LHS and B to the RHS.
// All functions take:
// A: offset into zero page where result is stored
// X: offset into zero page of A
// Y: offset into zero page of B
// They do not write to A or B.
.set fmantissa fscratch_start+0    // 80 bit mantissa (double size for multiply)
.set fsign __t0
.set fexp __t1
.set frshift __t2

.set fmanA fscratch_start+10       // A mantissa (40 bit 2's comp)
.set fmanB fscratch_start+15      // B mantissa (40 bit 2's comp)
.set fexpA fscratch_start+20
.set fexpB fscratch_start+21
.set fsignA fscratch_start+22
.set fsignB fscratch_start+23


#endif
