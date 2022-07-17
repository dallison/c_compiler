#include "vars.s"
#include "fp.s"

.text

// Entry:
// A: offset of result (1 = equal)
// X: offset of op1
// Y: offset of op2
__cmpeqf:
  PHA
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
  PLX
  LDA #1
  STA 0,X
  RTS

__cmpnef:
  PHA
  LDA 0,X
  CMP 0,Y
  BNE cmp1
  LDA 1,X
  CMP 1,Y
  BNE cmp1
  LDA 2,X
  CMP 2,Y
  BNE cmp1
  LDA 3,X
  CMP 3,Y
  BNE cmp1
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

__cmpltf:
  PHA
  LDA 0,X
  CMP 3,Y
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


__cmpgef:
  PHA
  LDA 0,X
  CMP 3,Y
  LDA 1,X
  SBC 1,Y
  LDA 2,X
  SBC 2,Y
  LDA 3,X
  SBC 3,Y
  BVC ge1
  EOR #0x80
ge1:
  BPL cmp1
  BRA cmp0
