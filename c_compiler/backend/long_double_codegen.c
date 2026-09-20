//
//  long_double_codegen.c
//
//  Lower distinct long double operations to pointer-based runtime helpers.
//  Values are 16-byte memory objects, matching the aggregate ABI used for
//  arguments and returns on aarch64, x86_64, and RISC-V 64.
//

#include "long_double_codegen.h"

#include <string.h>

#include "compiler.h"
#include "expr_codegen.h"
#include "fp_extended.h"
#include "symbol.h"
#include "symbol_table.h"

int LongDoubleRuntimeFormat(void) {
  if (compiler == NULL) {
    return kFPExtFormatFloat64;
  }
  return compiler->long_double_format;
}

static TypeRecord* BitsPointerType(void) {
  TypeRecord* ull = NewTypeRecordWithSize(kTypeLongLong | kTypeUnsigned,
                                          kQualPlain);
  TypeRecord* ptr = NewPointerTo(kQualPlain, ull);
  TypeRecordDelete(ull);
  return ptr;
}

static Symbol* GetLongDoubleHelper(const char* name, TypeRecord* return_type,
                                   int num_pointer_args,
                                   TypeRecord* extra_arg_type) {
  String s;
  StringInit(&s, name);
  Symbol* symbol = FindGlobalSymbol(&s);
  StringDestruct(&s);
  if (symbol != NULL) {
    return symbol;
  }
  TypeRecord* func = NewFunctionTypeRecord();
  TypeRecordChain(func, TypeRecordCopy(return_type));
  TypeRecord* ptr = BitsPointerType();
  for (int i = 0; i < num_pointer_args; i++) {
    TypeRecord* arg_type = TypeRecordCopy(ptr);
    Symbol* arg = NewSymbol("", arg_type, STO(implicit));
    arg->flags.is_argument = true;
    arg->flags.invented = true;
    VectorAppend(&func->info.function.prototype, arg);
  }
  if (extra_arg_type != NULL) {
    Symbol* arg = NewSymbol("", TypeRecordCopy(extra_arg_type), STO(implicit));
    arg->flags.is_argument = true;
    arg->flags.invented = true;
    VectorAppend(&func->info.function.prototype, arg);
  }
  TypeRecordDelete(ptr);
  symbol = NewSymbol(name, func, STO(extern));
  symbol->flags.invented = true;
  symbol->flags.is_forward_declared = true;
  SyntaxAddSymbol(&compiler->syntax, symbol);
  return symbol;
}

IRNode* LongDoubleObjectAddress(Generator* gen, IRNode* value,
                                TypeRecord* type) {
  if (value == NULL) {
    return NULL;
  }
  if (value->opcode == IR_OP(literalref) || value->opcode == IR_OP(addressof) ||
      value->opcode == IR_OP(adda) || value->opcode == IR_OP(suba) ||
      value->opcode == IR_OP(loada) ||
      TypeIsPointer(value->type)) {
    return value;
  }
  if (IRIsVariable(value) || value->opcode == IR_OP(argument) ||
      value->opcode == IR_OP(localvar) || value->opcode == IR_OP(tempvar) ||
      value->opcode == IR_OP(staticvar) || value->opcode == IR_OP(externvar)) {
    IRNode* addr = GeneratorEmit(gen, NewIR1(IR_OP(addressof), value));
    return IRSetType(addr, NewPointerTo(kQualPlain, type));
  }
  IRNode* addr = GeneratorEmit(gen, NewIR1(IR_OP(addressof), value));
  return IRSetType(addr, NewPointerTo(kQualPlain, type));
}

static IRNode* NewLongDoubleTemp(Generator* gen, TypeRecord* type) {
  Symbol* tmp = SyntaxNewTemporary(gen->syntax, type);
  tmp->flags.address_taken = true;
  return GeneratorGetVariable(gen, tmp);
}

IRNode* LoadLongDoubleFromAddress(Generator* gen, IRNode* addr,
                                  TypeRecord* type) {
  IRNode* dest = NewLongDoubleTemp(gen, type);
  IRNode* dest_addr = LongDoubleObjectAddress(gen, dest, type);
  GeneratorEmit(
      gen, NewIR3(IR_OP(memcpy), dest_addr, addr,
                  GeneratorGetIntConstant(gen, NULL, type->size)));
  return dest;
}

IRNode* StoreLongDoubleToAddress(Generator* gen, IRNode* dest_addr,
                                 IRNode* value, TypeRecord* type) {
  IRNode* src = LongDoubleObjectAddress(gen, value, type);
  return GeneratorEmit(
      gen, NewIR3(IR_OP(memcpy), dest_addr, src,
                  GeneratorGetIntConstant(gen, NULL, type->size)));
}

IRNode* GenerateLongDoubleConstant(Generator* gen, TypeRecord* type,
                                   double value) {
  FPBits bits = FPBitsFromF64(value, LongDoubleRuntimeFormat());
  unsigned char data[16];
  memset(data, 0, sizeof(data));
  memcpy(data, &bits.lo, 8);
  memcpy(data + 8, &bits.hi, 8);
  int id = CompilerAddBufferLiteral(data, sizeof(data));
  Literal* lit = CompilerFindLiteral(id);
  if (lit != NULL) {
    lit->disabled = false;
  }
  TypeRecord* ptr = NewPointerTo(kQualPlain, type);
  IRNode* ref = GeneratorEmit(
      gen, NewIR1(IR_OP(literalref),
                  GeneratorGetIntConstant(gen, type, id)));
  IRSetType(ref, ptr);
  IRNode* dest = NewLongDoubleTemp(gen, type);
  IRNode* dest_addr = LongDoubleObjectAddress(gen, dest, type);
  GeneratorEmit(
      gen, NewIR3(IR_OP(memcpy), dest_addr, ref,
                  GeneratorGetIntConstant(gen, NULL, type->size)));
  return dest;
}

static IRNode* CallVoidHelper(Generator* gen, const char* name,
                              IRNode* dest_addr, IRNode* a_addr,
                              IRNode* b_addr) {
  TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  int nptrs = 1 + (a_addr != NULL) + (b_addr != NULL);
  Symbol* fn = GetLongDoubleHelper(name, void_type, nptrs, NULL);
  TypeRecordDelete(void_type);
  IRNode* call = NewIR1(IR_OP(calla), GeneratorGetVariable(gen, fn));
  IRAddInput(call, dest_addr, false);
  if (a_addr != NULL) {
    IRAddInput(call, a_addr, false);
  }
  if (b_addr != NULL) {
    IRAddInput(call, b_addr, false);
  }
  return GeneratorEmit(gen, call);
}

static IRNode* CallCompareHelper(Generator* gen, IRNode* a_addr,
                                 IRNode* b_addr) {
  TypeRecord* int_type = NewTypeRecordWithSize(kTypeInt, kQualPlain);
  Symbol* fn = GetLongDoubleHelper("__davecc_ld_cmp", int_type, 2, NULL);
  IRNode* call = NewIR1(IR_OP(calla), GeneratorGetVariable(gen, fn));
  IRAddInput(call, a_addr, false);
  IRAddInput(call, b_addr, false);
  GeneratorEmit(gen, call);
  return IRSetType(call, int_type);
}

IRNode* LongDoubleBinaryValues(Generator* gen, const char* helper,
                               IRNode* left, IRNode* right, TypeRecord* type) {
  IRNode* dest = NewLongDoubleTemp(gen, type);
  IRNode* dest_addr = LongDoubleObjectAddress(gen, dest, type);
  CallVoidHelper(gen, helper, dest_addr,
                 LongDoubleObjectAddress(gen, left, type),
                 LongDoubleObjectAddress(gen, right, type));
  return dest;
}

IRNode* LongDoubleCompareValues(Generator* gen, IRNode* left, IRNode* right,
                                TypeRecord* type) {
  return CallCompareHelper(gen, LongDoubleObjectAddress(gen, left, type),
                           LongDoubleObjectAddress(gen, right, type));
}

IRNode* LongDoubleAbsoluteValue(Generator* gen, IRNode* value,
                                TypeRecord* type) {
  IRNode* zero = GenerateLongDoubleConstant(gen, type, 0.0);
  IRNode* cmp = LongDoubleCompareValues(gen, value, zero, type);
  IRNode* dest = NewLongDoubleTemp(gen, type);
  IRNode* dest_addr = LongDoubleObjectAddress(gen, dest, type);
  IRNode* negative = GeneratorEmit(
      gen, NewIR2(IR_OP(cmplti), cmp, GeneratorGetIntConstant(gen, cmp->type, 0)));
  IRNode* end = NewIR(IR_OP(label));
  IRNode* else_label = NewIR(IR_OP(label));
  GeneratorEmit(gen, NewIR2(IR_OP(bfalse), negative, else_label));
  CallVoidHelper(gen, "__davecc_ld_neg", dest_addr,
                 LongDoubleObjectAddress(gen, value, type), NULL);
  GeneratorEmit(gen, NewIR1(IR_OP(bra), end));
  GeneratorEmit(gen, else_label);
  StoreLongDoubleToAddress(gen, dest_addr, value, type);
  GeneratorEmit(gen, end);
  return dest;
}

IRNode* GenerateLongDoubleBinary(Generator* gen, BinaryASTNode* node) {
  IRNode* left = GenerateExpression(gen, node->left);
  IRNode* right = GenerateExpression(gen, node->right);

  switch (node->base.op) {
    case AST_OP(equal):
    case AST_OP(noteq):
    case AST_OP(less):
    case AST_OP(lesseq):
    case AST_OP(greater):
    case AST_OP(greatereq): {
      IRNode* cmp =
          LongDoubleCompareValues(gen, left, right, node->left->type);
      TypeRecord* bool_type = NewTypeRecordWithSize(kTypeBool, kQualPlain);
      IROpcode pred = IR_OP(cmpeqi);
      if (node->base.op == AST_OP(noteq)) {
        pred = IR_OP(cmpnei);
      } else if (node->base.op == AST_OP(less)) {
        pred = IR_OP(cmplti);
      } else if (node->base.op == AST_OP(lesseq)) {
        pred = IR_OP(cmplei);
      } else if (node->base.op == AST_OP(greater)) {
        pred = IR_OP(cmpgti);
      } else if (node->base.op == AST_OP(greatereq)) {
        pred = IR_OP(cmpgei);
      }
      IRNode* zero = GeneratorGetIntConstant(gen, cmp->type, 0);
      IRNode* result = GeneratorEmit(gen, NewIR2(pred, cmp, zero));
      return IRSetType(result, bool_type);
    }
    default:
      break;
  }

  const char* helper = NULL;
  switch (node->base.op) {
    case AST_OP(plus):
      helper = "__davecc_ld_add";
      break;
    case AST_OP(minus):
      helper = "__davecc_ld_sub";
      break;
    case AST_OP(mult):
      helper = "__davecc_ld_mul";
      break;
    case AST_OP(div):
      helper = "__davecc_ld_div";
      break;
    default:
      return left;
  }
  return LongDoubleBinaryValues(gen, helper, left, right, node->base.type);
}

IRNode* GenerateLongDoubleNegate(Generator* gen, IRNode* operand,
                                 TypeRecord* type) {
  IRNode* src = LongDoubleObjectAddress(gen, operand, type);
  IRNode* dest = NewLongDoubleTemp(gen, type);
  IRNode* dest_addr = LongDoubleObjectAddress(gen, dest, type);
  CallVoidHelper(gen, "__davecc_ld_neg", dest_addr, src, NULL);
  return dest;
}

IRNode* ConvertValueToLongDouble(Generator* gen, IRNode* value,
                                 TypeRecord* from, TypeRecord* to) {
  IRNode* dest = NewLongDoubleTemp(gen, to);
  IRNode* dest_addr = LongDoubleObjectAddress(gen, dest, to);
  if (TypeUsesFloat64Representation(from) || TypeIsDouble(from)) {
    TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
    Symbol* fn = GetLongDoubleHelper("__davecc_ld_from_f64", void_type, 1,
                                     from);
    TypeRecordDelete(void_type);
    IRNode* call = NewIR1(IR_OP(calla), GeneratorGetVariable(gen, fn));
    IRAddInput(call, dest_addr, false);
    IRAddInput(call, value, false);
    GeneratorEmit(gen, call);
    return dest;
  }
  if (TypeUsesFloat32Representation(from)) {
    TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
    Symbol* fn = GetLongDoubleHelper("__davecc_ld_from_f32", void_type, 1,
                                     from);
    TypeRecordDelete(void_type);
    IRNode* call = NewIR1(IR_OP(calla), GeneratorGetVariable(gen, fn));
    IRAddInput(call, dest_addr, false);
    IRAddInput(call, value, false);
    GeneratorEmit(gen, call);
    return dest;
  }
  TypeRecord* ll = NewTypeRecordWithSize(kTypeLongLong, kQualPlain);
  TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  Symbol* fn = GetLongDoubleHelper("__davecc_ld_from_i64", void_type, 1, ll);
  TypeRecordDelete(void_type);
  if (value->type != NULL && value->type->size < 8) {
    int diff_bits = (8 - (int)value->type->size) * 8;
    IROpcode ext = TypeIsUnsigned(value->type) ? IR_OP(zeroextendi)
                                               : IR_OP(signextendi);
    value = IRSetType(
        GeneratorEmit(gen,
                      NewIR2(ext, value,
                             GeneratorGetIntConstant(gen, ll, diff_bits))),
        ll);
  } else {
    TypeRecordDelete(ll);
  }
  IRNode* call = NewIR1(IR_OP(calla), GeneratorGetVariable(gen, fn));
  IRAddInput(call, dest_addr, false);
  IRAddInput(call, value, false);
  GeneratorEmit(gen, call);
  return dest;
}

IRNode* ConvertValueFromLongDouble(Generator* gen, IRNode* value,
                                   TypeRecord* from, TypeRecord* to) {
  IRNode* src = LongDoubleObjectAddress(gen, value, from);
  if (!TypeIsPointer(src->type)) {
    src = IRSetType(src, NewPointerTo(kQualPlain, from));
  }
  if (TypeUsesFloat64Representation(to) || TypeIsDouble(to)) {
    TypeRecord* ret = TypeRecordCopy(to);
    Symbol* fn = GetLongDoubleHelper("__davecc_ld_to_f64", ret, 1, NULL);
    TypeRecordDelete(ret);
    IRNode* call = NewIR1(IR_OP(calla), GeneratorGetVariable(gen, fn));
    IRAddInput(call, src, false);
    GeneratorEmit(gen, call);
    return IRSetType(call, to);
  }
  if (TypeUsesFloat32Representation(to)) {
    TypeRecord* ret = TypeRecordCopy(to);
    Symbol* fn = GetLongDoubleHelper("__davecc_ld_to_f32", ret, 1, NULL);
    TypeRecordDelete(ret);
    IRNode* call = NewIR1(IR_OP(calla), GeneratorGetVariable(gen, fn));
    IRAddInput(call, src, false);
    GeneratorEmit(gen, call);
    return IRSetType(call, to);
  }
  TypeRecord* ll = NewTypeRecordWithSize(kTypeLongLong, kQualPlain);
  Symbol* fn = GetLongDoubleHelper("__davecc_ld_to_i64", ll, 1, NULL);
  IRNode* call = NewIR1(IR_OP(calla), GeneratorGetVariable(gen, fn));
  IRAddInput(call, src, false);
  GeneratorEmit(gen, call);
  return IRSetType(call, to);
}
