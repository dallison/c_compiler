#include "vars.s"
#include "fp.s"

.text

// Entry:
// A: offset of result (1 = equal)
// X: offset of op1
// Y: offset of op2
__cmpeqf:
  PHA
  JSR __fisnanA
  BCS cmp0
  JSR __fisnanB
  BCS cmp0
  JSR __fisinfA
  BCS cmp0
  JSR __fisinfB
  BCS cmp0

  LDA 0,X
  CMP 0,Y
  BNE cmp0
  LDA 1,X
  CMP 1,Y
  BNE cmp0
  LDA 2,X
  CMP 2,Y
  BNE cmp0
  LDA 3,X
  CMP 3,Y
  BNE cmp0
cmp0:
  PLX
  LDA #0
  STA 0,X
  RTS
cmp1:
  PLX
  LDA #1
  STA 0,X
  RTS

__cmpnef:
  JSR __cmpeqf
cmpinvert:
  LDA 0,X
  EOR #1
  STA 0,X
  RTS
 
// Compare A and B (0,X to 0,Y).  This only works for like-signed numbers so we need
// to check the signs.
__cmpltf:
  PHA
  JSR __fisnanA
  BCS cmp0
  JSR __fisnanB
  BCS cmp0
  JSR __fisinfA
  BCS cmp0
  JSR __fisinfB
  BCS cmp0

  LDA 3,X
  BMI cmpltf_a_neg
  LDA 3,Y
  BMI cmp0      // A is positive, B is negative.

  // Like signed compare.
cmpltf:
  LDA 0,X
  CMP 0,Y
  LDA 1,X
  SBC 1,Y
  LDA 2,X
  SBC 2,Y
  LDA 3,X
  SBC 3,Y
  BVC lt1
  EOR #0x80
lt1:
  BMI cmp1
  BRA cmp0

// A is negative.
cmpltf_a_neg:
  LDA 3,Y
  BPL cmp1    // B is positive, A is less than B

  // Both negative, swap comparison
  LDA 0,Y
  CMP 0,X
  LDA 1,Y
  SBC 1,X
  LDA 2,Y
  SBC 2,X
  LDA 3,Y
  SBC 3,X
  BVC lt2
  EOR #0x80
lt2:
  BMI cmp1
  BRA cmp0


__cmpgef:
  JSR __cmpltf
  BRA cmpinvert
