#ifndef x86_64_codegen_h
#define x86_64_codegen_h
#include "../x86/x86_codegen.h"
typedef X86Generator X86_64Generator;
typedef X86AsmInstruction X86_64AsmInstruction;
typedef X86ExceptionRange X86_64ExceptionRange;
#define X86_64Lower X86Lower
#define X86_64Print X86Print
#define X86_64GeneratorInit X86GeneratorInit
#define X86_64GeneratorInitWithProfile X86GeneratorInitWithProfile
#define NewX86_64Generator NewX86Generator
#define NewX86_64GeneratorWithProfile NewX86GeneratorWithProfile
#define X86_64GeneratorDelete X86GeneratorDelete
#define X86_64GeneratorDestruct X86GeneratorDestruct
#define X86_64IsExpression X86IsExpression
#define X86_64IsFloatingPoint X86IsFloatingPoint
#define X86_64IsLoad X86IsLoad
#define X86_64IsSignedLoad X86IsSignedLoad
#define X86_64IsStore X86IsStore
#define X86_64IsFixedRegister X86IsFixedRegister
#define X86_64IsConst X86IsConst
#define X86_64IsSymbol X86IsSymbol
#define X86_64IsIntConst X86IsIntConst
#define X86_64IntValue X86IntValue
#define X86_64IsPossibleImmediate X86IsPossibleImmediate
#define X86_64IsBranch X86IsBranch
#define X86_64IsConditionalBranch X86IsConditionalBranch
#define X86_64IsReturn X86IsReturn
#define X86_64IsCall X86IsCall
#define X86_64GeneratesOutput X86GeneratesOutput
#define X86_64IsResult X86IsResult
#define X86_64IsSpill X86IsSpill
#define X86_64IsLabel X86IsLabel
#define X86_64IsArgRegister X86IsArgRegister
#define X86_64IsVarRegister X86IsVarRegister
#define X86_64IsJumpTableEntry X86IsJumpTableEntry
#define X86_64GetBranchTarget X86GetBranchTarget
#define X86_64OpcodeName X86OpcodeName
#define X86_64_REG_VAR X86_REG_VAR
#define X86_64_REG_VAR_MASK X86_REG_VAR_MASK
#define X86_64_IS_REG_VAR X86_IS_REG_VAR
#define X86_64_OP X86_OP
#define kX86_64RegTypeInt kX86RegTypeInt
#define kX86_64RegTypeFloat kX86RegTypeFloat
#endif /* x86_64_codegen_h */
