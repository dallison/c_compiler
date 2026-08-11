#include "vars.s"
#include "fp.s"

.text

// Entry:
// A: offset of boolean result (1 = true)
// X: offset of op1
// Y: offset of op2
__cmpeqf:
  PHA
  JSR __fisnanA
  BCS cmpeq_false
  JSR __fisnanB
  BCS cmpeq_false

  LDA 0,X
  CMP 0,Y
  BNE cmpeq_false
  LDA 1,X
  CMP 1,Y
  BNE cmpeq_false
  LDA 2,X
  CMP 2,Y
  BNE cmpeq_false
  LDA 3,X
  CMP 3,Y
  BNE cmpeq_false
  BRA cmpeq_true
cmpeq_false:
  PLA
  TAX
  LDA #0
  STA 0,X
  RTS
cmpeq_true:
  PLA
  TAX
  LDA #1
  STA 0,X
  RTS

__cmpnef:
  PHA
  JSR __fisnanA
  BCS cmpne_true
  JSR __fisnanB
  BCS cmpne_true

  LDA 0,X
  CMP 0,Y
  BNE cmpne_true
  LDA 1,X
  CMP 1,Y
  BNE cmpne_true
  LDA 2,X
  CMP 2,Y
  BNE cmpne_true
  LDA 3,X
  CMP 3,Y
  BNE cmpne_true
  BRA cmpne_false
cmpne_false:
  PLA
  TAX
  LDA #0
  STA 0,X
  RTS
cmpne_true:
  PLA
  TAX
  LDA #1
  STA 0,X
  RTS

// Compare A and B (0,X to 0,Y).  This only works for like-signed numbers so we need
// to check the signs.
__cmpltf:
  PHA
  JSR __fisnanA
  BCS cmpltf_false
  JSR __fisnanB
  BCS cmpltf_false

  LDA 3,X
  BMI cmpltf_a_neg
  LDA 3,Y
  BMI cmpltf_false      // A is positive, B is negative.

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
  BMI cmpltf_true
  BRA cmpltf_false

// A is negative.
cmpltf_a_neg:
  LDA 3,Y
  BPL cmpltf_true    // B is positive, A is less than B

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
  BMI cmpltf_true
  BRA cmpltf_false

cmpltf_false:
  PLA
  TAX
  LDA #0
  STA 0,X
  RTS
cmpltf_true:
  PLA
  TAX
  LDA #1
  STA 0,X
  RTS

__cmpgef:
  PHA
  JSR __fisnanA
  BCS cmpgef_false
  JSR __fisnanB
  BCS cmpgef_false
  PLA
  JSR __cmpltf
  LDA 0,X
  EOR #1
  STA 0,X
  RTS
cmpgef_false:
  PLA
  TAX
  LDA #0
  STA 0,X
  RTS
