//
//  x86_i386_wide.c
//  i386 64-bit integer (long long) lowering for the shared x86 backend.
//
//  Each wide value is a (lo, hi) pair of independently allocated 32-bit
//  TargetInstructions.  lo is in node->data.ptr; hi in node->data.lvalue.
//

#include "x86_codegen_private.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "codegen.h"
#include "compiler.h"
#include "errors.h"
#include "ir.h"
#include "symbol.h"
#include "type.h"

static void I386SetLoweredHi(IRNode* node, TargetInstruction* hi) {
  node->data.lvalue = (int64_t)(intptr_t)hi;
}

static TargetInstruction* I386GetLoweredHi(IRNode* node) {
  return (TargetInstruction*)(intptr_t)node->data.lvalue;
}

static void I386PairWideHalves(TargetInstruction* lo, TargetInstruction* hi) {
  if (lo != NULL && hi != NULL && lo != hi) {
    TargetAddUser(lo, hi);
  }
}

static TargetInstruction* I386Const32(X86Generator* rv, int32_t value) {
  if (value == 0) {
    return X86CgZero(rv);
  }
  return X86CgGetIntConstant(rv, NULL, kTargetType32Bit, value);
}

static TargetInstruction* I386WideMov(X86Generator* rv, TargetInstruction* src) {
  return X86CgEmit(rv, X86CgNewInstruction1(X86_OP(mv), src));
}

static void I386WideMemoryOffsets(X86Generator* rv, TargetInstruction* addr,
                                  TargetInstruction* offset,
                                  TargetInstruction** base,
                                  TargetInstruction** off_lo,
                                  TargetInstruction** off_hi) {
  *base = addr;
  *off_lo = X86CgGetIntConstant(rv, NULL, kTargetType32Bit, 0);
  *off_hi = X86CgGetIntConstant(rv, NULL, kTargetType32Bit, 4);
  if (offset == NULL || TargetIsZero(offset)) {
    return;
  }
  if (TargetIsConst(offset)) {
    int off = (int)TargetIntValue(offset);
    *off_lo = X86CgGetIntConstant(rv, NULL, kTargetType32Bit, off);
    *off_hi = X86CgGetIntConstant(rv, NULL, kTargetType32Bit, off + 4);
    return;
  }
  *base = X86CgEmit(rv, X86CgNewInstruction2(X86_OP(add), addr, offset));
}

static int I386AllocateWideMergeHome(X86Generator* rv) {
  int offset = -X86_P(rv)->stack_frame_header_size -
               rv->base.stack_frame_size - rv->saved_arg_area_size - 8;
  rv->saved_arg_area_size += 8;
  return offset;
}

void X86I386AssignWideMergeStackHomes(X86Generator* rv, Generator* gen) {
  for (IRNode* node = GeneratorFirstInstruction(gen); node != NULL;
       node = IRNext(node)) {
    IRNode* dest = node->dest;
    if (dest != NULL && dest->opcode == IR_OP(tmp) &&
        X86I386NodeIsWideInt(dest) && dest->data.ivalue == 0) {
      dest->data.ivalue = I386AllocateWideMergeHome(rv);
    }
  }
}

static bool I386WideMergeTmpHasStackHome(IRNode* tmp) {
  if (tmp == NULL || !X86I386NodeIsWideInt(tmp) || tmp->data.ivalue >= 0) {
    return false;
  }
  switch (tmp->opcode) {
    case IR_OP(tmp):
    case IR_OP(calla):
    case IR_OP(muli):
    case IR_OP(divi):
    case IR_OP(modi):
    case IR_OP(lsli):
    case IR_OP(lsri):
    case IR_OP(asri):
      return true;
    default:
      return false;
  }
}

static void I386StoreWideToMergeTmp(X86Generator* rv, IRNode* tmp,
                                    TargetInstruction* lo,
                                    TargetInstruction* hi) {
  if (!I386WideMergeTmpHasStackHome(tmp)) {
    return;
  }
  int off = (int)tmp->data.ivalue;
  TargetInstruction* fp = X86CgFramePointer(rv);
  X86CgEmit(rv, X86CgNewInstruction3(X86_OP(storel), lo, fp,
                                     X86CgGetIntConstant(rv, NULL,
                                                         kTargetType32Bit, off)));
  X86CgEmit(rv,
            X86CgNewInstruction3(
                X86_OP(storel), hi, fp,
                X86CgGetIntConstant(rv, NULL, kTargetType32Bit, off + 4)));
}

static void I386LoadWideFromMergeTmp(X86Generator* rv, IRNode* tmp,
                                     TargetInstruction** lo,
                                     TargetInstruction** hi) {
  if (!I386WideMergeTmpHasStackHome(tmp)) {
    *lo = TargetGetLoweredNode(tmp);
    *hi = I386GetLoweredHi(tmp);
    return;
  }
  int off = (int)tmp->data.ivalue;
  TargetInstruction* fp = X86CgFramePointer(rv);
  *lo = X86CgEmit(rv,
                  X86CgNewInstruction2(
                      X86_OP(loadl), fp,
                      X86CgGetIntConstant(rv, NULL, kTargetType32Bit, off)));
  *hi = X86CgEmit(
      rv, X86CgNewInstruction2(
              X86_OP(loadl), fp,
              X86CgGetIntConstant(rv, NULL, kTargetType32Bit, off + 4)));
  I386PairWideHalves(*lo, *hi);
}

static void I386RouteWideResultToDest(X86Generator* rv, Generator* gen,
                                      IRNode* node, TargetInstruction* lo,
                                      TargetInstruction* hi) {
  if (node->dest == NULL || !X86I386NodeIsWideInt(node->dest)) {
    return;
  }
  I386StoreWideToMergeTmp(rv, node->dest, lo, hi);
}

static bool I386WideValueInMemory(IRNode* node) {
  if (node == NULL || !X86I386NodeIsWideInt(node)) {
    return false;
  }
  if (IRIsConst(node) || I386WideMergeTmpHasStackHome(node)) {
    return false;
  }
  if (node->data.ptr != NULL && I386GetLoweredHi(node) != NULL) {
    return false;
  }
  if (IRIsAutoVariable(node) || IRIsArgument(node)) {
    return !X86_IS_REG_VAR(node->data.ivalue);
  }
  return IRIsStaticVariable(node);
}

static void I386LoadWideFromMemory(X86Generator* rv, IRNode* node,
                                    TargetInstruction** lo,
                                    TargetInstruction** hi) {
  TargetInstruction* addr;
  TargetInstruction* offset;
  if (IRIsStaticVariable(node)) {
    addr = TargetGetLoweredNode(node);
    if (addr == NULL) {
      IRVariable* var = (IRVariable*)node;
      addr = X86CgGetSymbol(rv, node, var->symbol);
    }
    offset = X86CgGetIntConstant(rv, NULL, kTargetType32Bit, 0);
  } else {
    bool found_address = X86CgGetRegAndOffset(rv, node, &addr, &offset);
    assert(found_address);
    (void)found_address;
    assert(TargetIsConst(offset));
  }
  int off = (int)TargetIntValue(offset);
  TargetInstruction* lo_base = I386WideMov(rv, addr);
  TargetInstruction* hi_base = I386WideMov(rv, addr);
  *lo = X86CgEmit(
      rv, X86CgNewInstruction2(
              X86_OP(loadl), lo_base,
              X86CgGetIntConstant(rv, NULL, kTargetType32Bit, off)));
  *hi = X86CgEmit(
      rv, X86CgNewInstruction3(
              X86_OP(loadl), hi_base,
              X86CgGetIntConstant(rv, NULL, kTargetType32Bit, off + 4), addr));
  I386PairWideHalves(*lo, *hi);
}

static void I386MaterializeWide(X86Generator* rv, IRNode* node,
                                TargetInstruction** lo,
                                TargetInstruction** hi) {
  if (node->opcode == IR_OP(pusharg) && node->inputs.length > 0) {
    node = node->inputs.value.p[0];
  }
  if (I386WideValueInMemory(node)) {
    I386LoadWideFromMemory(rv, node, lo, hi);
    return;
  }
  if (IRIsConst(node)) {
    uint64_t v = (uint64_t)((IRConstant*)node)->value.ivalue;
    *lo = I386Const32(rv, (int32_t)(uint32_t)v);
    *hi = I386Const32(rv, (int32_t)(uint32_t)(v >> 32));
    if (*lo != *hi) {
      I386PairWideHalves(*lo, *hi);
    }
    return;
  }
  if (I386WideMergeTmpHasStackHome(node)) {
    I386LoadWideFromMergeTmp(rv, node, lo, hi);
    return;
  }
  *lo = TargetGetLoweredNode(node);
  *hi = I386GetLoweredHi(node);
  if (*hi != NULL && *lo != NULL && *hi != *lo) {
    I386PairWideHalves(*lo, *hi);
  }
  if (*hi == NULL) {
    *hi = X86CgZero(rv);
  }
}

bool X86I386TypeIsWideInt(TypeRecord* type) {
  return type != NULL && !TypeIsFloatingPoint(type) &&
         !TypeIsPointerOrArray(type) && !TypeIsStructOrUnion(type) &&
         !TypeIsVector(type) && type->size == 8;
}

bool X86I386NodeIsWideInt(IRNode* node) {
  return node != NULL && X86I386TypeIsWideInt(node->type);
}

static Symbol* I386GetLibcallSymbol(const char* name) {
  static struct {
    const char* name;
    Symbol* sym;
  } cache[8];
  static int count = 0;
  for (int i = 0; i < count; i++) {
    if (strcmp(cache[i].name, name) == 0) {
      return cache[i].sym;
    }
  }
  TypeRecord* base = NewTypeRecord(kTypeInt, kQualPlain);
  TypeRecord* func = NewFunctionTypeRecord();
  func->info.function.unknown_args = true;
  TypeRecordChain(func, base);
  Symbol* sym = NewSymbol(name, func, STO(extern));
  assert(count < (int)(sizeof(cache) / sizeof(cache[0])));
  cache[count].name = name;
  cache[count].sym = sym;
  count++;
  return sym;
}

static TargetInstruction* I386WideLibCall(X86Generator* rv, Generator* gen,
                                          IRNode* node, const char* name) {
  IRNode* a = node->inputs.value.p[0];
  IRNode* b = node->inputs.value.p[1];
  TargetInstruction *alo, *ahi, *blo, *bhi;
  I386MaterializeWide(rv, a, &alo, &ahi);
  I386MaterializeWide(rv, b, &blo, &bhi);

  // cdecl: first param (a) at the lowest stack address, then b.
  size_t stack = 0;
  size_t a_lo = stack;
  stack += 4;
  size_t a_hi = stack;
  stack += 4;
  size_t b_lo = stack;
  stack += 4;
  size_t b_hi = stack;
  stack += 4;
  if ((stack & 15) != 0) {
    stack = (stack + 15) & ~(size_t)15;
  }
  if (stack > 0) {
    TargetInstruction* newsp =
        X86CgAddImmediate(rv, X86CgStackPointer(rv), -(int64_t)stack);
    TargetSetDest(newsp, X86CgStackPointer(rv));
  }
  X86CgPushArg(rv, node, alo, a_lo);
  X86CgPushArg(rv, node, ahi, a_hi);
  X86CgPushArg(rv, node, blo, b_lo);
  X86CgPushArg(rv, node, bhi, b_hi);

  TargetInstruction* sym =
      X86CgEmitSymbol(rv, X86CgGetSymbol(rv, NULL, I386GetLibcallSymbol(name)));
  TargetInstruction* call =
      X86CgEmit(rv, X86CgNewInstruction2(X86_OP(call), sym, NULL));

  if (stack > 0) {
    TargetInstruction* newsp =
        X86CgAddImmediate(rv, X86CgStackPointer(rv), (int64_t)stack);
    TargetSetDest(newsp, X86CgStackPointer(rv));
  }

  X86I386CaptureWideCallResult(rv, node, &call);
  TargetInstruction* result = TargetSetLoweredNode(node, call);
  I386RouteWideResultToDest(rv, gen, node, call, I386GetLoweredHi(node));
  return result;
}

static TargetInstruction* I386WideShiftLibCall(X86Generator* rv,
                                               Generator* gen, IRNode* node,
                                               const char* name) {
  TargetInstruction *lo, *hi;
  I386MaterializeWide(rv, node->inputs.value.p[0], &lo, &hi);
  TargetInstruction* count =
      X86CgMaterialize(rv, node->inputs.value.p[1]);

  // The libgcc shift ABI is (DI value, SI count).  Reserve a 16-byte outgoing
  // area to retain the backend's call-site alignment while placing the three
  // argument words at offsets 0, 4 and 8.
  TargetInstruction* newsp =
      X86CgAddImmediate(rv, X86CgStackPointer(rv), -16);
  TargetSetDest(newsp, X86CgStackPointer(rv));
  X86CgPushArg(rv, node, lo, 0);
  X86CgPushArg(rv, node, hi, 4);
  X86CgPushArg(rv, node, count, 8);

  TargetInstruction* sym =
      X86CgEmitSymbol(rv, X86CgGetSymbol(rv, NULL, I386GetLibcallSymbol(name)));
  TargetInstruction* call =
      X86CgEmit(rv, X86CgNewInstruction2(X86_OP(call), sym, NULL));

  newsp = X86CgAddImmediate(rv, X86CgStackPointer(rv), 16);
  TargetSetDest(newsp, X86CgStackPointer(rv));
  X86I386CaptureWideCallResult(rv, node, &call);
  TargetInstruction* result = TargetSetLoweredNode(node, call);
  I386RouteWideResultToDest(rv, gen, node, call, I386GetLoweredHi(node));
  return result;
}

TargetInstruction* X86I386LowerExpression(X86Generator* rv, Generator* gen,
                                            IRNode* node) {
  TargetInstruction* lo = NULL;
  TargetInstruction* hi = NULL;

  switch (node->opcode) {
    case IR_OP(const64):
    case IR_OP(consta):
      I386MaterializeWide(rv, node, &lo, &hi);
      break;

    case IR_OP(movi): {
      TargetInstruction *alo, *ahi;
      I386MaterializeWide(rv, node->inputs.value.p[0], &alo, &ahi);
      lo = I386WideMov(rv, alo);
      hi = I386WideMov(rv, ahi);
      break;
    }

    case IR_OP(addi): {
      TargetInstruction *alo, *ahi, *blo, *bhi;
      I386MaterializeWide(rv, node->inputs.value.p[0], &alo, &ahi);
      I386MaterializeWide(rv, node->inputs.value.p[1], &blo, &bhi);
      lo = X86CgEmit(rv, X86CgNewInstruction2(X86_OP(addl), alo, blo));
      hi = X86CgEmit(rv, X86CgNewInstruction2(X86_OP(adcl), ahi, bhi));
      break;
    }

    case IR_OP(subi): {
      TargetInstruction *alo, *ahi, *blo, *bhi;
      I386MaterializeWide(rv, node->inputs.value.p[0], &alo, &ahi);
      I386MaterializeWide(rv, node->inputs.value.p[1], &blo, &bhi);
      lo = X86CgEmit(rv, X86CgNewInstruction2(X86_OP(subl), alo, blo));
      hi = X86CgEmit(rv, X86CgNewInstruction2(X86_OP(sbbl), ahi, bhi));
      break;
    }

    case IR_OP(andi):
    case IR_OP(ori):
    case IR_OP(xori): {
      X86Opcode op = node->opcode == IR_OP(andi)   ? X86_OP(and)
                     : node->opcode == IR_OP(ori)  ? X86_OP(or)
                                                   : X86_OP(xor);
      TargetInstruction *alo, *ahi, *blo, *bhi;
      I386MaterializeWide(rv, node->inputs.value.p[0], &alo, &ahi);
      I386MaterializeWide(rv, node->inputs.value.p[1], &blo, &bhi);
      lo = X86CgEmit(rv, X86CgNewInstruction2(op, alo, blo));
      hi = X86CgEmit(rv, X86CgNewInstruction2(op, ahi, bhi));
      break;
    }

    case IR_OP(muli):
      return I386WideLibCall(rv, gen, node, "__muldi3");

    case IR_OP(divi):
      return I386WideLibCall(
          rv, gen, node,
          TypeIsUnsigned(node->type) ? "__udivdi3" : "__divdi3");

    case IR_OP(modi):
      return I386WideLibCall(
          rv, gen, node,
          TypeIsUnsigned(node->type) ? "__umoddi3" : "__moddi3");

    case IR_OP(onescomp): {
      TargetInstruction *alo, *ahi;
      I386MaterializeWide(rv, node->inputs.value.p[0], &alo, &ahi);
      lo = X86CgEmit(rv, X86CgNewInstruction1(X86_OP(not), alo));
      hi = X86CgEmit(rv, X86CgNewInstruction1(X86_OP(not), ahi));
      break;
    }

    case IR_OP(negi): {
      TargetInstruction *alo, *ahi;
      I386MaterializeWide(rv, node->inputs.value.p[0], &alo, &ahi);
      TargetInstruction* nlo =
          X86CgEmit(rv, X86CgNewInstruction1(X86_OP(not), alo));
      TargetInstruction* nhi =
          X86CgEmit(rv, X86CgNewInstruction1(X86_OP(not), ahi));
      lo = X86CgEmit(rv, X86CgNewInstruction2(X86_OP(addl), nlo, I386Const32(rv, 1)));
      hi = X86CgEmit(rv, X86CgNewInstruction2(X86_OP(adcl), nhi, I386Const32(rv, 0)));
      break;
    }

    case IR_OP(lsli):
    case IR_OP(lsri):
    case IR_OP(asri): {
      IRNode* amount = node->inputs.value.p[1];
      if (!IRIsConst(amount)) {
        return I386WideShiftLibCall(
            rv, gen, node,
            node->opcode == IR_OP(lsli)   ? "__ashldi3"
            : node->opcode == IR_OP(lsri) ? "__lshrdi3"
                                           : "__ashrdi3");
      }
      TargetInstruction *alo, *ahi;
      I386MaterializeWide(rv, node->inputs.value.p[0], &alo, &ahi);
      int n = (int)(((IRConstant*)amount)->value.ivalue) & 63;
      if (n == 0) {
        lo = I386WideMov(rv, alo);
        hi = I386WideMov(rv, ahi);
      } else if (node->opcode == IR_OP(lsli)) {
        if (n < 32) {
          TargetInstruction* hi_hi =
              X86CgEmit(rv, X86CgNewInstruction2(X86_OP(shll), ahi,
                                                 I386Const32(rv, n)));
          TargetInstruction* hi_lo = X86CgEmit(
              rv, X86CgNewInstruction2(X86_OP(shrl), alo, I386Const32(rv, 32 - n)));
          hi = X86CgEmit(rv, X86CgNewInstruction2(X86_OP(or), hi_hi, hi_lo));
          lo = X86CgEmit(rv, X86CgNewInstruction2(X86_OP(shll), alo,
                                                  I386Const32(rv, n)));
        } else {
          hi = (n == 32)
                   ? I386WideMov(rv, alo)
                   : X86CgEmit(rv, X86CgNewInstruction2(
                                        X86_OP(shll), alo,
                                        I386Const32(rv, n - 32)));
          lo = X86CgZero(rv);
        }
      } else {
        X86Opcode high_op =
            (node->opcode == IR_OP(asri)) ? X86_OP(sarl) : X86_OP(shrl);
        if (n < 32) {
          TargetInstruction* lo_lo =
              X86CgEmit(rv, X86CgNewInstruction2(X86_OP(shrl), alo,
                                                   I386Const32(rv, n)));
          TargetInstruction* lo_hi = X86CgEmit(
              rv, X86CgNewInstruction2(X86_OP(shll), ahi, I386Const32(rv, 32 - n)));
          lo = X86CgEmit(rv, X86CgNewInstruction2(X86_OP(or), lo_lo, lo_hi));
          hi = X86CgEmit(rv, X86CgNewInstruction2(high_op, ahi,
                                                  I386Const32(rv, n)));
        } else if (node->opcode == IR_OP(asri)) {
          lo = (n == 32)
                   ? I386WideMov(rv, ahi)
                   : X86CgEmit(rv, X86CgNewInstruction2(
                                        X86_OP(sarl), ahi,
                                        I386Const32(rv, n - 32)));
          hi = X86CgEmit(rv, X86CgNewInstruction2(X86_OP(sarl), ahi,
                                                  I386Const32(rv, 31)));
        } else {
          lo = (n == 32)
                   ? I386WideMov(rv, ahi)
                   : X86CgEmit(rv, X86CgNewInstruction2(
                                        X86_OP(shrl), ahi,
                                        I386Const32(rv, n - 32)));
          hi = X86CgZero(rv);
        }
      }
      break;
    }

    default:
      return NULL;
  }

  I386PairWideHalves(lo, hi);
  I386SetLoweredHi(node, hi);
  TargetInstruction* result = TargetSetLoweredNode(node, lo);
  I386RouteWideResultToDest(rv, gen, node, lo, hi);
  return result;
}

TargetInstruction* X86I386LowerLoad(X86Generator* rv, Generator* gen,
                                    IRNode* node) {
  if (node->opcode != IR_OP(load64)) {
    return NULL;
  }
  IRNode* addr_node = node->inputs.value.p[0];
  TargetInstruction* addr;
  TargetInstruction* offset;
  if (!X86CgGetRegAndOffset(rv, addr_node, &addr, &offset)) {
    TargetInstruction* lo = addr;
    TargetInstruction* hi = X86CgZero(rv);
    I386SetLoweredHi(node, hi);
    TargetSetLoweredNode(node, lo);
    I386RouteWideResultToDest(rv, gen, node, lo, hi);
    return lo;
  }
  TargetInstruction *base, *off_lo, *off_hi;
  I386WideMemoryOffsets(rv, addr, offset, &base, &off_lo, &off_hi);
  TargetInstruction* lo_base = I386WideMov(rv, base);
  TargetInstruction* hi_base = I386WideMov(rv, base);
  TargetInstruction* lo = X86CgEmit(
      rv, X86CgNewInstruction2(X86_OP(loadl), lo_base, off_lo));
  TargetInstruction* hi = X86CgEmit(
      rv, X86CgNewInstruction3(X86_OP(loadl), hi_base, off_hi, base));
  I386PairWideHalves(lo, hi);
  I386SetLoweredHi(node, hi);
  TargetInstruction* result = TargetSetLoweredNode(node, lo);
  I386RouteWideResultToDest(rv, gen, node, lo, hi);
  return result;
}

TargetInstruction* X86I386LowerStore(X86Generator* rv, Generator* gen,
                                     IRNode* node) {
  if (node->opcode != IR_OP(store64)) {
    return NULL;
  }
  IRNode* addr_node = node->inputs.value.p[0];
  IRNode* src_node = node->inputs.value.p[1];
  TargetInstruction *src_lo, *src_hi;
  I386MaterializeWide(rv, src_node, &src_lo, &src_hi);
  TargetInstruction* addr;
  TargetInstruction* offset;
  bool found_address = X86CgGetRegAndOffset(rv, addr_node, &addr, &offset);
  assert(found_address);
  (void)found_address;
  TargetInstruction *base, *off_lo, *off_hi;
  I386WideMemoryOffsets(rv, addr, offset, &base, &off_lo, &off_hi);
  X86CgEmit(rv, X86CgNewInstruction3(X86_OP(storel), src_lo, base, off_lo));
  TargetInstruction* last =
      X86CgEmit(rv, X86CgNewInstruction3(X86_OP(storel), src_hi, base, off_hi));
  return TargetSetLoweredNode(node, last);
}

static TargetInstruction* I386WideSetcc(X86Generator* rv, X86Opcode setcc,
                                        TargetInstruction* operand) {
  return X86CgEmit(rv, X86CgNewInstruction1(setcc, operand));
}

static TargetInstruction* I386WideSetcc2(X86Generator* rv, X86Opcode setcc,
                                         TargetInstruction* lhs,
                                         TargetInstruction* rhs) {
  return X86CgEmit(rv, X86CgNewInstruction2(setcc, lhs, rhs));
}

static TargetInstruction* I386WideLess(X86Generator* rv,
                                       TargetInstruction* lhs_lo,
                                       TargetInstruction* lhs_hi,
                                       TargetInstruction* rhs_lo,
                                       TargetInstruction* rhs_hi,
                                       bool is_unsigned) {
  TargetInstruction* hi_less =
      I386WideSetcc2(rv, is_unsigned ? X86_OP(setb) : X86_OP(setl),
                    lhs_hi, rhs_hi);
  TargetInstruction* hi_equal =
      I386WideSetcc2(rv, X86_OP(sete), lhs_hi, rhs_hi);
  // Once the high words are equal, the low-word comparison is always
  // unsigned, even for a signed 64-bit relation.
  TargetInstruction* lo_less =
      I386WideSetcc2(rv, X86_OP(setb), lhs_lo, rhs_lo);
  TargetInstruction* equal_and_less =
      X86CgEmit(rv, X86CgNewInstruction2(X86_OP(and), hi_equal, lo_less));
  return X86CgEmit(
      rv, X86CgNewInstruction2(X86_OP(or), hi_less, equal_and_less));
}

static TargetInstruction* I386WideCompareResult(
    X86Generator* rv, IRNode* lhs, IRNode* rhs, IROpcode opcode,
    bool is_unsigned) {
  TargetInstruction *alo, *ahi, *blo, *bhi;
  I386MaterializeWide(rv, lhs, &alo, &ahi);
  I386MaterializeWide(rv, rhs, &blo, &bhi);

  enum { CEQ, CNE, CLT, CGE, CGT, CLE } cat;
  switch (opcode) {
    case IR_OP(cmpeqi):
    case IR_OP(cmpeqa):
      cat = CEQ;
      break;
    case IR_OP(cmpnei):
    case IR_OP(cmpnea):
      cat = CNE;
      break;
    case IR_OP(cmplti):
    case IR_OP(cmplta):
      cat = CLT;
      break;
    case IR_OP(cmpgei):
    case IR_OP(cmpgea):
      cat = CGE;
      break;
    case IR_OP(cmpgti):
    case IR_OP(cmpgta):
      cat = CGT;
      break;
    case IR_OP(cmplei):
    case IR_OP(cmplea):
      cat = CLE;
      break;
    default:
      assert(false);
      cat = CEQ;
      break;
  }

  if (cat == CEQ || cat == CNE) {
    TargetInstruction* t1 =
        X86CgEmit(rv, X86CgNewInstruction2(X86_OP(xor), alo, blo));
    TargetInstruction* t2 =
        X86CgEmit(rv, X86CgNewInstruction2(X86_OP(xor), ahi, bhi));
    TargetInstruction* t3 =
        X86CgEmit(rv, X86CgNewInstruction2(X86_OP(or), t1, t2));
    return I386WideSetcc(rv, cat == CEQ ? X86_OP(sete) : X86_OP(setne), t3);
  }

  bool swap = cat == CGT || cat == CLE;
  TargetInstruction* less =
      swap ? I386WideLess(rv, blo, bhi, alo, ahi, is_unsigned)
           : I386WideLess(rv, alo, ahi, blo, bhi, is_unsigned);
  if (cat == CGE || cat == CLE) {
    less = X86CgEmit(
        rv, X86CgNewInstruction2(X86_OP(xor), less, I386Const32(rv, 1)));
  }
  return less;
}

static TargetInstruction* I386EmitConditionalBranch(
    X86Generator* rv, X86Opcode branch, TargetInstruction* operand,
    IRNode* target_node) {
  TargetInstruction* inst =
      X86CgEmit(rv, X86CgNewInstruction1(branch, operand));
  TargetInstruction* target = target_node->data.ptr;
  if (target == NULL) {
    VectorAppend(&rv->base.fixups, NewBranchFixup(inst, target_node, 1));
  } else {
    inst->operand[1] = target;
  }
  return inst;
}

TargetInstruction* X86I386LowerCompareBranch(X86Generator* rv, IRNode* expr,
                                             IRNode* target_node, bool btrue,
                                             bool reverse) {
  IRNode* lhs = expr->inputs.value.p[0];
  IRNode* rhs = expr->inputs.value.p[1];
  if (reverse) {
    IRNode* tmp = lhs;
    lhs = rhs;
    rhs = tmp;
  }
  TargetInstruction* result = I386WideCompareResult(
      rv, lhs, rhs, expr->opcode, IRComparisonIsUnsigned(expr));
  X86Opcode branch = btrue ? X86_OP(jnz) : X86_OP(jz);
  return I386EmitConditionalBranch(rv, branch, result, target_node);
}

TargetInstruction* X86I386LowerBranchOnWideValue(X86Generator* rv,
                                                 IRNode* expr,
                                                 IRNode* target_node,
                                                 bool btrue) {
  TargetInstruction *lo, *hi;
  I386MaterializeWide(rv, expr, &lo, &hi);
  TargetInstruction* combined =
      X86CgEmit(rv, X86CgNewInstruction2(X86_OP(or), lo, hi));
  X86Opcode branch = btrue ? X86_OP(jnz) : X86_OP(jz);
  return I386EmitConditionalBranch(rv, branch, combined, target_node);
}

TargetInstruction* X86I386LowerComparison(X86Generator* rv, Generator* gen,
                                          IRNode* node) {
  IRNode* op1 = node->inputs.value.p[0];
  if (!X86I386TypeIsWideInt(op1->type)) {
    return NULL;
  }
  bool is_unsigned = IRComparisonIsUnsigned(node);
  IRNode* lhs = node->inputs.value.p[0];
  IRNode* rhs = node->inputs.value.p[1];
  TargetInstruction* result =
      I386WideCompareResult(rv, lhs, rhs, node->opcode, is_unsigned);
  TargetInstruction* dest = X86CgGetDestInstruction(rv, gen, node);
  if (dest != NULL) {
    result = X86CgSetDestOrMove(rv, result, dest, X86_OP(mv));
  }
  return TargetSetLoweredNode(node, result);
}

TargetInstruction* X86I386LowerResult(X86Generator* rv, Generator* gen,
                                      IRNode* node) {
  if (node->opcode != IR_OP(resulti) || !X86I386NodeIsWideInt(node)) {
    return NULL;
  }
  TargetInstruction *lo, *hi;
  I386MaterializeWide(rv, node->inputs.value.p[0], &lo, &hi);
  TargetInstruction* eax =
      X86CgEmitSymbol(rv, X86CgNewInstruction(X86_OP(resulti)));
  X86CgSetDestOrMove(rv, lo, eax, X86_OP(mv));
  TargetInstruction* edx =
      X86CgEmitSymbol(rv, X86CgNewInstruction(X86_OP(resulth)));
  TargetInstruction* res_hi = X86CgSetDestOrMove(rv, hi, edx, X86_OP(mv));
  return TargetSetLoweredNode(node, res_hi);
}

void X86I386CaptureWideCallResult(X86Generator* rv, IRNode* node,
                                  TargetInstruction** call_inout) {
  if (!X86I386NodeIsWideInt(node) || call_inout == NULL ||
      *call_inout == NULL) {
    return;
  }
  TargetInstruction* original_call = *call_inout;
  TargetInstruction* lo = original_call;
  TargetInstruction* hi =
      X86CgEmitSymbol(rv, X86CgNewInstruction(X86_OP(resulth)));
  int off = I386AllocateWideMergeHome(rv);
  node->data.ivalue = off;
  TargetInstruction* fp = X86CgFramePointer(rv);
  X86CgEmit(rv, X86CgNewInstruction3(
                    X86_OP(storel), lo, fp,
                    X86CgGetIntConstant(rv, NULL, kTargetType32Bit, off)));
  X86CgEmit(rv, X86CgNewInstruction3(
                    X86_OP(storel), hi, fp,
                    X86CgGetIntConstant(rv, NULL, kTargetType32Bit, off + 4)));
  I386SetLoweredHi(node, hi);
  *call_inout = lo;
}

TargetInstruction* X86I386MaterializeWideArg(X86Generator* rv, IRNode* arg_node,
                                             size_t offset) {
  TargetInstruction *lo, *hi;
  I386MaterializeWide(rv, arg_node, &lo, &hi);
  X86CgPushArg(rv, arg_node, lo, offset);
  X86CgPushArg(rv, arg_node, hi, offset + 4);
  return lo;
}

TargetInstruction* X86I386LowerPushArg(X86Generator* rv, Generator* gen,
                                       IRNode* node) {
  if (!X86I386NodeIsWideInt(node) &&
      !X86I386NodeIsWideInt(node->inputs.value.p[0])) {
    return NULL;
  }
  TargetInstruction *lo, *hi;
  I386MaterializeWide(rv, node->inputs.value.p[0], &lo, &hi);
  I386PairWideHalves(lo, hi);
  I386SetLoweredHi(node, hi);
  return TargetSetLoweredNode(node, lo);
}

TargetInstruction* X86I386LowerVaArg(X86Generator* rv, IRNode* node) {
  int64_t arg_size = ((IRConstant*)node->inputs.value.p[1])->value.ivalue;
  if (arg_size != 8 || !X86I386NodeIsWideInt(node)) {
    return NULL;
  }
  IRNode* ap_node = node->inputs.value.p[0];
  TargetInstruction* ap_addr;
  TargetInstruction* ap_offset;
  X86CgGetRegAndOffset(rv, ap_node, &ap_addr, &ap_offset);
  TargetInstruction* ap_load = X86CgEmit(
      rv, X86CgNewInstruction2(X86_OP(loadl), ap_addr, ap_offset));
  TargetInstruction* lo_base = I386WideMov(rv, ap_load);
  TargetInstruction* hi_base = I386WideMov(rv, ap_load);
  TargetInstruction* lo = X86CgEmit(
      rv, X86CgNewInstruction2(X86_OP(loadl), lo_base,
                             X86CgGetIntConstant(rv, NULL, kTargetType32Bit, 0)));
  TargetInstruction* hi = X86CgEmit(
      rv, X86CgNewInstruction3(
              X86_OP(loadl), hi_base,
              X86CgGetIntConstant(rv, NULL, kTargetType32Bit, 4), ap_load));
  TargetInstruction* next_ap = X86CgAddImmediate(rv, ap_load, 8);
  X86CgEmit(rv, X86CgNewInstruction3(X86_OP(storel), next_ap, ap_addr, ap_offset));
  I386SetLoweredHi(node, hi);
  I386PairWideHalves(lo, hi);
  return TargetSetLoweredNode(node, lo);
}

static TargetInstruction* I386FinishSignExtend(X86Generator* rv, Generator* gen,
                                               IRNode* node,
                                               TargetInstruction* lo) {
  if (X86I386NodeIsWideInt(node)) {
    TargetInstruction* hi = X86CgEmit(
        rv, X86CgNewInstruction2(X86_OP(sarl), lo, I386Const32(rv, 31)));
    I386PairWideHalves(lo, hi);
    I386SetLoweredHi(node, hi);
    lo = TargetSetLoweredNode(node, lo);
    I386RouteWideResultToDest(rv, gen, node, lo, hi);
    return lo;
  }
  TargetInstruction* dest = X86CgGetDestInstruction(rv, gen, node);
  if (dest != NULL) {
    lo = X86CgSetDestOrMove(rv, lo, dest, X86_OP(mv));
  }
  return TargetSetLoweredNode(node, lo);
}

TargetInstruction* X86I386LowerSignExtend(X86Generator* rv, Generator* gen,
                                          IRNode* node) {
  IRNode* input = node->inputs.value.p[0];
  TargetInstruction* value;
  if (X86I386NodeIsWideInt(input)) {
    TargetInstruction* unused_hi;
    I386MaterializeWide(rv, input, &value, &unused_hi);
  } else {
    value = X86CgMaterialize(rv, input);
  }
  IRConstant* diff_value = node->inputs.value.p[1];
  int64_t diff = diff_value->value.ivalue;
  if (diff > 0) {
    if (diff == 32) {
      return I386FinishSignExtend(rv, gen, node, I386WideMov(rv, value));
    }
    TargetInstruction* lsl = X86CgEmit(
        rv, X86CgNewInstruction2(X86_OP(shll), value,
                                  I386Const32(rv, (int32_t)diff)));
    TargetInstruction* sar = X86CgEmit(
        rv, X86CgNewInstruction2(X86_OP(sarl), lsl,
                                  I386Const32(rv, (int32_t)diff)));
    return I386FinishSignExtend(rv, gen, node, sar);
  }
  diff = -diff;
  if (diff == 32 || diff == 0) {
    return I386FinishSignExtend(rv, gen, node, I386WideMov(rv, value));
  }
  TargetInstruction* lsl = X86CgEmit(
      rv, X86CgNewInstruction2(X86_OP(shll), value, I386Const32(rv, (int32_t)diff)));
  TargetInstruction* sar = X86CgEmit(
      rv, X86CgNewInstruction2(X86_OP(sarl), lsl, I386Const32(rv, (int32_t)diff)));
  return I386FinishSignExtend(rv, gen, node, sar);
}

TargetInstruction* X86I386LowerZeroExtend(X86Generator* rv, Generator* gen,
                                          IRNode* node) {
  IRNode* input = node->inputs.value.p[0];
  TargetInstruction* value;
  if (X86I386NodeIsWideInt(input)) {
    TargetInstruction* unused_hi;
    I386MaterializeWide(rv, input, &value, &unused_hi);
  } else {
    value = X86CgMaterialize(rv, input);
  }
  IRConstant* diff_value = node->inputs.value.p[1];
  int64_t diff = diff_value->value.ivalue;
  if (X86I386NodeIsWideInt(node)) {
    TargetInstruction* lo = value;
    if (diff > 0 && diff < 32) {
      TargetInstruction* lsl = X86CgEmit(
          rv, X86CgNewInstruction2(X86_OP(shll), value,
                                   I386Const32(rv, (int32_t)diff)));
      lo = X86CgEmit(rv, X86CgNewInstruction2(X86_OP(shrl), lsl,
                                              I386Const32(rv, (int32_t)diff)));
    }
    TargetInstruction* hi = X86CgZero(rv);
    I386PairWideHalves(lo, hi);
    I386SetLoweredHi(node, hi);
    lo = TargetSetLoweredNode(node, lo);
    I386RouteWideResultToDest(rv, gen, node, lo, hi);
    return lo;
  }
  if (diff <= 0 || diff >= 32) {
    return TargetSetLoweredNode(node, value);
  }
  TargetInstruction* lsl = X86CgEmit(
      rv, X86CgNewInstruction2(X86_OP(shll), value, I386Const32(rv, (int32_t)diff)));
  TargetInstruction* lsr = X86CgEmit(
      rv, X86CgNewInstruction2(X86_OP(shrl), lsl, I386Const32(rv, (int32_t)diff)));
  TargetInstruction* dest = X86CgGetDestInstruction(rv, gen, node);
  if (dest != NULL) {
    lsr = X86CgSetDestOrMove(rv, lsr, dest, X86_OP(mv));
  }
  return TargetSetLoweredNode(node, lsr);
}

TargetInstruction* X86I386LowerCast(X86Generator* rv, Generator* gen,
                                    IRNode* node) {
  if (!X86I386NodeIsWideInt(node)) {
    return NULL;
  }
  TargetInstruction *lo, *hi;
  I386MaterializeWide(rv, node->inputs.value.p[0], &lo, &hi);
  I386PairWideHalves(lo, hi);
  I386SetLoweredHi(node, hi);
  TargetSetLoweredNode(node, lo);
  I386RouteWideResultToDest(rv, gen, node, lo, hi);
  return lo;
}

TargetInstruction* X86I386LowerIncDec(X86Generator* rv, Generator* gen,
                                      IRNode* node, bool increment) {
  if (node->opcode != IR_OP(inc64) && node->opcode != IR_OP(uinc64) &&
      node->opcode != IR_OP(dec64) && node->opcode != IR_OP(udec64)) {
    return NULL;
  }
  IRNode* addr_node = node->inputs.value.p[0];
  TargetInstruction* addr;
  TargetInstruction* offset;
  bool found_address = X86CgGetRegAndOffset(rv, addr_node, &addr, &offset);
  assert(found_address);
  (void)found_address;
  TargetInstruction *base, *off_lo, *off_hi;
  I386WideMemoryOffsets(rv, addr, offset, &base, &off_lo, &off_hi);
  TargetInstruction* lo_base = I386WideMov(rv, base);
  TargetInstruction* hi_base = I386WideMov(rv, base);
  TargetInstruction* lo =
      X86CgEmit(rv, X86CgNewInstruction2(X86_OP(loadl), lo_base, off_lo));
  TargetInstruction* hi =
      X86CgEmit(rv, X86CgNewInstruction3(X86_OP(loadl), hi_base, off_hi, base));
  IRNode* amount_node = node->inputs.value.p[1];
  int64_t amt = IRIsIntConst(amount_node)
                    ? ((IRConstant*)amount_node)->value.ivalue
                    : 1;
  TargetInstruction* new_lo;
  TargetInstruction* new_hi;
  if (increment) {
    new_lo = X86CgEmit(rv, X86CgNewInstruction2(X86_OP(addl), lo,
                                              I386Const32(rv, (int32_t)amt)));
    new_hi = X86CgEmit(rv, X86CgNewInstruction2(X86_OP(adcl), hi, I386Const32(rv, 0)));
  } else {
    new_lo = X86CgEmit(rv, X86CgNewInstruction2(X86_OP(subl), lo,
                                              I386Const32(rv, (int32_t)amt)));
    new_hi = X86CgEmit(rv, X86CgNewInstruction2(X86_OP(sbbl), hi, I386Const32(rv, 0)));
  }
  X86CgEmit(rv, X86CgNewInstruction3(X86_OP(storel), new_lo, base, off_lo));
  X86CgEmit(rv, X86CgNewInstruction3(X86_OP(storel), new_hi, base, off_hi));
  I386SetLoweredHi(node, new_hi);
  return TargetSetLoweredNode(node, new_lo);
}

static void I386ReportUnsupported(Generator* gen, const char* msg) {
  const char* file = "<i386>";
  int line = 0;
  if (gen->func != NULL && gen->func->info.function.symbol != NULL) {
    Symbol* sym = gen->func->info.function.symbol;
    int start = 0;
    int end = 0;
    DecodeSourceLocation(sym->location, &file, &line, &start, &end);
  }
  ReportError(file, line, "%s", msg);
}

bool X86I386ValidateFunction(X86Generator* rv, Generator* gen) {
  (void)rv;
  for (IRNode* node = GeneratorFirstInstruction(gen); node != NULL;
       node = IRNext(node)) {
    switch (node->opcode) {
      case IR_OP(atomic_load):
      case IR_OP(atomic_store):
      case IR_OP(atomic_fetch_add):
      case IR_OP(atomic_fetch_sub):
      case IR_OP(atomic_add_fetch):
      case IR_OP(atomic_sub_fetch):
      case IR_OP(atomic_compare_exchange_bool):
      case IR_OP(atomic_compare_exchange_val):
      case IR_OP(atomic_compare_exchange_n):
        if (node->type != NULL && node->type->size == 8) {
          I386ReportUnsupported(
              gen, "8-byte atomic operations are not supported on i386");
          return false;
        }
        break;
      default:
        break;
    }
    if (node->opcode == IR_OP(asm) && node->type != NULL &&
        X86I386TypeIsWideInt(node->type)) {
      I386ReportUnsupported(
          gen, "inline asm with 64-bit integer operands is not supported on i386");
      return false;
    }
  }
  return true;
}
