#ifndef AARCH64_ENCODE_H
#define AARCH64_ENCODE_H

#include <stdbool.h>
#include <stdint.h>

#include "asm_object.h"
#include "assembler.h"

struct AARCH64Generator;

typedef enum {
  AARCH64_ASM_REG_BAD,
  AARCH64_ASM_REG_W,
  AARCH64_ASM_REG_X,
  AARCH64_ASM_REG_S,
  AARCH64_ASM_REG_D,
  AARCH64_ASM_REG_B,
  AARCH64_ASM_REG_H,
  AARCH64_ASM_REG_Q,
} AARCH64AsmRegKind;

typedef struct {
  int num;
  AARCH64AsmRegKind kind;
  bool fp_or_simd;
  bool is_sp;
  int size;
} AARCH64AsmRegister;

typedef enum {
  AARCH64_ASM_SHIFT_LSL,
  AARCH64_ASM_SHIFT_LSR,
  AARCH64_ASM_SHIFT_ASR,
} AARCH64AsmShiftKind;

typedef enum {
  AARCH64_ASM_OPERAND_UNKNOWN,
  AARCH64_ASM_OPERAND_REGISTER,
  AARCH64_ASM_OPERAND_INT_IMM,
  AARCH64_ASM_OPERAND_FLOAT_IMM,
} AARCH64AsmOperandKind;

typedef struct {
  AARCH64AsmShiftKind type;
  int amount;
} AARCH64AsmShift;

typedef struct {
  AARCH64AsmOperandKind type;
  union {
    AARCH64AsmRegister reg;
    int64_t i;
    double f;
  };
  AARCH64AsmShift shift;
} AARCH64AsmOperand;

typedef enum {
  AARCH64_ASM_COND_EQ = 0,
  AARCH64_ASM_COND_NE = 1,
  AARCH64_ASM_COND_CS = 2,
  AARCH64_ASM_COND_HS = AARCH64_ASM_COND_CS,
  AARCH64_ASM_COND_CC = 3,
  AARCH64_ASM_COND_LO = AARCH64_ASM_COND_CC,
  AARCH64_ASM_COND_MI = 4,
  AARCH64_ASM_COND_PL = 5,
  AARCH64_ASM_COND_VS = 6,
  AARCH64_ASM_COND_VC = 7,
  AARCH64_ASM_COND_HI = 8,
  AARCH64_ASM_COND_LS = 9,
  AARCH64_ASM_COND_GE = 10,
  AARCH64_ASM_COND_LT = 11,
  AARCH64_ASM_COND_GT = 12,
  AARCH64_ASM_COND_LE = 13,
  AARCH64_ASM_COND_AL = 14,
} AARCH64AsmCondition;

typedef struct {
  AssemblerSymbol* symbol;
  int32_t reloc_type;
  int32_t addend;
} AARCH64BranchTarget;

static inline bool AARCH64AsmRegIs64Bit(const AARCH64AsmRegister* reg) {
  return reg->kind == AARCH64_ASM_REG_X;
}

static inline int AARCH64AsmRegSf(const AARCH64AsmRegister* reg) {
  return AARCH64AsmRegIs64Bit(reg) ? 1 : 0;
}

AARCH64AsmRegister AARCH64AsmZeroRegister(AARCH64AsmRegKind kind);

uint32_t AARCH64EncodeMoveWide(bool is_64bit, int opc, uint16_t immediate,
                               int halfword);
uint32_t AARCH64EncodeBranchRegister(int opc, int op3, int reg);
uint32_t AARCH64EncodeReturn(int reg);

uint32_t AARCH64EncodeAddSubImmediate(const AARCH64AsmRegister* rd,
                                      const AARCH64AsmRegister* rn, int immed,
                                      bool subtract, bool set_flags,
                                      int shift);
uint32_t AARCH64EncodeAddSubShiftedRegister(const AARCH64AsmRegister* rd,
                                            const AARCH64AsmRegister* rn,
                                            const AARCH64AsmOperand* rm,
                                            bool subtract, bool set_flags);
uint32_t AARCH64EncodeAddSubWithCarry(const AARCH64AsmRegister* rd,
                                      const AARCH64AsmRegister* rn,
                                      const AARCH64AsmRegister* rm,
                                      bool subtract, bool set_flags);

bool AARCH64EncodeLogicalImmediate(uint64_t imm, int sf, unsigned* encoding);
uint32_t AARCH64EncodeLogicalImmediateInst(const AARCH64AsmRegister* rd,
                                           const AARCH64AsmRegister* rn,
                                           int64_t immed, int sf, int opc);
uint32_t AARCH64EncodeLogicalShiftedRegister(const AARCH64AsmRegister* rd,
                                             const AARCH64AsmRegister* rn,
                                             const AARCH64AsmOperand* rm, int sf,
                                             int opc, int invert_rn);

uint32_t AARCH64EncodeConditionalBranch(int32_t offset, AARCH64AsmCondition cond,
                                        bool consistent);
uint32_t AARCH64EncodeUnconditionalBranchImmediate(int32_t offset, bool link);
uint32_t AARCH64EncodeRotateRightImmediate(const AARCH64AsmRegister* rd,
                                           const AARCH64AsmRegister* rn,
                                           int64_t shift);
uint32_t AARCH64EncodeSvc(uint16_t immediate);

void AARCH64EmitInstruction(AsmObject* object, uint32_t word);
void AARCH64EmitInstructionInSection(AsmObject* object, int32_t section,
                                     uint32_t word);

void AARCH64EmitAddSubImmediate(AsmObject* object, const AARCH64AsmRegister* rd,
                                const AARCH64AsmRegister* rn, int immed,
                                bool subtract, bool set_flags, int shift);
void AARCH64EmitAddSubShiftedRegister(AsmObject* object,
                                      const AARCH64AsmRegister* rd,
                                      const AARCH64AsmRegister* rn,
                                      const AARCH64AsmOperand* rm, bool subtract,
                                      bool set_flags);
void AARCH64EmitAddSubWithCarry(AsmObject* object, const AARCH64AsmRegister* rd,
                                const AARCH64AsmRegister* rn,
                                const AARCH64AsmRegister* rm, bool subtract,
                                bool set_flags);
void AARCH64EmitLogicalImmediate(AsmObject* object, const AARCH64AsmRegister* rd,
                                 const AARCH64AsmRegister* rn, int64_t immed,
                                 int sf, int opc);
void AARCH64EmitLogicalShiftedRegister(AsmObject* object,
                                       const AARCH64AsmRegister* rd,
                                       const AARCH64AsmRegister* rn,
                                       const AARCH64AsmOperand* rm, int sf,
                                       int opc, int invert_rn);
void AARCH64EmitMoveWide(AsmObject* object, const AARCH64AsmRegister* rd, int sf,
                         int opc, int imm16, int hw);
void AARCH64EmitMoveImmediate(AsmObject* object, const AARCH64AsmRegister* rd,
                              int64_t immed);
void AARCH64EmitBranchRegister(AsmObject* object, int opc, int op3, int reg);
void AARCH64EmitReturn(AsmObject* object, int reg);
void AARCH64EmitConditionalBranch(AsmObject* object, int32_t offset,
                                  AARCH64AsmCondition cond, bool consistent);
void AARCH64EmitUnconditionalBranchImmediate(AsmObject* object, int32_t offset,
                                             bool link);
void AARCH64EmitUnconditionalBranchToSymbol(Assembler* assembler,
                                            AssemblerSymbol* sym, bool link,
                                            bool pic);

bool AARCH64CanDirectEncodeFunction(struct AARCH64Generator* generator);
void AARCH64DirectEncodeFunction(struct AARCH64Generator* generator,
                                 Assembler* assembler);

#endif /* AARCH64_ENCODE_H */
