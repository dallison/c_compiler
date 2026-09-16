//
//  x86_codegen_private.h
//  Internal API shared between x86_codegen.c and x86_i386_wide.c.
//

#ifndef x86_codegen_private_h
#define x86_codegen_private_h

#include "x86_codegen.h"

struct Generator;
struct Symbol;

TargetInstruction* X86CgEmit(X86Generator* rv, TargetInstruction* inst);
TargetInstruction* X86CgEmitSymbol(X86Generator* rv, TargetInstruction* sym);
TargetInstruction* X86CgMaterialize(X86Generator* rv, IRNode* node);
TargetInstruction* X86CgGetIntConstant(X86Generator* rv, IRNode* node,
                                       TargetType type, int64_t value);
TargetInstruction* X86CgZero(X86Generator* rv);
TargetInstruction* X86CgFramePointer(X86Generator* rv);
TargetInstruction* X86CgStackPointer(X86Generator* rv);
TargetInstruction* X86CgAddImmediate(X86Generator* rv, TargetInstruction* src,
                                     int64_t immed);
TargetInstruction* X86CgSetDestOrMove(X86Generator* rv, TargetInstruction* from,
                                      TargetInstruction* to, X86Opcode mov);
TargetInstruction* X86CgGetDestInstruction(X86Generator* rv, Generator* gen,
                                           IRNode* node);
TargetInstruction* X86CgRouteResultToDest(X86Generator* rv, Generator* gen,
                                          IRNode* node,
                                          TargetInstruction* result);
TargetInstruction* X86CgGetSymbol(X86Generator* rv, IRNode* node,
                                  Symbol* symbol);
bool X86CgGetRegAndOffset(X86Generator* rv, IRNode* node,
                          TargetInstruction** reg, TargetInstruction** offset);
TargetInstruction* X86CgPushArg(X86Generator* rv, IRNode* node,
                                TargetInstruction* inst, size_t offset);
TargetInstruction* X86CgNewInstruction(X86Opcode opcode);
TargetInstruction* X86CgNewInstruction1(X86Opcode opcode, TargetInstruction* op0);
TargetInstruction* X86CgNewInstruction2(X86Opcode opcode, TargetInstruction* op0,
                                        TargetInstruction* op1);
TargetInstruction* X86CgNewInstruction3(X86Opcode opcode, TargetInstruction* op0,
                                        TargetInstruction* op1,
                                        TargetInstruction* op2);

bool X86I386TypeIsWideInt(TypeRecord* type);
bool X86I386NodeIsWideInt(IRNode* node);
void X86I386AssignWideMergeStackHomes(X86Generator* rv, Generator* gen);
bool X86I386ValidateFunction(X86Generator* rv, Generator* gen);
TargetInstruction* X86I386LowerExpression(X86Generator* rv, Generator* gen,
                                            IRNode* node);
TargetInstruction* X86I386LowerLoad(X86Generator* rv, Generator* gen,
                                    IRNode* node);
TargetInstruction* X86I386LowerStore(X86Generator* rv, Generator* gen,
                                     IRNode* node);
TargetInstruction* X86I386LowerComparison(X86Generator* rv, Generator* gen,
                                          IRNode* node);
TargetInstruction* X86I386LowerCompareBranch(X86Generator* rv, IRNode* expr,
                                             IRNode* target_node, bool btrue,
                                             bool reverse);
TargetInstruction* X86I386LowerBranchOnWideValue(X86Generator* rv,
                                                 IRNode* expr,
                                                 IRNode* target_node,
                                                 bool btrue);
TargetInstruction* X86I386LowerResult(X86Generator* rv, Generator* gen,
                                      IRNode* node);
TargetInstruction* X86I386LowerPushArg(X86Generator* rv, Generator* gen,
                                       IRNode* node);
TargetInstruction* X86I386LowerIncDec(X86Generator* rv, Generator* gen,
                                      IRNode* node, bool increment);
TargetInstruction* X86I386LowerVaArg(X86Generator* rv, IRNode* node);
TargetInstruction* X86I386LowerSignExtend(X86Generator* rv, Generator* gen,
                                          IRNode* node);
TargetInstruction* X86I386LowerZeroExtend(X86Generator* rv, Generator* gen,
                                          IRNode* node);
TargetInstruction* X86I386LowerCast(X86Generator* rv, Generator* gen,
                                    IRNode* node);
void X86I386CaptureWideCallResult(X86Generator* rv, IRNode* node,
                                  TargetInstruction** call_inout);
TargetInstruction* X86I386MaterializeWideArg(X86Generator* rv, IRNode* arg_node,
                                             size_t offset);

#endif /* x86_codegen_private_h */
