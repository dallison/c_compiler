//
//  expr_codegen.c
//  c_compiler
//
//  Created by David Allison on 11/21/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "expr_codegen.h"
#include <assert.h>
#include "compiler.h"
#include "symbol_table.h"
#include "rtti.h"

// Table to translate an AST node and type into an IR operation.
static struct {
  ASTOpcode node_op;
  bool (*type_func)(TypeRecord*);
  IROpcode ir_op;
  bool commutative;
} expr_operators[] = {
    {AST_OP(plus), TypeIsIntegral, IR_OP(addi), true},
    {AST_OP(plus), TypeIsFloat, IR_OP(addf), true},
    {AST_OP(plus), TypeIsDouble, IR_OP(addd), true},
    {AST_OP(plus), TypeIsPointerOrArray, IR_OP(adda), true},

    {AST_OP(minus), TypeIsIntegral, IR_OP(subi)},
    {AST_OP(minus), TypeIsFloat, IR_OP(subf)},
    {AST_OP(minus), TypeIsDouble, IR_OP(subd)},
    {AST_OP(minus), TypeIsPointerOrArray, IR_OP(suba)},

    {AST_OP(mult), TypeIsIntegral, IR_OP(muli), true},
    {AST_OP(mult), TypeIsFloat, IR_OP(mulf), true},
    {AST_OP(mult), TypeIsDouble, IR_OP(muld), true},

    {AST_OP(div), TypeIsIntegral, IR_OP(divi)},
    {AST_OP(div), TypeIsFloat, IR_OP(divf)},
    {AST_OP(div), TypeIsDouble, IR_OP(divd)},

    {AST_OP(mod), TypeIsIntegral, IR_OP(modi)},

    {AST_OP(lshift), TypeIsIntegral, IR_OP(lsli)},
    {AST_OP(rshiftl), TypeIsIntegral, IR_OP(lsri)},
    {AST_OP(rshifta), TypeIsIntegral, IR_OP(asri)},

    {AST_OP(and), TypeIsIntegral, IR_OP(andi), true},
    {AST_OP(bitor), TypeIsIntegral, IR_OP(ori), true},
    {AST_OP(exor), TypeIsIntegral, IR_OP(xori), true},

    {AST_OP(equal), TypeIsIntegral, IR_OP(cmpeqi), true},
    {AST_OP(noteq), TypeIsIntegral, IR_OP(cmpnei), true},
    {AST_OP(less), TypeIsIntegral, IR_OP(cmplti)},
    {AST_OP(lesseq), TypeIsIntegral, IR_OP(cmplei)},
    {AST_OP(greater), TypeIsIntegral, IR_OP(cmpgti)},
    {AST_OP(greatereq), TypeIsIntegral, IR_OP(cmpgei)},

    {AST_OP(equal), TypeIsFloat, IR_OP(cmpeqf), true},
    {AST_OP(noteq), TypeIsFloat, IR_OP(cmpnef), true},
    {AST_OP(less), TypeIsFloat, IR_OP(cmpltf)},
    {AST_OP(lesseq), TypeIsFloat, IR_OP(cmplef)},
    {AST_OP(greater), TypeIsFloat, IR_OP(cmpgtf)},
    {AST_OP(greatereq), TypeIsFloat, IR_OP(cmpgef)},

    {AST_OP(equal), TypeIsDouble, IR_OP(cmpeqd), true},
    {AST_OP(noteq), TypeIsDouble, IR_OP(cmpned), true},
    {AST_OP(less), TypeIsDouble, IR_OP(cmpltd)},
    {AST_OP(lesseq), TypeIsDouble, IR_OP(cmpled)},
    {AST_OP(greater), TypeIsDouble, IR_OP(cmpgtd)},
    {AST_OP(greatereq), TypeIsDouble, IR_OP(cmpged)},

    {AST_OP(equal), TypeIsPointerOrArray, IR_OP(cmpeqa), true},
    {AST_OP(noteq), TypeIsPointerOrArray, IR_OP(cmpnea), true},
    {AST_OP(less), TypeIsPointerOrArray, IR_OP(cmplta)},
    {AST_OP(lesseq), TypeIsPointerOrArray, IR_OP(cmplea)},
    {AST_OP(greater), TypeIsPointerOrArray, IR_OP(cmpgta)},
    {AST_OP(greatereq), TypeIsPointerOrArray, IR_OP(cmpgea)},

    // A function designator used as an operand decays to its address, so a
    // comparison against it (e.g. `fp == some_function`) compares pointers.
    {AST_OP(equal), TypeIsFunction, IR_OP(cmpeqa), true},
    {AST_OP(noteq), TypeIsFunction, IR_OP(cmpnea), true},

    {AST_OP(not), TypeIsIntegral, IR_OP(noti)},
    {AST_OP(not), TypeIsPointer, IR_OP(nota)},
    {AST_OP(onescomp), TypeIsIntegral, IR_OP(onescomp)},
    {AST_OP(uminus), TypeIsIntegral, IR_OP(negi)},
    {AST_OP(uminus), TypeIsFloat, IR_OP(negf)},
    {AST_OP(uminus), TypeIsDouble, IR_OP(negd)},
 

    {AST_OP(bad), NULL, IR_OP(nop)},
};

// Given an AST node and an operation (might not be the same as node->op),
// return an IR opcode based on the AST operation and node type.
static IROpcode FindIROpcodeForType(TypeRecord* type, ASTOpcode op) {
  for (size_t i = 0; expr_operators[i].node_op != AST_OP(bad); i++) {
    if (expr_operators[i].node_op == op) {
      if (expr_operators[i].type_func(type)) {
        return expr_operators[i].ir_op;
      }
    }
  }
  // During speculative constant evaluation `type` may be NULL (or an otherwise
  // unhandled type) because the callee's body has not been semantically
  // analyzed yet.  Bail out of the fold gracefully rather than aborting; see
  // Compiler::constexpr_codegen_recover.
  if (compiler->constexpr_codegen_recover) {
    longjmp(compiler->constexpr_codegen_abort, 1);
  }
  assert(false);
  return IR_OP(nop);
}

static IROpcode FindIROpcode(ASTNode* node, ASTOpcode op) {
  return FindIROpcodeForType(node->type, op);
}

static bool IsCommutative(ASTOpcode op) {
  for (size_t i = 0; expr_operators[i].node_op != AST_OP(bad); i++) {
    if (expr_operators[i].node_op == op) {
      return expr_operators[i].commutative;
    }
  }
  assert(false);
  return false;
}

// Table to translate a type into a size.
static struct {
  bool (*type_func)(TypeRecord*);
  int (*size_func)(void);
} int_type_sizes[] = {
  {TypeIsShort, ShortSize},
  {TypeIsLong, LongSize},
  {TypeIsLongLong, LongLongSize},
  {TypeIsInt, IntSize},
  {TypeIsChar, CharSize},
  {TypeIsBool, BoolSize},
  {NULL, NULL},
};

static struct {
  int size;
  IROpcode signed_load;
  IROpcode unsigned_load;
  IROpcode store;
} int_opcodes[] = {
  {1, IR_OP(load8), IR_OP(loadu8), IR_OP(store8)},
  {2, IR_OP(load16), IR_OP(loadu16), IR_OP(store16)},
  {4, IR_OP(load32), IR_OP(loadu32), IR_OP(store32)},
  {8, IR_OP(load64), IR_OP(load64), IR_OP(store64)},
  {0, IR_OP(nop), IR_OP(nop), IR_OP(nop)},
};

// Table to translate a type into load and store operations.
static struct {
  bool (*type_func)(TypeRecord*);
  IROpcode signed_load;
  IROpcode unsigned_load;
  IROpcode store;
} load_store_ops[] = {
    {TypeIsFloat, IR_OP(loadf), IR_OP(loadf), IR_OP(storef)},
  {TypeIsDouble, IR_OP(loadd), IR_OP(loadd), IR_OP(stored)},
  {TypeIsLongDouble, IR_OP(loadd), IR_OP(loadd), IR_OP(stored)},
    {TypeIsPointerOrArray, IR_OP(loada), IR_OP(loada), IR_OP(storea)},
    {TypeIsFunction, IR_OP(loada), IR_OP(loada), IR_OP(storea)},
    {TypeIsStructOrUnion, IR_OP(loada), IR_OP(loada), IR_OP(storea)},
    {NULL, IR_OP(nop), IR_OP(nop), IR_OP(nop)},
};

static IROpcode GetLoadOpcodeForType(TypeRecord* type) {
  // Get size of integral type from compiler object.
  int size = 0;
  for (size_t i = 0; int_type_sizes[i].type_func != NULL; i++) {
    if (int_type_sizes[i].type_func(type)) {
      size = int_type_sizes[i].size_func();
      break;
    }
  }
  
  // If integral size, translate to opcode.
  if (size > 0) {
    for (size_t i = 0; int_opcodes[i].size != 0; i++) {
      if (int_opcodes[i].size == size) {
        if (TypeIsUnsigned(type)) {
          return int_opcodes[i].unsigned_load;
        } else {
          return int_opcodes[i].signed_load;
        }
      }
    }
  }
  
  // Not integral, call type inference funcs.
  for (size_t i = 0; load_store_ops[i].type_func != NULL; i++) {
    if (load_store_ops[i].type_func(type)) {
      if (TypeIsUnsigned(type)) {
        return load_store_ops[i].unsigned_load;
      } else {
        return load_store_ops[i].signed_load;
      }
    }
  }
  // During speculative constant evaluation `type` may be NULL because the
  // callee's inline body has not been analyzed yet.  Bail out of the fold
  // instead of aborting; see Compiler::constexpr_codegen_recover.
  if (compiler->constexpr_codegen_recover) {
    longjmp(compiler->constexpr_codegen_abort, 1);
  }
  assert(false);
  return IR_OP(nop);
}

static IROpcode GetLoadOpcode(ASTNode* node) {
  return GetLoadOpcodeForType(node->type);
}

static int BitSizeToByteSize(int bit_size) {
  if (bit_size > 32) {
    return 8;
  } else if (bit_size > 16) {
    return 4;
  } else if (bit_size > 8) {
    return 2;
  }
  return 1;
}

static COMPILER_UNUSED IROpcode GetLoadOpcodeFromSize(ASTNode* node, int bit_size) {
  // Round bit size to byte size.
  int size = BitSizeToByteSize(bit_size);

  for (size_t i = 0; int_opcodes[i].size != 0; i++) {
    if (int_opcodes[i].size == size) {
      if (TypeIsUnsigned(node->type)) {
        return int_opcodes[i].unsigned_load;
      } else {
        return int_opcodes[i].signed_load;
      }
    }
  }
  assert(false);
  return IR_OP(nop);
}

static IROpcode GetStoreOpcodeForType(TypeRecord* type) {
  // Get size of integral type from compiler object.
  int size = 0;
  for (size_t i = 0; int_type_sizes[i].type_func != NULL; i++) {
    if (int_type_sizes[i].type_func(type)) {
      size = int_type_sizes[i].size_func();
      break;
    }
  }
  
  // If integral size, translate to opcode.
  if (size > 0) {
    for (size_t i = 0; int_opcodes[i].size != 0; i++) {
      if (int_opcodes[i].size == size) {
        return int_opcodes[i].store;
      }
    }
  }

  for (size_t i = 0; load_store_ops[i].type_func != NULL; i++) {
    if (load_store_ops[i].type_func(type)) {
      return load_store_ops[i].store;
    }
  }
  assert(false);
  return IR_OP(nop);
}

static IROpcode GetStoreOpcode(ASTNode* node) {
  return GetStoreOpcodeForType(node->type);
}

// String literals are global to the compiler.  Each one has a
// unique id allocated by the compiler.  The IR instruction
// contains this ID.
static IRNode* GenerateLiteral(Generator* gen, ConstantASTNode* node) {
  int literal_id = CompilerAddStringLiteral(node->value.string, node->base.op == AST_OP(string_wide));

  return IRSetType(GeneratorEmit(
      gen, NewIR1(IR_OP(literalref),
                  GeneratorGetIntConstant(gen, node->base.type, literal_id))), node->base.type);
}

static SourceLocation BuiltinSourceLocation(VectorASTNode* node) {
  ASTNode* current = &node->base;
  while ((current->flags & kASTDefaultArgument) != 0 &&
         current->parent != NULL) {
    current = current->parent;
  }
  return current->location;
}

static IRNode* GenerateBuiltinSourceString(Generator* gen, VectorASTNode* node,
                                           const char* value) {
  String string;
  StringInit(&string, value);
  int literal_id = CompilerAddStringLiteral(&string, false);
  StringDestruct(&string);
  return IRSetType(GeneratorEmit(
      gen, NewIR1(IR_OP(literalref),
                  GeneratorGetIntConstant(gen, node->base.type, literal_id))),
                   node->base.type);
}

static TypeRecord* BuiltinCurrentFunction(Generator* gen) {
  if (gen != NULL && gen->func != NULL) {
    return gen->func;
  }
  return compiler->current_function;
}

static IRNode* GenerateBuiltinSourceFile(Generator* gen, VectorASTNode* node) {
  const char* filename = NULL;
  int lineno = 0;
  int start = 0;
  int end = 0;
  DecodeSourceLocation(BuiltinSourceLocation(node), &filename, &lineno, &start,
                       &end);
  (void)lineno;
  (void)start;
  (void)end;
  return GenerateBuiltinSourceString(gen, node,
                                     filename != NULL ? filename : "<unknown>");
}

static IRNode* GenerateBuiltinSourceFunction(Generator* gen,
                                             VectorASTNode* node) {
  const char* function_name = "";
  TypeRecord* func = BuiltinCurrentFunction(gen);
  if (func != NULL && func->info.function.symbol != NULL) {
    function_name = func->info.function.symbol->name.value;
  }
  return GenerateBuiltinSourceString(gen, node, function_name);
}

static IRNode* GenerateBuiltinSourcePrettyFunction(Generator* gen,
                                                   VectorASTNode* node) {
  String name;
  StringInit(&name, NULL);
  TypeRecordFunctionPrettyName(BuiltinCurrentFunction(gen), &name);
  IRNode* result =
      GenerateBuiltinSourceString(gen, node, name.value != NULL ? name.value : "");
  StringDestruct(&name);
  return result;
}

static IRNode* GenerateBuiltinSourceLine(Generator* gen, VectorASTNode* node) {
  int fileno = 0;
  int lineno = 0;
  int colno = 0;
  SourceLocationNumbers(BuiltinSourceLocation(node), &fileno, &lineno, &colno);
  (void)fileno;
  return GeneratorGetIntConstant(gen, node->base.type, lineno);
}

static IRNode* GenerateBuiltinSourceColumn(Generator* gen,
                                           VectorASTNode* node) {
  int fileno = 0;
  int lineno = 0;
  int colno = 0;
  SourceLocationNumbers(BuiltinSourceLocation(node), &fileno, &lineno, &colno);
  (void)fileno;
  (void)lineno;
  return GeneratorGetIntConstant(gen, node->base.type, colno + 1);
}

static struct {
  bool (*type_func)(TypeRecord*);
  IROpcode opcode;
} mov_opcodes[] = {
    {TypeIsIntegral, IR_OP(movi)},       {TypeIsFloat, IR_OP(movf)},
    {TypeIsDouble, IR_OP(movd)},         {TypeIsStructOrUnion, IR_OP(mova)},
    {TypeIsPointerOrArray, IR_OP(mova)}, {TypeIsFunction, IR_OP(mova)},
    {TypeIsVoid, IR_OP(mova)},           {NULL, 0},
};

static IROpcode MoveToTmpOpcode(TypeRecord* type) {
  for (size_t i = 0; type != NULL && mov_opcodes[i].type_func != NULL; i++) {
    if (mov_opcodes[i].type_func(type)) {
      return mov_opcodes[i].opcode;
    }
  }
  // A NULL/unclassifiable type only reaches here during speculative constant
  // evaluation of a not-yet-fully-analyzed body (e.g. a forward-declared
  // mutually-recursive constexpr callee); recover instead of aborting.  See
  // Compiler::constexpr_codegen_recover.
  if (compiler->constexpr_codegen_recover) {
    longjmp(compiler->constexpr_codegen_abort, 1);
  }
  assert(false);
  return IR_OP(nop);
}

static IRNode* RemoveUnnecesaryShortening(Generator* gen, IRNode* value,
                                          IROpcode opcode) {
  if (value->opcode != IR_OP(signextendi) && value->opcode != IR_OP(zeroextendi)) {
    return value;
  }
  IRNode* input = value->inputs.value.p[0];
  int width = input->type->size;
  bool remove_op = false;
  switch (opcode) {
    case IR_OP(store8):
      remove_op = width > 1;
      break;
    case IR_OP(store16):
      remove_op = width > 2;
      break;
    case IR_OP(store32):
      remove_op = width > 4;
      break;
    default:
      return value;
  }
  return remove_op ? input : value;
}

// Stash the result of a calla node into a temporary.
static IRNode* StashCallResult(Generator* gen, IRNode* node,
                               bool route_conversion_to_dest) {
  IRNode* tmp = GeneratorEmit(gen, NewIR(IR_OP(tmp)));
  node->dest = tmp;
  if (route_conversion_to_dest) {
    node->flags |= kIRStashedCallResult;
  }
  // IROpcode op = MoveToTmpOpcode(node->type);
  // IRNode* rmov = GeneratorEmit(gen, NewIR2(op, tmp, node));
  //IRSetType(rmov, node->type);
  IRSetType(tmp, node->type);
  return tmp;
}

typedef struct {
  bool found_call;
} CallFinder;

static void FindCall(ASTNode* node, void* data, int child_id, VisitorMode mode) {
  CallFinder* finder = data;
  if (node->op == AST_OP(call)) {
    finder->found_call = true;
  }
}

static bool ContainsCall(ASTNode* node) {
  CallFinder finder = {false};
  ASTNodeVisit(node, FindCall, 0, &finder);
  return finder.found_call;
}

static IRNode* GenerateBinaryExpression(Generator* gen, BinaryASTNode* node) {
  // If the left and right nodes contains a call then we need to move their
  // results into a temp node. Calls always return in the same register.
  bool stash_call_results = compiler->call_return_fixed_reg &&
      ContainsCall(node->left) &&
      ContainsCall(node->right);
  
  ASTNode* lhs = node->left;
  ASTNode* rhs = node->right;
  bool is_commutative = IsCommutative(node->base.op);
  // If the operation is commutative, put a constant on the right side so that
  // it's easier to fold constants.
  if (is_commutative) {
    if (ASTNodeIsIntConstant(node->left) && !ASTNodeIsIntConstant(node->right)) {
      ASTNode* tmp = lhs;
      lhs = rhs;
      rhs = tmp;
    }
  }
  IRNode* left = GenerateExpression(gen, lhs);
  if (stash_call_results) {
    left = StashCallResult(gen, left, /*route_conversion_to_dest=*/false);
  }
  IRNode* right = GenerateExpression(gen, rhs);
  if (stash_call_results) {
    right = StashCallResult(gen, right, /*route_conversion_to_dest=*/false);
  }
  ASTNode* opcode_type_node =
      (node->base.op == AST_OP(plus) || node->base.op == AST_OP(minus)) &&
              TypeIsPointerOrArray(node->base.type)
          ? &node->base
          : node->right;
  IROpcode opcode = FindIROpcode(opcode_type_node, node->base.op);
  return IRSetType(GeneratorEmit(gen, NewIR2(opcode, left, right)), node->base.type);
}

// Pick the cmp3way IR opcode for the (converted, common) operand type.
static IROpcode ThreeWayIROpcode(TypeRecord* type) {
  if (TypeIsFloat(type)) {
    return IR_OP(cmp3wayf);
  }
  if (TypeIsDouble(type) || TypeIsLongDouble(type)) {
    return IR_OP(cmp3wayd);
  }
  if (TypeIsPointerOrArray(type)) {
    return IR_OP(cmp3waya);
  }
  if (TypeIsUnsigned(type)) {
    return IR_OP(cmp3wayu);
  }
  return IR_OP(cmp3wayi);
}

// Lower a built-in scalar `a <=> b`.  The cmp3way op yields a signed integer
// -1/0/1 (and 2 = unordered for floats); that value is stored into the `int _v`
// member (offset 0) of a fresh comparison-category temporary, mirroring the way
// struct-by-value results are materialized.
static IRNode* GenerateThreeWayComparison(Generator* gen, BinaryASTNode* node) {
  IRNode* left = GenerateExpression(gen, node->left);
  IRNode* right = GenerateExpression(gen, node->right);
  TypeRecord* operand_type = node->right->type;
  IROpcode opcode = ThreeWayIROpcode(operand_type);
  IRNode* cmp = GeneratorEmit(gen, NewIR2(opcode, left, right));
  TypeRecord* int_type = NewTypeRecordWithSize(kTypeInt, kQualPlain);
  IRSetType(cmp, int_type);

  Symbol* tmp = SyntaxNewTemporary(gen->syntax, node->base.type);
  IRNode* var = GeneratorGetVariable(gen, tmp);
  IRNode* addr = GeneratorEmit(gen, NewIR1(IR_OP(addressof), var));
  IRSetType(addr, NewPointerTo(kQualPlain, node->base.type));
  // Store into the category's `int _v` member (offset 0).  The store width must
  // match the target's int size (e.g. 16-bit on the 6502).
  IROpcode store_op = int_type->size >= 4   ? IR_OP(store32)
                      : int_type->size == 2 ? IR_OP(store16)
                                            : IR_OP(store8);
  GeneratorEmit(gen, NewIR2(store_op, addr, cmp));
  return var;
}

static IRNode* GenerateUnaryExpression(Generator* gen, UnaryASTNode* node) {
  IRNode* sub = GenerateExpression(gen, node->sub);
  if (node->base.op == AST_OP(uplus)) {
    return sub;
  }
  IROpcode opcode = FindIROpcode(&node->base, node->base.op);
  return IRSetType(GeneratorEmit(gen, NewIR1(opcode, sub)), node->base.type);
}


static IROpcode IncDecArithmeticOp(ASTNode* node, bool is_inc) {
  if (TypeIsIntegral(node->type)) {
    return is_inc ? IR_OP(addi) : IR_OP(subi);
  }
  if (TypeIsFloat(node->type)) {
    return is_inc ? IR_OP(addf) : IR_OP(subf);
  }
  if (TypeIsDouble(node->type)) {
    return is_inc ? IR_OP(addd) : IR_OP(subd);
  }
  return is_inc ? IR_OP(adda) : IR_OP(suba);
}

static IROpcode IncDecOp(ASTNode* node, bool is_inc) {
  if (TypeIsIntegral(node->type)) {
    int size = node->type->size;
    if (TypeIsUnsigned(node->type)) {
      switch (size) {
        case 1:
          return is_inc ? IR_OP(uinc8) : IR_OP(udec8);
        case 2:
          return is_inc ? IR_OP(uinc16) : IR_OP(udec16);
        case 4:
          return is_inc ? IR_OP(uinc32) : IR_OP(udec32);
        case 8:
          return is_inc ? IR_OP(uinc64) : IR_OP(udec64);
        default:
          abort();
      }
    } else {
      switch (size) {
        case 1:
          return is_inc ? IR_OP(inc8) : IR_OP(dec8);
        case 2:
          return is_inc ? IR_OP(inc16) : IR_OP(dec16);
        case 4:
          return is_inc ? IR_OP(inc32) : IR_OP(dec32);
        case 8:
          return is_inc ? IR_OP(inc64) : IR_OP(dec64);
        default:
          abort();
      }
    }
  }
  if (TypeIsFloat(node->type)) {
    return is_inc ? IR_OP(incf) : IR_OP(decf);
  }
  if (TypeIsDouble(node->type)) {
    return is_inc ? IR_OP(incd) : IR_OP(decd);
  }
  return is_inc ? IR_OP(inca) : IR_OP(deca);
}

static IRNode* LoadBitfield(Generator* gen, IRNode* load, BinaryASTNode* node) {
  StructMemberASTNode* member_node = (StructMemberASTNode*)node->right;
  StructMember* bitfield = member_node->member;
  
  if (bitfield->bit_size == bitfield->symbol->type->size * 8) {
    // Bitfield that is the whole word, just use the load.
    return load;
  }

  // Check for variable use.
  CheckForVarUse(load, (ASTNode*)node);
  IRNode* bitload =  GeneratorEmit(gen, NewIR3(IR_OP(getbit),
                                               load,
                                               GeneratorGetIntConstant(
                                                   gen, bitfield->symbol->type,
                                                   bitfield->bit_offset),
                                               GeneratorGetIntConstant(
                                                   gen, bitfield->symbol->type,
                                                                       bitfield->bit_size)));
  return IRSetType(bitload, bitfield->symbol->type);
#if 0
  if (TypeIsUnsigned(bitfield->symbol->type)) {
    // Unsigned, shift it right so that the low bit of the bitfield is in bit 0
    // then mask it to the correct length.
    IRNode* rshift =
        bitfield->bit_offset == 0
            ? load
            : IRSetType(GeneratorEmit(gen, NewIR2(IR_OP(lsri), load,
                                        GeneratorGetIntConstant(
                                            gen, bitfield->symbol->type,
                                            bitfield->bit_offset))), bitfield->symbol->type);
    int64_t mask = (1 << bitfield->bit_size) - 1;
    return GeneratorEmit(gen, NewIR2(IR_OP(zeroextendi), rshift,
                                     GeneratorGetIntConstant(
                                         gen, bitfield->symbol->type, mask)));
  }

  // Signed type, sign extend it.  Given the target register width, width,
  // do this by shifting left width - (offset+size)
  // bits then shifting right by width - size bits.  The idea is to put the
  // top bit of the bitfield in the sign bit position in a register, then do an
  // arithmetic right shift to move the bottom bit of the bitfield to bit 0 in
  // the register,
  int register_width = compiler->int_size * 8;
  IRNode* lshift = IRSetType(GeneratorEmit(
      gen, NewIR2(IR_OP(lsli), load,
                  GeneratorGetIntConstant(
                      gen, bitfield->symbol->type,
                      register_width - (bitfield->bit_size + bitfield->bit_offset)))), bitfield->symbol->type);
  return IRSetType(GeneratorEmit(
      gen, NewIR2(IR_OP(asri), lshift,
                  GeneratorGetIntConstant(gen, bitfield->symbol->type,
                                          register_width - bitfield->bit_size))), bitfield->symbol->type);
#endif
}

// Given a value loaded from a struct word containing a bitfield and new value
// to store in the bitfield, return the new word value to be stored back into
// the struct.
static IRNode* CalculateNewBitfieldValue(Generator* gen, IRNode* load,
                                         IRNode* value,
                                         BinaryASTNode* member_ref_node) {
  StructMemberASTNode* member_node =
      (StructMemberASTNode*)member_ref_node->right;
  StructMember* member = member_node->member;

  if (member->bit_size == member->symbol->type->size * 8) {
    // Bitfield that is the whole word, just use the new value.
    return value;
  }
  
  IRNode* bitstore =  GeneratorEmit(gen, NewIR4(IR_OP(setbit),
                                               load,
                                               value,
                                               GeneratorGetIntConstant(
                                                   gen, member->symbol->type,
                                                                       member->bit_offset),
                                               GeneratorGetIntConstant(
                                                   gen, member->symbol->type,
                                                                       member->bit_size)));
  CheckForVarDef(bitstore, &member_ref_node->base);
  return IRSetType(bitstore, member->symbol->type);

#if 0
  
  // Clear the field by ANDing with the clearing_mask.
  int64_t clearing_mask =
      ~(((1 << member->bit_size) - 1) << member->bit_offset);
  IRNode* cleared_field = GeneratorEmit(
      gen, NewIR2(IR_OP(andi), load,
                  GeneratorGetIntConstant(gen, member_ref_node->base.type,
                                          clearing_mask)));

  // Mask the value to set to the correct width.
  int64_t setting_mask = ((1 << member->bit_size) - 1);
  IRNode* shifted_value;
  if (IRIsConst(value)) {
    // Assigning a constant, mask it at compile time.
    int64_t ivalue = ((IRConstant*)value)->value.ivalue;
    shifted_value =
        GeneratorGetIntConstant(gen, member_ref_node->base.type,
                                (ivalue & setting_mask) << member->bit_offset);
  } else {
    IRNode* masked_value = GeneratorEmit(
        gen, NewIR2(IR_OP(andi), value,
                    GeneratorGetIntConstant(gen, member_ref_node->base.type,
                                            setting_mask)));
    // Shift to the correct bit position.
    shifted_value =
        member->bit_offset == 0
            ? masked_value
            : GeneratorEmit(gen, NewIR2(IR_OP(lsli), masked_value,
                                        GeneratorGetIntConstant(
                                            gen, member_ref_node->base.type,
                                            member->bit_offset)));
  }

  // OR in the shifted value.
  return GeneratorEmit(gen, NewIR2(IR_OP(ori), cleared_field, shifted_value));
#endif
}

static IRNode* GenerateVariableReference(Generator* gen,
                                         IdentifierASTNode* node) {
  IRNode* var_ref = GeneratorGetVariable(gen, node->symbol);
  if (TypeIsReference(node->symbol->type)) {
    if ((node->base.flags & kASTIsDeclaration) != 0 &&
        (node->base.flags & kASTNeedAddress) != 0) {
      return var_ref;
    }
    IRNode* ref_addr =
        IRSetType(GeneratorEmit(gen, NewIR1(IR_OP(loada), var_ref)),
                  NewPointerTo(kQualPlain, node->base.type));
    if ((node->base.flags & kASTNeedAddress) != 0 ||
        TypeIsStructOrUnion(node->base.type)) {
      return ref_addr;
    }
    IROpcode load = GetLoadOpcode(&node->base);
    IRNode* result = IRSetType(GeneratorEmit(gen, NewIR1(load, ref_addr)),
                               node->base.type);
    IRSetVarUse(result, node->symbol);
    return result;
  }
  IRNode* result;
  if ((node->base.flags & kASTNeedAddress) != 0 ||
      TypeIsStructOrUnion(node->base.type)) {
    // Need the address of the node, not the value.  A whole struct/union is
    // likewise handled by its address: the raw variable/argument node is
    // returned directly and never loaded here.  Do NOT mark it as a var-use --
    // it is the variable node itself, not a load, and a non-load var reference
    // breaks SSA renaming.  Consumers that copy the whole aggregate (a
    // struct-by-value call argument, a struct return, etc.) attach the var-use
    // to the load-like node (`structarg`, ...) they build around it.
    result = var_ref;
  } else {
    IROpcode load = GetLoadOpcode(&node->base);
    result = GeneratorEmit(gen, NewIR1(load, var_ref));
    IRSetVarUse(result, node->symbol);
  }
  if ((node->base.flags & kASTNrvoMarker) != 0) {
    var_ref->flags |= kIRNrvoMarker;
  }
  return result;
}

// Increment or decrement a complex thing.  This will generate a load, modify,
// store sequence.
static IRNode* IncDecComplex(Generator* gen, UnaryASTNode* node, bool is_post,
                              bool is_inc) {
  IRNode* addr = GenerateExpression(gen, node->sub);
  IRNode* inc_amount;
  // IROpcode mov_op;
  TypeRecord* type = node->base.type;
  if (TypeIsPointer(node->sub->type)) {
    if (TypeIsVLA(node->sub->type->next)) {
      inc_amount = node->sub->type->next->info.array.size.vla.codegen_info;
     // mov_op = IR_OP(rmova);
    } else {
      // Increment by size of thing pointed to.
      inc_amount = GeneratorGetIntConstant(gen, type,
                                           node->sub->type->next->size);
      //mov_op = IR_OP(rmova);
      type = node->sub->type;
    }
  } else {
    // Increment or decrement by one.
   // mov_op = IR_OP(rmovi);
    if (TypeIsFloatingPoint(node->sub->type)) {
//      if (TypeIsFloat(node->sub->type)) {
//        mov_op = IR_OP(rmovf);
//      } else {
//        mov_op = IR_OP(rmovd);
//      }
      inc_amount = GeneratorGetFloatingPointConstant(gen, type, 1);
    } else {
      inc_amount = GeneratorGetIntConstant(gen, type, 1);
    }
  }

   
  IROpcode load_op = GetLoadOpcode((ASTNode*)node);
  IRNode* load = GeneratorEmit(gen, NewIR1(load_op, addr));

  // If his is a bitfield, the 'load' contains the value of the whole
  // word containing the bitfield.  We extract the bits of the field into
  // 'value'.
  IRNode* value = load;
  if (IsBitfieldReference(node->sub)) {
    value = LoadBitfield(gen, load, (BinaryASTNode*)node->sub);
  }

  CheckForVarUse(load, node->sub);

  bool value_is_used = ASTNodeUsesValue(node->base.parent, &node->base);
  
  IRNode* tmp = NULL;
  if (is_post && value_is_used) {
    // If this is a post-operation (x++ or x--) then we need to record
    // the pre-incremented value in a temporary.
    tmp = GeneratorEmit(gen, NewIR(IR_OP(tmp)));
    IRSetType(tmp, load->type);
    load->dest = tmp;
    // IRNode* move = GeneratorEmit(gen, NewIR2(mov_op, tmp, load));
    // IRSetType(move, load->type);
  }
  IROpcode op = IncDecArithmeticOp(node->sub, is_inc);

  // Increment or decrement the value.
  IRNode* new_value = GeneratorEmit(gen, NewIR2(op, value, inc_amount));
  IRSetType(new_value, node->base.type);

  // If we are operating on a bitfield, we need to set the value back into the
   // word we are storing.
   if (IsBitfieldReference(node->sub)) {
     new_value = CalculateNewBitfieldValue(gen, load, new_value,
                                           (BinaryASTNode*)node->sub);
   }

  IROpcode store = GetStoreOpcode((ASTNode*)node);
  IRNode* write = GeneratorEmit(gen, NewIR2(store, addr,
                                            RemoveUnnecesaryShortening(gen, new_value, store)));

  IRSetType(write, new_value->type);
  CheckForVarDef(write, node->sub);

  if (tmp == NULL) {
    // This is a pre-increment operation, store the value and return the
    // post-incremented value.
    // Also does this if the value of the expression is not used.
    return write;
  }

  // Post increment operation, store value and return pre-incremented value.
  return tmp;
}

// Increment and decrement, both pre and post.
// This is a little complex because of the variations in the
// types.
static IRNode* GenerateIncDec(Generator* gen, UnaryASTNode* node, bool is_post,
                              bool is_inc) {
  // For a bitfield or VLA, we use a load/add/store operation sequence.
  if (IsBitfieldReference(node->sub) ||
      (TypeIsPointer(node->sub->type) && TypeIsVLA(node->sub->type->next))) {
    return IncDecComplex(gen, node, is_post, is_inc);
   }
  
  // Not a bitfield.  We an use the inc/dec IR operations to allow
  // for optimizations at lowering time.
  IRNode* addr = GenerateExpression(gen, node->sub);
  IRNode* inc_amount;
  // IROpcode mov_op;
  TypeRecord* type = node->base.type;
  if (TypeIsPointer(node->sub->type)) {
    // Increment by size of thing pointed to.
    inc_amount = GeneratorGetIntConstant(gen, type,
                                         node->sub->type->next->size);
    // mov_op = IR_OP(rmova);
    type = node->sub->type;
  } else {
    // Increment or decrement by one.
   //  mov_op = IR_OP(rmovi);
    if (TypeIsFloatingPoint(node->sub->type)) {
//      if (TypeIsFloat(node->sub->type)) {
//        mov_op = IR_OP(rmovf);
//      } else {
//        mov_op = IR_OP(rmovd);
//      }
      inc_amount = GeneratorGetFloatingPointConstant(gen, type, 1);
    } else {
      inc_amount = GeneratorGetIntConstant(gen, type, 1);
    }
  }
  
  IRNode* load = NULL;
  bool value_is_used = ASTNodeUsesValue(node->base.parent, &node->base);
  if (is_post && value_is_used) {
    // If this is a post-operation (x++ or x--) then we need to record
    // the pre-incremented value in a temporary.
    IROpcode load_op = GetLoadOpcode((ASTNode*)node);
    load = GeneratorEmit(gen, NewIR1(load_op, addr));
    CheckForVarUse(load, node->sub);
  }
  
  IROpcode op = IncDecOp(node->sub, is_inc);

  // Increment or decrement the value.
  IRNode* modified_value = GeneratorEmit(gen, NewIR2(op, addr, inc_amount));
  if (load != NULL) {
    // Add a fake input to the load.  This allows an optimizer to know that
    // it can't be removed even though it would otherwise have only one
    // output.
    IRAddInput(modified_value, load, true);
  }
  IRSetType(modified_value, node->base.type);

  CheckForVarDef(modified_value, node->sub);

  if (load == NULL) {
    // This is a pre-increment operation, store the value and return the
    // post-incremented value.
    // Also does this if the value of the expression is not used.
    return modified_value;
  }

  // Post increment operation, store value and return pre-incremented value.
  return load;
}

// This is the scaling of an expression by the size of the type a pointer is
// referring to.  The type is in ref_type and the expression is in expr.  The
// scale can be multiply or divide.  The type's size can be a constant or can
// be derived from the declaration of a Varaible Length Array (VLA).
static IRNode* GeneratePointerScale(Generator* gen, PtrScaleASTNode* node) {
  IRNode* expr = GenerateExpression(gen, node->expr);
  IRNode* size_expr;
  if (TypeIsVLA(node->ref_type)) {
    size_expr = node->ref_type->info.array.size.vla.codegen_info;
  } else {
    int64_t size = node->ref_type->size;
    if (size == 1) {
      return expr;
    }
    size_expr = GeneratorGetIntConstant(gen, NULL, size);
  }
  
  // Scaled by multiplying or dividing.
  IROpcode scale_opcode = IR_OP(muli);
  if (node->scale_op == AST_OP(div)) {
    scale_opcode = IR_OP(divi);
  }
  return GeneratorEmit(gen, NewIR2(scale_opcode, expr, size_expr));
}

static IRNode* InitArrayWithString(Generator* gen, ASTNode* node, IRNode* destaddr,
                      IRNode* value, ASTNode* init) {
  size_t memory_size = init->type->size;
  size_t length;
  ConstantASTNode* string_node = (ConstantASTNode*)init;
  if (TypeIsInt(init->type)) {
    // Wide string literal, string length includes 0 at end.
    length = string_node->value.string->length;
  } else {
    // String literal, string length doesn't include \0.
    length = string_node->value.string->length + 1;
  }
  if (length > memory_size) {
    length = memory_size;
  }
  IRNode* result = IRSetType(GeneratorEmit(gen, NewIR3(IR_OP(memcpy), destaddr, value,
                            GeneratorGetIntConstant(
                                gen, NULL, length))), node->type);
  // CheckForVarDef(result, node);
  return result;
}


// Generate code for a braced initializer.  This contains a vector of designated
// initializers generated by the semantic analyzer.
static void GenerateBracedInitializer(Generator* gen, ASTNode* node,
                                      BracedInitializerASTNode* init,
                                      IRNode* dest) {
  for (size_t i = 0; i < init->initializers->length; i++) {
    IRNode* destaddr = dest;
    ASTNode* subinit = (ASTNode*)init->initializers->value.p[i];
    assert(subinit->op == AST_OP(designated_init));

    DesignatedInitializerASTNode* designated_init =
        (DesignatedInitializerASTNode*)subinit;
    if (designated_init->designators != NULL) {
      if (designated_init->designators->length > 0) {
        // We have designators.  Need to build up an address from the
        // designators.
        int offset = 0;
        for (size_t i = 0; i < designated_init->designators->length; i++) {
          Designator* d = (Designator*)designated_init->designators->value.p[i];
          switch (d->designator_type) {
            case kDesignatorArray:
              // Array index.  Add index * size of lower dimensions to offset.
              offset += d->value.array_index * d->type->size;
              break;
            case kDesignatorStruct:
              offset += d->value.struct_member->byte_offset;
              break;
            case kDesignatorBase:
              offset += d->value.base->byte_offset;
              break;
          }
        }

        // Add the offset to the destination to get the address.
        if (offset == 0) {
          destaddr = dest;
        } else {
          destaddr = GeneratorEmit(
            gen, NewIR2(IR_OP(adda), dest,
                        GeneratorGetIntConstant(gen, NULL, offset)));
        }
      }
    }
    IRNode* old_struct_address = gen->current_struct_address;
    // For a struct/union we set the gen->current_struct_address to the
    // address we want to store it in.
    if (TypeIsStructOrUnion(designated_init->base.type)) {
      IRNode* ref = IRSetType(GeneratorEmit(gen, NewIR1(IR_OP(addressof), destaddr)), NewPointerTo(kQualPlain, designated_init->base.type));
      IRSetType(ref, NewPointerTo(kQualPlain, designated_init->base.type));
      gen->current_struct_address = ref;
    }
    
    ASTNode* designated_expr = designated_init->init;
    if (designated_expr->op == AST_OP(expr_init)) {
      designated_expr =
          ((ExpressionInitializerASTNode*)designated_expr)->expr;
    }
    IRNode* value = GenerateExpression(gen, designated_init->init);
    IRNode* write = NULL;
    if (TypeIsStructOrUnion(designated_init->base.type)) {
      if (designated_expr->op != AST_OP(call) &&
          designated_expr->op != AST_OP(comma)) {
        // A call will place its result in the address given.  If the
        // value is a struct we need to be copied in.
        
        // Initialization of a struct/union.
        value = IRSetType(GeneratorEmit(gen, NewIR1(IR_OP(addressof), value)), NewPointerTo(kQualPlain, designated_init->base.type));
        CheckForVarUse(value, designated_init->init);
        write = GeneratorEmit(gen, NewIR3(IR_OP(memcpy), destaddr, value,
                                  GeneratorGetIntConstant(
                                      gen, NULL, subinit->type->size)));
        }
    } else if (TypeIsArray(designated_init->base.type)) {
      // Init of an array with a string literal.
      write = InitArrayWithString(gen, node, destaddr, value, designated_init->init);
    } else {
      if (IsBitfieldReference(subinit)) {
        // Initialization of a bitfield.
        IROpcode load_op = GetLoadOpcode((ASTNode*)node);
        IRNode* load = GeneratorEmit(gen, NewIR1(load_op, dest));
        load = LoadBitfield(gen, load, (BinaryASTNode*)subinit);
        value = CalculateNewBitfieldValue(gen, load, value, 
                                          (BinaryASTNode*)subinit);
      }
      // If this is zero inside a real braced initializer we
      // can omit the store because the memory will already be zero:
      //    struct T s = {0};
      // but if it was from a scalar initailizer;
      //    int i = 0;
      // we have to store it.
      if (init->base.type != NULL &&
          (TypeIsArray(init->base.type) || TypeIsStructOrUnion(init->base.type)) &&
          IRIsZero(value)) {
        // If we have eliminated a memzero for a union we have to store the zero
        // and can't eliminate it.
        if ((init->base.type->type & kTypeUnion) == 0) {
          // No need to store zero.
          continue;
        }
       }
      IROpcode store = GetStoreOpcode(subinit);
      write = IRSetType(GeneratorEmit(gen, NewIR2(store, destaddr,
                                RemoveUnnecesaryShortening(gen, value, store))), node->type);
    }
    if (write != NULL) {
      CheckForVarDef(write, node);
    }
    gen->current_struct_address = old_struct_address;
  }
}

// We can omit the memzero if:
// There is one initializer and the type of the only initializer
// is array or struct.
static bool CanElideMemzero(BracedInitializerASTNode* node) {
  if (node->initializers->length != 1) {
    return false;
  }
  DesignatedInitializerASTNode* init = node->initializers->value.p[0];
  TypeRecord* type = node->base.type;
  if (TypeIsStructOrUnion(type)) {
    // Initializing a struct or union.  We can eliminate the memzero if
    // it is a union and the size of the member we are initializing is
    // the same as the initializer size.
    if ((type->type & kTypeUnion) != 0) {
      // Type is a union.  Can eliminate if the size of the type we are
      // using to initialize is the same as the size of tne union.
      if (type->size == init->init->type->size) {
        return true;
      }
    }
  }
  return TypeIsStructOrUnion(init->init->type) || TypeIsArray(init->init->type);
}

// Initialization.  Semantic analysis converts the initializer to a
// braced initializer containing only designated initalizers.
static IRNode* GenerateInitialization(Generator* gen, BinaryASTNode* node) {
  if (node->right->op != AST_OP(braced_init)) {
    // During speculative constant evaluation the callee's (e.g. freshly
    // instantiated constexpr template) body may not have been semantically
    // analyzed yet, so its initializers were never lowered to braced-init
    // form.  Bail out of the fold gracefully rather than aborting; the
    // function is later analyzed and code-generated through the normal path.
    // See Compiler::constexpr_codegen_recover.
    if (compiler->constexpr_codegen_recover) {
      longjmp(compiler->constexpr_codegen_abort, 1);
    }
  }
  assert(node->right->op == AST_OP(braced_init));

  // Get destination address.
  IRNode* dest = GenerateExpression(gen, node->left);

  BracedInitializerASTNode* init = (BracedInitializerASTNode*)node->right;

  // Zero the memory if we are initializing a struct or an array.  The
  // standard says that all non-initialized members should be initialized
  // as if they are static.  This means that we zero out the memory.
  if (TypeIsStructOrUnion(node->base.type) || TypeIsArray(node->base.type)) {
    if (!CanElideMemzero(init)) {
      IRNode* memzero = GeneratorEmit(gen, NewIR1(IR_OP(memzero), dest));
      // The memzero spans the whole object being initialized.  Record its type
      // explicitly: the destination may be a pointer-typed slot (e.g. an sret
      // return location) with no backing symbol, so the size cannot always be
      // recovered from the destination operand during lowering.
      IRSetType(memzero, node->base.type);
      CheckForVarDef(memzero, &node->base);
    }
  }
  GenerateBracedInitializer(gen, (ASTNode*)node, init, dest);
  return dest;
}

static IRNode* GenerateCompoundLiteral(Generator* gen, CompoundLiteralASTNode* node) {
  assert(node->initializer->op == AST_OP(braced_init));

  // Get destination address.
  IRNode* dest = gen->current_struct_address != NULL &&
                         TypeIsStructOrUnion(node->base.type)
                     ? gen->current_struct_address
                     : GenerateExpression(gen, node->sym);

  BracedInitializerASTNode* init = (BracedInitializerASTNode*)node->initializer;

  // Zero the memory if we are initializing a struct or an array.  The
  // standard says that all non-initialized members should be initialized
  // as if they are static.  This means that we zero out the memory.
  if (TypeIsStructOrUnion(node->base.type) || TypeIsArray(node->base.type)) {
    if (!CanElideMemzero(init)) {
      IRNode* memzero = GeneratorEmit(gen, NewIR1(IR_OP(memzero), dest));
      // See GenerateInitialization: record the zeroed object's type so the size
      // is available even when the destination is a symbol-less pointer slot.
      IRSetType(memzero, node->base.type);
      CheckForVarDef(memzero, &node->base);
    }
  }
  GenerateBracedInitializer(gen, (ASTNode*)node, init, dest);
  return dest;
}

// Simple assignment.
static IRNode* GenerateAssignment(Generator* gen, BinaryASTNode* node) {
  IRNode* dest = GenerateExpression(gen, node->left);

  IRNode* value;
  IRNode* assignment;
  if (TypeIsStructOrUnion(node->left->type)) {
    if (node->right->op == AST_OP(call)) {
      // Assignment from a function call.
      IRNode* old_struct_address = gen->current_struct_address;
      IRNode* ref = GeneratorEmit(gen, NewIR1(IR_OP(addressof), dest));
      IRSetType(ref, NewPointerTo(kQualPlain, node->left->type));
      gen->current_struct_address = ref;
      value = GenerateExpression(gen, node->right);
      gen->current_struct_address = old_struct_address;
      return value;
    } else {
      // Struct or union assignment, use memcpy.
      value = GenerateExpression(gen, node->right);
      value = GeneratorEmit(gen, NewIR1(IR_OP(addressof), value));
      CheckForVarUse(value, node->right);
      assignment = GeneratorEmit(
          gen,
          NewIR3(IR_OP(memcpy), dest, value,
                 GeneratorGetIntConstant(gen, NULL, node->base.type->size)));
    }
  } else {
    Symbol* dest_tmp = NULL;
    IRNode* dest_tmp_var = NULL;
    bool dest_was_spilled = false;
    if (node->right->op == AST_OP(call) && !IRIsVariable(dest)) {
      dest_tmp =
          SyntaxNewTemporary(gen->syntax,
                             NewPointerTo(kQualPlain, node->left->type));
      dest_tmp_var = GeneratorGetVariable(gen, dest_tmp);
      IRNode* save_dest = GeneratorEmit(gen, NewIR2(IR_OP(storea),
                                                    dest_tmp_var, dest));
      IRSetVarDef(save_dest, dest_tmp);
    }
    value = GenerateExpression(gen, node->right);
    if (dest_tmp_var != NULL) {
      dest = IRSetType(GeneratorEmit(gen, NewIR1(IR_OP(loada), dest_tmp_var)),
                       dest->type);
      IRSetVarUse(dest, dest_tmp);
      dest_was_spilled = true;
    }
    if (IsBitfieldReference(node->left)) {
      // Assigning to a bitfield.  The dest will be the address of the word
      // containing the bitfield.  We need to mask out the bitfield (set the
      // bits to zero) then OR in the new value, masked appropriately.
      IROpcode load_op = GetLoadOpcode(node->left);
      IRNode* load = GeneratorEmit(gen, NewIR1(load_op, dest));

      if (!dest_was_spilled) {
        CheckForVarUse(load, node->left);
      }

      value = CalculateNewBitfieldValue(gen, load, value,
                                        (BinaryASTNode*)node->left);
    }

    // Simple scalar assignment.
    IROpcode store = GetStoreOpcode((ASTNode*)node);
    assignment = GeneratorEmit(gen, NewIR2(store, dest,
                                       RemoveUnnecesaryShortening(gen, value, store)));
    if (dest_was_spilled) {
      return value;
    }
  }

  CheckForVarDef(assignment, node->left);

  return value;
}

static struct {
  ASTOpcode assignment;
  ASTOpcode op;
} compound_assignment_ops[] = {
    {AST_OP(pluseq), AST_OP(plus)},       {AST_OP(minuseq), AST_OP(minus)},
    {AST_OP(multeq), AST_OP(mult)},       {AST_OP(diveq), AST_OP(div)},
    {AST_OP(percenteq), AST_OP(mod)}, {AST_OP(lshifteq), AST_OP(lshift)},
    {AST_OP(rshifteqa), AST_OP(rshifta)},  {AST_OP(rshifteql), AST_OP(rshiftl)},
    {AST_OP(andeq), AST_OP(and)},
    {AST_OP(oreq), AST_OP(bitor)},        {AST_OP(exoreq), AST_OP(exor)},
    {AST_OP(bad), AST_OP(bad)},
};

// Compound assignment.
static IRNode* GenerateCompoundAssignment(Generator* gen,
                                           BinaryASTNode* node) {
  // Get value of operation.
  IRNode* value = GenerateExpression(gen, node->right);

  // Get destination/source.
  IRNode* dest = GenerateExpression(gen, node->left);

  // Translate the assignment into an ALU operation.
  ASTOpcode alu_op = AST_OP(bad);
  for (int i = 0; compound_assignment_ops[i].assignment != AST_OP(bad); i++) {
    if (compound_assignment_ops[i].assignment == node->base.op) {
      alu_op = compound_assignment_ops[i].op;
      break;
    }
  }
  assert(alu_op != AST_OP(bad));

  // Special case for right shift.  If the type is unsigned we use a logical
  // shift, otherwise it's an arithmetic shift (sign extended).
  if (alu_op == AST_OP(rshifta)) {
    if (TypeIsUnsigned(node->base.type)) {
      alu_op = AST_OP(rshiftl);
    }
  }

  // The result is stored back to the left operand using its own type, but the
  // arithmetic may need to be performed in a wider type (e.g. `float += double`
  // computes in double then narrows to float - see ConvertCompoundAssignmentOperand).
  TypeRecord* store_type = node->base.type;       // == node->left->type
  TypeRecord* op_type = node->right->type;          // common arithmetic type
  bool widen = !TypeEqual(store_type, op_type) &&
               TypeIsFloatingPoint(store_type) && TypeIsFloatingPoint(op_type);

  // Load the value (always at the left operand's storage type).
  IROpcode load_op = GetLoadOpcode((ASTNode*)node);
  IRNode* load = IRSetType(GeneratorEmit(gen, NewIR1(load_op, dest)), store_type);

  // If we are operating on a variable we have a reference to it.
  CheckForVarUse(load, node->left);

  // For a bitfield we need to load the bits from the word loaded.
  if (IsBitfieldReference(node->left)) {
    load = LoadBitfield(gen, load, (BinaryASTNode*)node->left);
  }

  if (widen) {
    // Promote the loaded float to the wider operation type before operating.
    load = IRSetType(GeneratorEmit(gen, NewIR1(IR_OP(f2d), load)), op_type);
  }

  // Get the ALU operation to perform (in the operation type).
  IROpcode ir_op = FindIROpcodeForType(op_type, alu_op);

  // Operate on the value.
  value = IRSetType(GeneratorEmit(gen, NewIR2(ir_op, load, value)), op_type);

  if (widen) {
    // Narrow the result back to the stored (float) type.
    value = IRSetType(GeneratorEmit(gen, NewIR1(IR_OP(d2f), value)), store_type);
  }

  // Bitfield? Mask in the value.
  if (IsBitfieldReference(node->left)) {
    value = CalculateNewBitfieldValue(gen, load, value,
                                    (BinaryASTNode*)node->left);
  }

  // Store back to the destination.
  IROpcode store_op = GetStoreOpcode((ASTNode*)node);

  IRNode* result = GeneratorEmit(gen, NewIR2(store_op, dest,
                                             RemoveUnnecesaryShortening(gen, value, store_op)));
  CheckForVarDef(result, node->left);
    
  return IRSetType(result, node->base.type);
}

static IRNode* GenerateIndexExpression(Generator* gen, BinaryASTNode* node) {
  IRNode* addr = GenerateExpression(gen, node->left);
  IRNode* index = GenerateExpression(gen, node->right);
  IRNode* scaled_index;
  TypeRecord* element_type =
      node->left->type != NULL ? node->left->type->next : NULL;
  if (element_type == NULL) {
    element_type = node->base.type;
  }
  if (TypeIsVLA(element_type)) {
    IRNode* size = element_type->info.array.size.vla.codegen_info;
    scaled_index = GeneratorEmit(gen, NewIR2(IR_OP(muli), index, size));
  } else {
    int size = element_type != NULL ? element_type->size : 1;
    if (IRIsConst(index)) {
      // Index is constant, do scale in compiler.
      IRConstant* c = (IRConstant*)index;
      scaled_index = GeneratorGetIntConstant(gen, node->right->type,
                                             c->value.ivalue * size);
    } else {
      scaled_index = GeneratorEmit(
        gen,
        NewIR2(IR_OP(muli), index, GeneratorGetIntConstant(gen, NULL, size)));
    }
  }
  IRSetType(scaled_index, index->type);
  addr = GeneratorEmit(gen, NewIR2(IR_OP(adda), addr, scaled_index));
  if ((node->base.flags & kASTNeedAddress) != 0) {
    return addr;
  }
  // If the indexed element is itself an aggregate (e.g. the inner subscript of
  // a multi-dimensional array, where arr[i] has array type), it decays to its
  // address rather than being loaded.
  if (TypeIsArray(node->base.type) || TypeIsStructOrUnion(node->base.type) ||
      TypeIsFunction(node->base.type)) {
    return IRSetType(addr, node->base.type);
  }
  IROpcode load_op = GetLoadOpcode((ASTNode*)node);
  IRNode* result = GeneratorEmit(gen, NewIR1(load_op, addr));
  CheckForVarUse(result, node->left);
  return IRSetType(result, node->base.type);
}

static void PushArg(Generator* gen, IRNode* call,
                    IRNode* expr, size_t argnum, Vector* callargs) {
  // Generate an IR_OP(pusharg) containing the expression to push and the
  // argument number.  Some backends will use this to either push the
  // expression onto the stack or put it in a register.
  // Pass NULL so the (plain int) type is only created on a constant-pool miss;
  // creating it here would leak it whenever the constant is already pooled.
  IRNode* arg_num = GeneratorGetIntConstant(gen, NULL, argnum);
  IRNode* push = NewIR2(IR_OP(pusharg), expr, arg_num);
  IRSetType(push, expr->type);
  VectorAppend(callargs, GeneratorEmit(gen, push));
}

static IRNode* FreshCallAddress(Generator* gen, IRNode* address) {
  if (address == NULL) {
    return address;
  }
  if (address->opcode != IR_OP(addressof) || address->inputs.length == 0) {
    return address;
  }
  IRNode* fresh =
      GeneratorEmit(gen, NewIR1(IR_OP(addressof), address->inputs.value.p[0]));
  return IRSetType(fresh, address->type);
}

static IRNode* GenerateFunctionCall(Generator* gen, VectorASTNode* node) {
  // Address to call.
  IRNode* func = GenerateExpression(gen, node->left);

  // Call instruction (not yet emitted).
  IRNode* call = NewIR1(IR_OP(calla), func);
  
  // Arguments are evaluated right-to-left.  When more than one argument
  // contains a call, an earlier-evaluated call's result (held in the fixed
  // return register) would be clobbered by a later argument's call before it
  // reaches its own argument register.  Stash each call-bearing scalar
  // argument's result into a temporary so it survives, mirroring
  // GenerateBinaryExpression.
  int call_arg_count = 0;
  if (compiler->call_return_fixed_reg) {
    for (size_t i = 0; i < node->children->length; i++) {
      if (ContainsCall((ASTNode*)node->children->value.p[i])) {
        call_arg_count++;
      }
    }
  }
  bool stash_call_results = call_arg_count >= 2;

  Vector args_right_to_left = {0};
  TypeRecord* callee_type = node->left->type;
  if (node->left->op == AST_OP(identifier)) {
    callee_type = ((IdentifierASTNode*)node->left)->symbol->type;
  }
  if (TypeIsPointer(callee_type)) {
    callee_type = callee_type->next;
  }
  bool returns_reference = TypeIsFunction(callee_type) &&
                           TypeIsReference(callee_type->next);
  bool returns_struct = TypeIsStructOrUnion(node->base.type) &&
                        !returns_reference;
  bool cxx_constructor_call = TypeIsFunction(callee_type) &&
                              callee_type->info.function.is_constructor;
  
  // All arguments, in reverse order.
  for (ssize_t i = node->children->length-1; i >= 0; i--) {
    size_t argnum = returns_struct ? i + 1 : i;
    ASTNode* arg = (ASTNode*)node->children->value.p[i];
    IRNode* arg_value = NULL;
    if (cxx_constructor_call && i == 0 &&
        gen->current_struct_address != NULL) {
      arg_value = FreshCallAddress(gen, gen->current_struct_address);
    } else {
      IRNode* old_struct_address = gen->current_struct_address;
      gen->current_struct_address = NULL;
      bool reference_formal = false;
      if (callee_type != NULL && TypeIsFunction(callee_type) &&
          (size_t)i < callee_type->info.function.prototype.length) {
        Symbol* formal = callee_type->info.function.prototype.value.p[i];
        reference_formal = TypeIsReference(formal->type);
      }
      int old_arg_flags = arg->flags;
      if (reference_formal && !TypeIsStructOrUnion(arg->type) &&
          !TypeIsArray(arg->type)) {
        arg->flags |= kASTNeedAddress;
      }
      arg_value = GenerateExpression(gen, arg);
      arg->flags = old_arg_flags;
      gen->current_struct_address = old_struct_address;
    }
    if (cxx_constructor_call && i == 0 &&
        arg_value == gen->current_struct_address &&
        TypeIsStructOrUnion(arg_value->type)) {
      IRSetType(arg_value, NewPointerTo(kQualPlain, arg_value->type));
    }
    bool reference_formal = false;
    if (callee_type != NULL && TypeIsFunction(callee_type) &&
        (size_t)i < callee_type->info.function.prototype.length) {
      Symbol* formal = callee_type->info.function.prototype.value.p[i];
      reference_formal = TypeIsReference(formal->type);
    }

    // Scalar arguments (integer, pointer and floating-point) need stashing
    // here: a call returns its result in a fixed return register, so an
    // earlier-evaluated call's result would be clobbered by a later argument's
    // call before it reaches its own argument register.  Struct/array
    // arguments are passed by address (and struct returns land in memory), so
    // leave those untouched.
    bool aggregate_actual =
        TypeIsStructOrUnion(arg->type) || TypeIsArray(arg->type);
    bool stashable_reference_actual =
        reference_formal && !TypeIsStructOrUnion(arg_value->type) &&
        !TypeIsArray(arg_value->type);
    if (stash_call_results && ContainsCall(arg) &&
        (!aggregate_actual || stashable_reference_actual)) {
      arg_value = StashCallResult(gen, arg_value,
                                  /*route_conversion_to_dest=*/true);
    }

    if (reference_formal && !TypeIsPointerOrArray(arg_value->type) &&
        !TypeIsFunction(arg_value->type) &&
        !TypeIsStructOrUnion(arg_value->type) &&
        !TypeIsArray(arg_value->type)) {
      IRNode* ref_source = arg_value;
      if (!IRIsVariable(ref_source)) {
        Symbol* tmp = SyntaxNewTemporary(gen->syntax, arg->type);
        IRNode* var = GeneratorGetVariable(gen, tmp);
        IROpcode store = GetStoreOpcodeForType(arg->type);
        IRNode* write = GeneratorEmit(
            gen, NewIR2(store, var,
                        RemoveUnnecesaryShortening(gen, arg_value, store)));
        IRSetVarDef(write, tmp);
        ref_source = var;
      }
      arg_value = GeneratorEmit(gen, NewIR1(IR_OP(addressof), ref_source));
      IRSetType(arg_value, NewPointerTo(kQualPlain, arg->type));
    }

    if (reference_formal && TypeIsStructOrUnion(arg_value->type)) {
      arg_value = GeneratorEmit(gen, NewIR1(IR_OP(addressof), arg_value));
      IRSetType(arg_value, NewPointerTo(kQualPlain, arg->type));
      CheckForVarDef(arg_value, arg);
    } else if (reference_formal) {
      IRSetType(arg_value, NewPointerTo(kQualPlain, arg->type));
    }

    if (TypeIsStructOrUnion(arg->type) && !reference_formal) {
      // If the argument is the result of another call it may
      // have been converted to an IR_OP(addressof) which is no longer
      // a struct type (we want its address, not its value)
      if (TypeIsStructOrUnion(arg_value->type)) {
        arg_value = GeneratorEmit(gen,
                               NewIR1(IR_OP(structarg),
                                      arg_value));
        if (arg->op == AST_OP(call)) {
          arg_value->flags |= kIRFromCall;
        }
        CheckForVarUse(arg_value, arg);
      }
    } else if (TypeIsArray(arg->type) &&
               arg->op != AST_OP(string) && arg->op != AST_OP(string_wide)) {
       arg_value = GeneratorEmit(gen,
                                 NewIR1(IR_OP(addressof), arg_value));
       CheckForVarUse(arg_value, arg);
    }
    PushArg(gen, call, arg_value, argnum, &args_right_to_left);
  }

  // If we are returning a struct or union we need to add an invisible
  // first argument holding the address of where the function is to
  // store the result.
  if (returns_struct) {
    if ((node->base.flags & kASTRvoCall) != 0) {
      // Return Value Optimization call.
      PushArg(gen, call, gen->struct_return_value, 0, &args_right_to_left);
      call->flags |= kIRRvoCall;
    } else {
      if (gen->current_struct_address == NULL) {
        // No assignment address, create a temporary.
        Symbol* tmp = SyntaxNewTemporary(gen->syntax, node->base.type);
        IRNode* var = GeneratorGetVariable(gen, tmp);
        IRNode* ref = GeneratorEmit(gen, NewIR1(IR_OP(addressof), var));
        IRSetType(ref, NewPointerTo(kQualPlain, node->base.type));
        PushArg(gen, call, ref, 0, &args_right_to_left);
      } else {
        // We have a destination address, add it to the args.
        PushArg(gen, call, FreshCallAddress(gen, gen->current_struct_address),
                0, &args_right_to_left);
      }
    }
  }
  
  // Add all the pusharg instructions, left to right.
  for (ssize_t i = args_right_to_left.length - 1; i >= 0; i--) {
    IRAddInput(call, GeneratorEmit(gen, args_right_to_left.value.p[i]), false);
  }
  VectorDestruct(&args_right_to_left);
  
  // Emit call instruction.
  TypeRecord* call_result_type =
      returns_reference ? NewPointerTo(kQualPlain, node->base.type)
                        : node->base.type;
  call = IRSetType(GeneratorEmit(gen, call), call_result_type);
  if (returns_struct) {
    // We are returning a struct.  The result in whatever was passed
    // as the first arguments to the call (the second input to the
    // calla instruction).
    // However this is going to be an IR_OP(addressof) and the function
    // is returning the struct itself.
    IRNode* ret = NULL;
    for (size_t i = 1; i < call->inputs.length; i++) {
      IRNode* input = call->inputs.value.p[i];
      if (input == NULL || input->opcode != IR_OP(pusharg) ||
          input->inputs.length < 2 || !IRIsConst(input->inputs.value.p[1])) {
        continue;
      }
      IRConstant* arg_num = (IRConstant*)input->inputs.value.p[1];
      if (arg_num->value.ivalue == 0) {
        ret = input;
        break;
      }
    }
    assert(ret != NULL && ret->opcode == IR_OP(pusharg));
    // This will be a pusharg so dereference its first operand to get
    // the value being pushed,
    ret = ret->inputs.value.p[0];
    if (ret->opcode == IR_OP(addressof)) {
      ret = ret->inputs.value.p[0];
    }
    return ret;
  }
  if (returns_reference &&
      (node->base.flags & kASTNeedAddress) == 0 &&
      !TypeIsStructOrUnion(node->base.type) &&
      !TypeIsArray(node->base.type) &&
      !TypeIsFunction(node->base.type)) {
    IROpcode load = GetLoadOpcodeForType(node->base.type);
    return IRSetType(GeneratorEmit(gen, NewIR1(load, call)), node->base.type);
  }
  return call;
}

static Symbol* GetDaveCCThrowFunction(SourceLocation location) {
  String name;
  StringInit(&name, "__davecc_throw");
  Symbol* symbol = FindGlobalSymbol(&name);
  StringDestruct(&name);
  if (symbol != NULL) {
    symbol->flags.noreturn = true;
    return symbol;
  }

  TypeRecord* func_type = NewFunctionTypeRecord();
  TypeRecordChain(func_type, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
  symbol = NewSymbol("__davecc_throw", func_type, STO(extern));
  symbol->flags.invented = true;
  symbol->flags.is_forward_declared = true;
  symbol->flags.noreturn = true;
  symbol->location = location;
  SyntaxAddSymbol(&compiler->syntax, symbol);
  return symbol;
}

static Symbol* GetDaveCCDynamicCastFunction(bool is_reference,
                                            SourceLocation location) {
  const char* func_name =
      is_reference ? "__davecc_dynamic_cast_ref" : "__davecc_dynamic_cast";
  String name;
  StringInit(&name, func_name);
  Symbol* symbol = FindGlobalSymbol(&name);
  StringDestruct(&name);
  if (symbol != NULL) {
    return symbol;
  }
  TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  TypeRecord* void_ptr = NewPointerTo(kQualPlain, void_type);
  TypeRecord* func_type = NewFunctionTypeRecord();
  TypeRecordChain(func_type, void_ptr);
  symbol = NewSymbol(func_name, func_type, STO(extern));
  symbol->flags.invented = true;
  symbol->flags.is_forward_declared = true;
  symbol->location = location;
  SyntaxAddSymbol(&compiler->syntax, symbol);
  return symbol;
}

// Lowers a polymorphic dynamic_cast (downcast/sidecast) into a call to the RTTI
// runtime.  The pointer form yields the runtime result directly (null on
// failure); the reference form calls the throwing variant.
static IRNode* GenerateDynamicCast(Generator* gen, CastASTNode* node) {
  bool is_reference = TypeIsReference(node->cast_type);
  TypeRecord* dest_class =
      is_reference ? node->cast_type->next : node->cast_type->next;
  IRNode* source = GenerateExpression(gen, node->expr);

  Symbol* type_info_symbol = RttiGetTypeInfoSymbol(dest_class);
  Symbol* func_symbol =
      GetDaveCCDynamicCastFunction(is_reference, node->base.location);
  IRNode* func = GeneratorGetVariable(gen, func_symbol);
  IRNode* type_info = GeneratorGetVariable(gen, type_info_symbol);

  IRNode* call = NewIR1(IR_OP(calla), func);
  Vector args = {0};
  PushArg(gen, call, type_info, 1, &args);
  PushArg(gen, call, source, 0, &args);
  for (ssize_t i = args.length - 1; i >= 0; i--) {
    IRAddInput(call, GeneratorEmit(gen, args.value.p[i]), false);
  }
  VectorDestruct(&args);
  return IRSetType(GeneratorEmit(gen, call), node->base.type);
}

static Symbol* NewExceptionTypeInfoSymbol(EHTypeInfo* info,
                                          SourceLocation location) {
  TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  TypeRecord* typeinfo_type = NewPointerTo(kQualPlain, void_type);
  Symbol* symbol = NewSymbol(info->symbol_name.value, typeinfo_type, STO(extern));
  symbol->flags.invented = true;
  symbol->flags.is_forward_declared = true;
  symbol->location = location;
  return symbol;
}

static IRNode* GenerateThrowExpression(Generator* gen, ThrowASTNode* node) {
  if (gen->for_constant_evaluation) {
    Symbol* throw_symbol = GetDaveCCThrowFunction(node->base.location);
    IRNode* func = GeneratorGetVariable(gen, throw_symbol);
    IRNode* call = NewIR1(IR_OP(calla), func);
    return IRSetType(GeneratorEmit(gen, call),
                     NewTypeRecordWithSize(kTypeVoid, kQualPlain));
  }

  IRNode* exception_object = NULL;
  if (node->expr != NULL) {
    exception_object = GenerateExpression(gen, node->expr);
    if (TypeIsStructOrUnion(node->expr->type) &&
        exception_object->opcode != IR_OP(addressof)) {
      IRNode* address = GeneratorEmit(gen, NewIR1(IR_OP(addressof),
                                                  exception_object));
      IRSetType(address, NewPointerTo(kQualPlain, node->expr->type));
      exception_object = address;
    }
  }

  Symbol* throw_symbol = GetDaveCCThrowFunction(node->base.location);
  IRNode* func = GeneratorGetVariable(gen, throw_symbol);
  IRNode* call = NewIR1(IR_OP(calla), func);
  Vector args = {0};
  if (exception_object == NULL || TypeIsVoid(exception_object->type)) {
    exception_object = GeneratorGetIntConstant(gen, NULL, 0);
  }
  IRNode* exception_typeinfo = GeneratorGetIntConstant(gen, NULL, 0);
  if (node->expr != NULL) {
    EHTypeInfo* info = GeneratorGetExceptionTypeInfo(gen, node->expr->type);
    Symbol* typeinfo_symbol =
        NewExceptionTypeInfoSymbol(info, node->base.location);
    exception_typeinfo = GeneratorGetVariable(gen, typeinfo_symbol);
  }
  PushArg(gen, call, exception_typeinfo, 1, &args);
  PushArg(gen, call, exception_object, 0, &args);
  for (ssize_t i = args.length - 1; i >= 0; i--) {
    IRAddInput(call, GeneratorEmit(gen, args.value.p[i]), false);
  }
  VectorDestruct(&args);
  call = IRSetType(GeneratorEmit(gen, call),
                   NewTypeRecordWithSize(kTypeVoid, kQualPlain));
  return call;
}

static IRNode* GenerateContentsOf(Generator* gen, UnaryASTNode* node) {
  IRNode* addr = GenerateExpression(gen, node->sub);
  if ((node->base.flags & kASTNeedAddress) != 0) {
    return addr;
  }

  if (TypeIsStructOrUnion(node->base.type)) {
    // We are dereferencing a struct pointer.  We can't load the value
    // because the user of this expression will be expecting the
    // address of the struct.
    return addr;
  }

  if (TypeIsArray(node->base.type)) {
    // Loading contents of a pointer to an array.  Don't dereference the
    // array.
    return addr;
  }

  if (TypeIsFunction(node->base.type)) {
    // Loading contents of a pointer to a function.  Don't dereference the
    // function.
    return addr;
  }

  // All other types: load the value.
  IROpcode load_op = GetLoadOpcode((ASTNode*)node);
  return IRSetType(GeneratorEmit(gen, NewIR1(load_op, addr)), node->base.type);
}

// An inline function call is an InlineCallASTNode containing a
// CompoundStatementASTNode as its inlined node.  If the ret_value node
// is non-NULL it refers to the variable that will contain the
// result of the call.
static IRNode* GenerateInlineCall(Generator* gen, InlineCallASTNode* node) {
  extern void GenerateStatement(Generator* gen, ASTNode* node);
  GenerateStatement(gen, node->inlined);
  if (node->ret_value != NULL) {
    return GenerateExpression(gen, node->ret_value);
  }
  // No return value, return a zero constant.
  return GeneratorGetIntConstant(gen, NULL, 0);
}

static IRNode* GenerateAddressOf(Generator* gen, UnaryASTNode* node) {
  // The sub node has the kASTNeedAddress flag set so generating code for
  // it will calculate its address.
  IRNode* expr = GenerateExpression(gen, node->sub);
  if (node->sub->op == AST_OP(compound_literal)) {
    IRSetType(expr, node->base.type);
    return expr;
  }
  if (node->sub->op == AST_OP(identifier) &&
      TypeIsReference(((IdentifierASTNode*)node->sub)->symbol->type)) {
    IRSetType(expr, node->base.type);
    return expr;
  }
  if (TypeIsFunction(node->sub->type)) {
    IRSetType(expr, node->base.type);
    return expr;
  }
  IRNode* result = GeneratorEmit(gen, NewIR1(IR_OP(addressof), expr));
  CheckForVarDef(result, &node->base);
  IRSetType(result, node->base.type);
  return result;
}

static IRNode* GenerateMemberReference(Generator* gen, BinaryASTNode* node) {
  StructMemberASTNode* member = (StructMemberASTNode*)node->right;
  if (member->member->is_static) {
    Symbol* symbol = member->member->symbol;
    IRNode* var_ref = GeneratorGetVariable(gen, symbol);
    if ((node->base.flags & kASTNeedAddress) != 0) {
      return var_ref;
    }
    if (TypeIsArray(symbol->type) || TypeIsStructOrUnion(symbol->type)) {
      IRSetType(var_ref, node->base.type);
      return var_ref;
    }
    IROpcode load_op = GetLoadOpcode((ASTNode*)node);
    IRNode* load = GeneratorEmit(gen, NewIR1(load_op, var_ref));
    IRSetType(load, node->base.type);
    IRSetVarUse(load, symbol);
    return load;
  }

  // Address of struct or pointer.
  IRNode* addr = GenerateExpression(gen, node->left);

  // Member, containing information on the location inside the struct.

  addr = GeneratorEmit(
      gen,
      NewIR2(IR_OP(adda), addr,
             GeneratorGetIntConstant(gen, NULL, member->byte_offset)));
  IRSetType(addr, NewPointerTo(kQualPlain, node->base.type));
  if ((node->base.flags & kASTNeedAddress) != 0) {
    // Only address needed.
    return addr;
  }

  // For a bitfield we need to load the word, shift it right to the LSB and then
  // either sign extend or mask it.
  if (StructMemberIsBitField(member->member)) {
    IROpcode load_op = GetLoadOpcode((ASTNode*)node);
    IRNode* load = IRSetType(GeneratorEmit(gen, NewIR1(load_op, addr)), node->base.type);
    return LoadBitfield(gen, load, node);
  } else {
    // Need value, load it.
    // Unless it's an array or struct/union.
    if (TypeIsArray(member->member->symbol->type) ||
        TypeIsStructOrUnion(member->member->symbol->type)) {
      IRSetType(addr, node->base.type);
      return addr;
    }
    IROpcode load_op = GetLoadOpcode((ASTNode*)node);
    IRNode* load = GeneratorEmit(gen, NewIR1(load_op, addr));
    IRSetType(load, node->base.type);
    CheckForVarUse(load, (ASTNode*)node);
    return load;
  }
}




static IRNode* GenerateLogicalOperation(Generator* gen, BinaryASTNode* node) {
 // If the left node is constant we can omit the comparison and branches.
 if (OptLevel1() && ASTNodeIsIntConstant(node->left)) {
   ConstantASTNode* c = (ConstantASTNode*)node->left;
   if (node->base.op == AST_OP(logand)) {
     if (c->value.ivalue == 0) {
       // Left of && is zero, no need to evaluate the right, result is
       // zero.
       return GenerateExpression(gen, node->left);
     }
     // Left of && is non-zero, result is the right.
     return GenerateExpression(gen, node->right);
   }
   
   // Logical OR
   if (c->value.ivalue != 0) {
     // Left of || is non-zero, no need to evaluate the right, result is
     // left.
     return GenerateExpression(gen, node->left);
   }
   // Left of || is zero, result is the right.
   return GenerateExpression(gen, node->right);
 }
 bool value_is_used = OptLevel0() ||
       ASTNodeUsesValue(node->base.parent, &node->base);
 // Non-constant logical operation, generate comparison and branches.
 IRNode* label = NewIR(IR_OP(label));
 IRNode* tmp = NULL;
 if (value_is_used) {
   tmp = GeneratorEmit(gen, NewIR(IR_OP(tmp)));
 }

 // Evaluate left node.
 IRNode* left = GenerateExpression(gen, node->left);
 
 if (value_is_used) {
   // If left is a tmp, use it as our temp, ignoring the one we've allocated.
   if (left->opcode == IR_OP(tmp)) {
     GeneratorRemoveInstruction(gen, tmp);
     tmp = left;
   } else {
     if (!IRIsExpression(left) || IRIsConstant(left) || IRIsVariable(left)) {
       left = IRSetType(GeneratorEmit(gen, NewIR1(MoveToTmpOpcode(node->left->type), left)),
                        node->left->type);
     }
     left->dest = tmp;
   }
 }


 // Short circuit.
 GeneratorEmit(gen, NewIR2(node->base.op == AST_OP(logand) ? IR_OP(bfalse)
                                                           : IR_OP(btrue),
                           left, label));

 // Evaluate right node and place result in tmp.
 IRNode* right = GenerateExpression(gen, node->right);
 if (value_is_used) {
   if (!IRIsExpression(right) || IRIsConstant(right) || IRIsVariable(right)) {
     right = IRSetType(GeneratorEmit(gen, NewIR1(MoveToTmpOpcode(node->right->type), right)),
                       node->right->type);
   }
   if (right->opcode != IR_OP(tmp)) {
     right->dest = tmp;
   } else {
     // Right is in a tmp, if this isn't the same temp as left, copy
     // it.
     if (right != tmp) {
       IRNode* copy = GeneratorEmit(gen, NewIR1(IR_OP(movi), right));
       copy->dest = tmp;
     }
   }
 }

 GeneratorEmit(gen, label);
 if (tmp != NULL) {
   IRSetType(tmp, NewTypeRecordWithSize(kTypeBool, kQualPlain));
 }
 return value_is_used ? tmp : right;
}

static IRNode* GenerateConditionalExpression(Generator* gen,
                                             BinaryASTNode* node) {
  BinaryASTNode* colon = (BinaryASTNode*)node->right;
  
  if (OptLevel1() && ASTNodeIsIntConstant(node->left)) {
    // Condition is constant  Just return the left or right.
    ConstantASTNode* c = (ConstantASTNode*)node->left;
    if (c->value.ivalue != 0) {
      return GenerateExpression(gen, colon->left);
    }
    return GenerateExpression(gen, colon->right);
  }
  
  bool value_is_used = OptLevel0() || ASTNodeUsesValue(node->base.parent, &node->base);
  bool need_address = (node->base.flags & kASTNeedAddress) != 0;

  // Non-constant condition, emit comparison and assignments to tmp.
  IRNode* false_label = NewIR(IR_OP(label));
  IRNode* end_label = NewIR(IR_OP(label));

  IRNode* tmp = NULL;
  if (value_is_used) {
    tmp = GeneratorEmitVariable(gen, NewIR(IR_OP(tmp)));
    IRSetType(tmp, need_address ? NewPointerTo(kQualPlain, node->base.type)
                                 : node->base.type);
  }

  // Evaluate condition.
  IRNode* cond = GenerateExpression(gen, node->left);
  
  // Branch to false label.
  GeneratorEmit(gen, NewIR2(IR_OP(bfalse), cond, false_label));

  // Generate true branch.
  if (need_address) {
    colon->left->flags |= kASTNeedAddress;
  }
  IRNode* left = GenerateExpression(gen, colon->left);
  if (value_is_used && colon->left->op != AST_OP(throw)) {
    if (!IRIsExpression(left) || IRIsConstant(left) || IRIsVariable(left)) {
      IROpcode move_opcode = need_address ? IR_OP(mova) :
          MoveToTmpOpcode(colon->left->type);
      left = IRSetType(GeneratorEmit(gen, NewIR1(move_opcode, left)),
                       need_address ? tmp->type : colon->left->type);
   }
    left->dest = tmp;
    // IRSetType(GeneratorEmit(gen, NewIR2(MoveToTmpOpcode(colon->left->type), tmp, left)), colon->left->type);
  }
  
  // Branch to end.
  if (colon->left->op != AST_OP(throw)) {
    GeneratorEmit(gen, NewIR1(IR_OP(bra), end_label));
  }

  // false_label:
  GeneratorEmit(gen, false_label);

  // Generate false branch,
  if (need_address) {
    colon->right->flags |= kASTNeedAddress;
  }
  IRNode* right = GenerateExpression(gen, colon->right);
  if (value_is_used && colon->right->op != AST_OP(throw)) {
    if (!IRIsExpression(right) || IRIsConstant(right) || IRIsVariable(right)) {
      IROpcode move_opcode = need_address ? IR_OP(mova) :
          MoveToTmpOpcode(colon->left->type);
      right = IRSetType(GeneratorEmit(gen, NewIR1(move_opcode, right)),
                        need_address ? tmp->type : colon->left->type);
   }
    right->dest = tmp;
    // IRSetType(GeneratorEmit(gen, NewIR2(MoveToTmpOpcode(colon->right->type), tmp, right)), colon->right->type);
  }
  // end_label:
  GeneratorEmit(gen, end_label);

  // If the value is used we return the temporary holding it.  Otherwise
  // the value will be ignored so we just return zero.
  return value_is_used ? tmp :
      GeneratorGetIntConstant(gen, NULL, 0);
}

static IRNode* GenerateBuiltinVaStart(Generator* gen, VectorASTNode* node) {
  IRNode* ap = GenerateExpression(gen, node->children->value.p[0]);
  IRNode* arg = GenerateExpression(gen, node->children->value.p[1]);
  IRNode* result = GeneratorEmit(gen, NewIR2(IR_OP(builtin_va_start), ap, arg));
  CheckForVarDef(result, &node->base);
  return IRSetType(result, node->base.type);
}

static IRNode* GenerateBuiltinVaArg(Generator* gen, VectorASTNode* node) {
  IRNode* ap = GenerateExpression(gen, node->children->value.p[0]);
  IRNode* size = GeneratorEmit(
      gen, NewIntIRConstant(node->base.type, node->base.type->size));
  IRNode* result = GeneratorEmit(gen, NewIR2(IR_OP(builtin_va_arg), ap, size));
  CheckForVarDef(result, &node->base);
  return IRSetType(result, node->base.type);
}

static IRNode* GenerateBuiltinVaEnd(Generator* gen, VectorASTNode* node) {
  IRNode* ap = GenerateExpression(gen, node->children->value.p[0]);
  IRNode* result = GeneratorEmit(gen, NewIR1(IR_OP(builtin_va_end), ap));
  CheckForVarDef(result, &node->base);
  return IRSetType(result, node->base.type);
}

static IRNode* GenerateBuiltinVaCopy(Generator* gen, VectorASTNode* node) {
  IRNode* d = GenerateExpression(gen, node->children->value.p[0]);
  IRNode* s = GenerateExpression(gen, node->children->value.p[1]);
  IRNode* result = GeneratorEmit(gen, NewIR2(IR_OP(builtin_va_copy), d, s));
  CheckForVarDef(result, &node->base);
  return IRSetType(result, node->base.type);
}

static IRNode* GenerateBuiltinAtomic(Generator* gen, VectorASTNode* node,
                                     IROpcode opcode) {
  IRNode* result = NewIR(opcode);
  for (size_t i = 0; i < node->children->length; i++) {
    IRNode* child = GenerateExpression(gen, node->children->value.p[i]);
    IRAddInput(result, child, i == 0);
  }
  result = GeneratorEmit(gen, result);
  CheckForVarDef(result, &node->base);
  return IRSetType(result, node->base.type);
}

static IRNode* GenerateBuiltinAtomicCompareExchange(Generator* gen,
                                                    VectorASTNode* node,
                                                    bool expected_is_pointer,
                                                    bool returns_bool) {
  ASTNode* ptr_arg = node->children->value.p[0];
  TypeRecord* value_type = ptr_arg->type->next;
  IROpcode load_op = GetLoadOpcodeForType(value_type);
  IROpcode store_op = GetStoreOpcodeForType(value_type);

  IRNode* ptr = GenerateExpression(gen, ptr_arg);
  IRNode* old_value = IRSetType(GeneratorEmit(gen, NewIR1(load_op, ptr)),
                                value_type);

  IRNode* expected_value;
  IRNode* expected_ptr = NULL;
  if (expected_is_pointer) {
    expected_ptr = GenerateExpression(gen, node->children->value.p[1]);
    expected_value = IRSetType(
        GeneratorEmit(gen, NewIR1(load_op, expected_ptr)), value_type);
  } else {
    expected_value = GenerateExpression(gen, node->children->value.p[1]);
  }
  IRNode* desired = GenerateExpression(gen, node->children->value.p[2]);

  IROpcode cmp_op = FindIROpcodeForType(value_type, AST_OP(equal));
  IRNode* matches = IRSetType(
      GeneratorEmit(gen, NewIR2(cmp_op, old_value, expected_value)),
      NewTypeRecordWithSize(kTypeBool, kQualPlain));

  IRNode* success_label = NewIR(IR_OP(label));
  IRNode* end_label = NewIR(IR_OP(label));
  GeneratorEmit(gen, NewIR2(IR_OP(btrue), matches, success_label));

  if (expected_is_pointer) {
    IRNode* update_expected =
        GeneratorEmit(gen, NewIR2(store_op, expected_ptr, old_value));
    (void)update_expected;
  }
  GeneratorEmit(gen, NewIR1(IR_OP(bra), end_label));

  GeneratorEmit(gen, success_label);
  IRNode* store_desired = GeneratorEmit(gen, NewIR2(store_op, ptr, desired));
  (void)store_desired;
  GeneratorEmit(gen, end_label);

  return IRSetType(returns_bool ? matches : old_value, node->base.type);
}

static IRNode* GenerateZeroExtend(Generator* gen, ASTNode* node, IRNode* input) {
  int diff = node->type->size - input->type->size;  // Difference in bytes.
   if (diff == 0) {
     return input;
   }
   
   if (IRIsConst(input)) {
     IRConstant* c = (IRConstant*)input;
     int64_t value = c->value.ivalue;
     if (diff > 0) {
       int64_t mask = (1LL << input->type->size * 8) - 1;
       value &= mask;
     }
     return GeneratorGetIntConstant(gen, node->type, value);
   }
   return IRSetType(GeneratorEmit(gen,
                        NewIR2(IR_OP(zeroextendi), input,
                               GeneratorGetIntConstant(gen, node->type, diff*8))), node->type);

}

static IRNode* GenerateSignExtend(Generator* gen, IRNode* from, ASTNode* to) {
  int diff = to->type->size - from->type->size;  // Difference in bytes.
  if (diff == 0) {
    return from;
  }
  
  if (IRIsConst(from)) {
    IRConstant* c = (IRConstant*)from;
    int64_t value = c->value.ivalue;
    if (diff < 0) {
      // Shorten int.  Say we are shorting a short to a char.  diff will
      // be -1.
      value <<= (64 - from->type->size * 8);
      bool negative = value < 0;
      value <<= -diff * 0;
      if (negative) {
        // Negative, upper bits of result are 1
        value |= (1LL << 63);
      }
      value >>= to->type->size * 8;
    } else {
      // Getting longer by diff bytes.
      // Say we are extending a char to a short.  Diff will be 1.
      // We shift left 8 bits and then shift right 8 bits.
      value <<= (64 - from->type->size * 8);
      value >>= (64 - from->type->size * 8);
    }
    return GeneratorGetIntConstant(gen, to->type, value);
  }
  return IRSetType(GeneratorEmit(gen,
                       NewIR2(IR_OP(signextendi), from,
                              GeneratorGetIntConstant(gen, to->type, diff*8))), to->type);
}

static IRNode* GenerateToInt(Generator* gen, ASTNode* node, IROpcode op,
                             IRNode* input, int mask) {
  IRNode* convert = GeneratorEmit(gen, NewIR1(op, input));
  IRSetType(convert, node->type);
  return IRSetType(GeneratorEmit(gen,
                       NewIR2(IR_OP(zeroextendi), convert,
                              GeneratorGetIntConstant(gen, node->type, mask))), node->type);
}

static IRNode* ShortenInt(Generator* gen, ASTNode* node, IRNode* sub) {
  if (TypeIsUnsigned(sub->type)) {
    return GenerateZeroExtend(gen, node, sub);
  } else {
    return GenerateSignExtend(gen, sub, node);
  }
}

static IRNode* LengthenInt(Generator* gen, ASTNode* node, IRNode* sub) {
  if (TypeIsBool(sub->type)) {
    // Booleans are unsigned.
    return GenerateZeroExtend(gen, node, sub);
  }
  // Widening preserves the value, so the kind of extension is determined by the
  // signedness of the *source* type, not the destination: a signed value is
  // sign-extended (e.g. (unsigned long long)(int)-1 == 0xffffffffffffffff).
  if (TypeIsUnsigned(sub->type)) {
    return GenerateZeroExtend(gen, node, sub);
  } else {
    return GenerateSignExtend(gen, sub, node);
  }
}

// Conversion.
static IRNode* GenerateConversion(Generator* gen, ASTNode* node, IRNode* sub) {
  switch (node->op) {
    case AST_OP(i2s):
    case AST_OP(l2s):
    case AST_OP(ll2s):
      return ShortenInt(gen, node, sub);
    case AST_OP(i2c):
    case AST_OP(i2b):
    case AST_OP(s2c):
    case AST_OP(s2b):
    case AST_OP(l2c):
    case AST_OP(l2b):
    case AST_OP(ll2c):
    case AST_OP(ll2b):
      return ShortenInt(gen, node, sub);
    case AST_OP(i2l):
      return ShortenInt(gen, node, sub);
    case AST_OP(i2ll):
      return LengthenInt(gen, node, sub);
    case AST_OP(i2f):
    case AST_OP(c2f):
    case AST_OP(s2f):
    case AST_OP(l2f):
    case AST_OP(ll2f):
      return IRSetType(GeneratorEmit(gen, NewIR1(IR_OP(i2f), sub)), node->type);
    case AST_OP(i2d):
    case AST_OP(i2ld):
    case AST_OP(c2d):
    case AST_OP(c2ld):
    case AST_OP(s2d):
    case AST_OP(s2ld):
    case AST_OP(l2d):
    case AST_OP(l2ld):
    case AST_OP(ll2d):
    case AST_OP(ll2ld):
      return IRSetType(GeneratorEmit(gen, NewIR1(IR_OP(i2d), sub)), node->type);
    case AST_OP(c2i):
    case AST_OP(c2s):
    case AST_OP(c2l):
    case AST_OP(c2ll):
    case AST_OP(c2b):
    case AST_OP(l2ll):
    case AST_OP(ll2l):
    case AST_OP(s2i):
    case AST_OP(s2l):
    case AST_OP(s2ll):
      return LengthenInt(gen, node, sub);
    case AST_OP(l2i):
    case AST_OP(ll2i):
      return ShortenInt(gen, node, sub);
    case AST_OP(f2i):
      return GenerateToInt(gen, node, IR_OP(f2i), sub, 0xffff);
    case AST_OP(f2s):
      return GenerateToInt(gen, node, IR_OP(f2i), sub, 0xffffffff);

    case AST_OP(f2l):
    case AST_OP(f2ll):
      return IRSetType(GeneratorEmit(gen, NewIR1(IR_OP(f2i), sub)), node->type);

    case AST_OP(f2d):
    case AST_OP(f2ld):
      return IRSetType(GeneratorEmit(gen, NewIR1(IR_OP(f2d), sub)), node->type);
    case AST_OP(f2b):
    case AST_OP(f2c):
      return GenerateToInt(gen, node, IR_OP(f2i), sub, 0xff);
    case AST_OP(d2i):
    case AST_OP(ld2i):
      return GenerateToInt(gen, node, IR_OP(d2i), sub, 0xffffffff);
    case AST_OP(d2c):
    case AST_OP(d2b):
    case AST_OP(ld2c):
    case AST_OP(ld2b):
      return GenerateToInt(gen, node, IR_OP(d2i), sub, 0xff);
    case AST_OP(d2s):
    case AST_OP(ld2s):
      return GenerateToInt(gen, node, IR_OP(d2i), sub, 0xfffff);
    case AST_OP(d2l):
    case AST_OP(d2ll):
    case AST_OP(ld2l):
    case AST_OP(ld2ll):
      return IRSetType(GeneratorEmit(gen, NewIR1(IR_OP(d2i), sub)), node->type);
    case AST_OP(d2f):
    case AST_OP(ld2f):
      return IRSetType(GeneratorEmit(gen, NewIR1(IR_OP(d2f), sub)), node->type);
    case AST_OP(d2ld):
    case AST_OP(ld2d):
      return sub;
    case AST_OP(b2c):
      return sub;
    case AST_OP(b2s):
    case AST_OP(b2l):
    case AST_OP(b2ll):
    case AST_OP(b2i):
      return LengthenInt(gen, node, sub);
    case AST_OP(b2f):
      return IRSetType(GeneratorEmit(gen, NewIR1(IR_OP(i2f), sub)), node->type);
    case AST_OP(b2d):
    case AST_OP(b2ld):
      return IRSetType(GeneratorEmit(gen, NewIR1(IR_OP(i2d), sub)), node->type);
    default:
      return sub;
  }
}

// Assembly language IR node.  This refers to a string literal.
static IRNode* GenerateAsm(Generator* gen, AsmASTNode* node) {
  int literal_id = CompilerAddStringLiteral(node->text, false);

  IRNode* asm_ir = NewIR1(IR_OP(asm), GeneratorGetIntConstant(gen, NULL, literal_id));
  asm_ir->aux = node;
  return IRSetType(GeneratorEmit(gen, asm_ir), node->base.type);
}


#if 0
static IRNode* SignExtendOrMaskExpression(Generator* gen, IRNode* inst,
                                          ASTNode* node) {
  if (!IRIsExpression(inst)) {
    return inst;
  }
  if (TypeIsChar(node->type)) {
    if (TypeIsUnsigned(node->type)) {
      // Unsigned char, mask to 8 bits.
      return GenerateMask(gen, node, inst, 0xff);
    }
    // Signed char, sign extend to 64 bits.
    return GenerateSignExtend(gen, inst, node, 8, 64);
  }
  if (TypeIsShort(node->type)) {
    if (TypeIsUnsigned(node->type)) {
      // Unsigned short, mask to 16 bits.
      return GenerateMask(gen, node, inst, 0xffff);
    }
    // Signed short, sign extend to 64 bits.
    return GenerateSignExtend(gen, inst, node, 16, 64);
  }
  if (TypeIsInt(node->type)) {
    if (TypeIsUnsigned(node->type)) {
      // Unsigned int, mask to 32 bits.
      return GenerateMask(gen, node, inst, 0xffffffff);
    }
    // Signed int, sign extend to 64 bits.
    return GenerateSignExtend(gen, inst, node, 32, 64);
  }

  return inst;
}
#endif

// Main expression code generator.  Generates code for one expression and
// returns the IR Node coding it.
IRNode* GenerateExpression(Generator* gen, ASTNode* node) {
  ConstantASTNode* const_node = (ConstantASTNode*)node;
  IdentifierASTNode* id_node = (IdentifierASTNode*)node;
  BinaryASTNode* binary_node = (BinaryASTNode*)node;
  UnaryASTNode* unary_node = (UnaryASTNode*)node;
  CastASTNode* cast_node = (CastASTNode*)node;
  VectorASTNode* vector_node = (VectorASTNode*)node;
  SizeofASTNode* sizeof_node = (SizeofASTNode*)node;

  IRSetLocation(node->location);

  IRNode* result = NULL;
  switch (node->op) {
    case AST_OP(number):
    case AST_OP(charconst):
    case AST_OP(charwide):
      result =
          GeneratorGetIntConstant(gen, node->type, const_node->value.ivalue);
      break;

    case AST_OP(fnumber):
      result = GeneratorEmitConstant(
          gen,
          NewFloatingPointIRConstant(node->type, const_node->value.fvalue));
      break;

    case AST_OP(string):
    case AST_OP(string_wide):
      result = GenerateLiteral(gen, const_node);
      break;

    case AST_OP(identifier):
      result = GenerateVariableReference(gen, id_node);
      break;

    case AST_OP(plus):
    case AST_OP(minus):
    case AST_OP(mult):
    case AST_OP(div):
    case AST_OP(mod):
    case AST_OP(lshift):
    case AST_OP(rshifta):
    case AST_OP(rshiftl):
    case AST_OP(and):
    case AST_OP(bitor):
    case AST_OP(exor):
    case AST_OP(less):
    case AST_OP(lesseq):
    case AST_OP(greater):
    case AST_OP(greatereq):
    case AST_OP(equal):
    case AST_OP(noteq):
      result = GenerateBinaryExpression(gen, binary_node);
      break;

    case AST_OP(spaceship):
      result = GenerateThreeWayComparison(gen, binary_node);
      break;

    // Conversions.
    case AST_OP(i2s):
    case AST_OP(i2c):
    case AST_OP(i2l):
    case AST_OP(i2ll):
    case AST_OP(i2f):
    case AST_OP(i2d):
    case AST_OP(i2ld):
    case AST_OP(i2b):
    case AST_OP(c2i):
    case AST_OP(c2s):
    case AST_OP(c2l):
    case AST_OP(c2ll):
    case AST_OP(c2f):
    case AST_OP(c2d):
    case AST_OP(c2ld):
    case AST_OP(c2b):
    case AST_OP(s2i):
    case AST_OP(s2c):
    case AST_OP(s2l):
    case AST_OP(s2ll):
    case AST_OP(s2f):
    case AST_OP(s2d):
    case AST_OP(s2ld):
    case AST_OP(s2b):
    case AST_OP(l2i):
    case AST_OP(l2c):
    case AST_OP(l2s):
    case AST_OP(l2ll):
    case AST_OP(l2f):
    case AST_OP(l2d):
    case AST_OP(l2ld):
    case AST_OP(l2b):
    case AST_OP(ll2i):
    case AST_OP(ll2c):
    case AST_OP(ll2s):
    case AST_OP(ll2l):
    case AST_OP(ll2f):
    case AST_OP(ll2d):
    case AST_OP(ll2ld):
    case AST_OP(ll2b):
    case AST_OP(f2i):
    case AST_OP(f2c):
    case AST_OP(f2s):
    case AST_OP(f2l):
    case AST_OP(f2ll):
    case AST_OP(f2d):
    case AST_OP(f2ld):
    case AST_OP(f2b):
    case AST_OP(d2i):
    case AST_OP(d2c):
    case AST_OP(d2s):
    case AST_OP(d2l):
    case AST_OP(d2ll):
    case AST_OP(d2f):
    case AST_OP(d2ld):
    case AST_OP(d2b):
    case AST_OP(ld2i):
    case AST_OP(ld2c):
    case AST_OP(ld2s):
    case AST_OP(ld2l):
    case AST_OP(ld2ll):
    case AST_OP(ld2f):
    case AST_OP(ld2d):
    case AST_OP(ld2b):
    case AST_OP(b2i):
    case AST_OP(b2c):
    case AST_OP(b2s):
    case AST_OP(b2l):
    case AST_OP(b2ll):
    case AST_OP(b2f):
    case AST_OP(b2d):
    case AST_OP(b2ld): {
      IRNode* sub = GenerateExpression(gen, unary_node->sub);
      result = GenerateConversion(gen, (ASTNode*)unary_node, sub);
      break;
    }

    case AST_OP(cast):
      if (cast_node->kind == kCastDynamic && cast_node->dynamic_runtime) {
        result = GenerateDynamicCast(gen, cast_node);
        break;
      }
      // Propagate flags down to child.  Use OR rather than assignment so that
      // flags the operand already carries are preserved.  In particular an
      // array/function/struct operand has kASTNeedAddress set during semantic
      // analysis (it decays to its address); clobbering the operand's flags
      // would drop that and cause a spurious load of the array contents.
      cast_node->expr->flags |= cast_node->base.flags;
      result = GenerateExpression(gen, cast_node->expr);
      if (TypeIsReference(cast_node->cast_type)) {
        IRSetType(result, node->type);
        break;
      }
      result = GeneratorEmit(gen, NewIR1(IR_OP(cast), result));
      IRSetType(result, node->type);
      break;

    case AST_OP(compound_literal):
      if ((node->flags & kASTStaticInit) == 0) {
        // Only generate code for the initialization if we are not
        // initializing a static variable.
        result = GenerateCompoundLiteral(gen, (CompoundLiteralASTNode*)node);
      } else {
        // Just emit an integer zero.
        result =
          GeneratorGetIntConstant(gen, NULL, 0);
      }
      break;
      
    case AST_OP(ptr_scale):
      result = GeneratePointerScale(gen, (PtrScaleASTNode*)node);
      break;
      
    case AST_OP(preinc):
      result = GenerateIncDec(gen, unary_node, false, true);
      break;

    case AST_OP(predec):
      result = GenerateIncDec(gen, unary_node, false, false);
      break;

    case AST_OP(postinc):
      result = GenerateIncDec(gen, unary_node, true, true);
      break;

    case AST_OP(postdec):
      result = GenerateIncDec(gen, unary_node, true, false);
      break;

    case AST_OP(init):
      if ((node->flags & kASTStaticInit) == 0) {
        // Only generate code for the initialization if we are not
        // initializing a static variable.
        result = GenerateInitialization(gen, binary_node);
      } else {
        // Just emit an integer zero.
        result =
          GeneratorGetIntConstant(gen, NULL, 0);
      }
      break;

    case AST_OP(assign):
      result = GenerateAssignment(gen, binary_node);
      break;

    case AST_OP(pluseq):
    case AST_OP(minuseq):
    case AST_OP(multeq):
    case AST_OP(diveq):
    case AST_OP(percenteq):
    case AST_OP(lshifteq):
    case AST_OP(rshifteqa):
    case AST_OP(rshifteql):
    case AST_OP(andeq):
    case AST_OP(oreq):
    case AST_OP(exoreq):
      result = GenerateCompoundAssignment(gen, binary_node);
      break;

    case AST_OP(uplus):
    case AST_OP(uminus):
    case AST_OP(not):
    case AST_OP(onescomp):
      result = GenerateUnaryExpression(gen, unary_node);
      break;

    case AST_OP(subscript):
      result = GenerateIndexExpression(gen, binary_node);
      break;

    case AST_OP(call):
      result = GenerateFunctionCall(gen, vector_node);
      break;

    case AST_OP(throw):
      result = GenerateThrowExpression(gen, (ThrowASTNode*)node);
      break;

    case AST_OP(inline_call):
      result = GenerateInlineCall(gen, (InlineCallASTNode*)node);
      break;
      
    case AST_OP(contents):
      result = GenerateContentsOf(gen, unary_node);
      break;

    case AST_OP(address):
      result = GenerateAddressOf(gen, unary_node);
      break;

    case AST_OP(dot):
    case AST_OP(arrow):
      result = GenerateMemberReference(gen, binary_node);
      break;

    case AST_OP(sizeof):
    case AST_OP(alignof):
      if (node->op == AST_OP(sizeof) && sizeof_node->expr != NULL &&
          TypeIsVLA(sizeof_node->expr->type)) {
        result = sizeof_node->expr->type->info.array.size.vla.codegen_info;
        assert(result != NULL);
      } else {
        result = GeneratorGetIntConstant(gen, node->type,
                                       sizeof_node->base.value.ivalue);
      }
      break;

    case AST_OP(logand):
    case AST_OP(logor):
      result = GenerateLogicalOperation(gen, binary_node);
      break;

    case AST_OP(question):
      result = GenerateConditionalExpression(gen, binary_node);
      break;

    case AST_OP(comma):
      GenerateExpression(gen, binary_node->left);
      result = GenerateExpression(gen, binary_node->right);
      break;

    case AST_OP(stmt_expr): {
      // GCC statement expression: emit each statement; the value is the result
      // of the final statement when it is an expression statement.
      extern void GenerateStatement(Generator * gen, ASTNode * node);
      CompoundStatementASTNode* comp =
          (CompoundStatementASTNode*)unary_node->sub;
      size_t n = comp->statements->length;
      for (size_t i = 0; i < n; i++) {
        ASTNode* stmt = comp->statements->value.p[i];
        if (i + 1 == n && stmt->op == AST_OP(expr)) {
          result =
              GenerateExpression(gen, ((ExpressionStatementASTNode*)stmt)->expr);
        } else {
          GenerateStatement(gen, stmt);
        }
      }
      if (result == NULL) {
        result =
            GeneratorGetIntConstant(gen, NULL, 0);
      }
      break;
    }

    case AST_OP(designated_init):
      result =
          GenerateExpression(gen, ((DesignatedInitializerASTNode*)node)->init);
      break;

    case AST_OP(expr_init):
      ((ExpressionInitializerASTNode*)node)->expr->flags |= node->flags;
      result =
          GenerateExpression(gen, ((ExpressionInitializerASTNode*)node)->expr);
      break;

    case AST_OP(braced_init):
      // No code to generate for this.
      break;

    case AST_OP(builtin_va_start):
      result = GenerateBuiltinVaStart(gen, vector_node);
      break;

    case AST_OP(builtin_va_arg):
      result = GenerateBuiltinVaArg(gen, vector_node);
      break;
    case AST_OP(builtin_va_end):
      result = GenerateBuiltinVaEnd(gen, vector_node);
      break;
    case AST_OP(builtin_va_copy):
      result = GenerateBuiltinVaCopy(gen, vector_node);
      break;

    case AST_OP(builtin_atomic_load):
      result = GenerateBuiltinAtomic(gen, vector_node, IR_OP(atomic_load));
      break;

    case AST_OP(builtin_atomic_store):
      result = GenerateBuiltinAtomic(gen, vector_node, IR_OP(atomic_store));
      break;

    case AST_OP(builtin_atomic_fetch_add):
      result = GenerateBuiltinAtomic(gen, vector_node, IR_OP(atomic_fetch_add));
      break;

    case AST_OP(builtin_atomic_fetch_sub):
      result = GenerateBuiltinAtomic(gen, vector_node, IR_OP(atomic_fetch_sub));
      break;

    case AST_OP(builtin_atomic_add_fetch):
      result = GenerateBuiltinAtomic(gen, vector_node, IR_OP(atomic_add_fetch));
      break;

    case AST_OP(builtin_atomic_sub_fetch):
      result = GenerateBuiltinAtomic(gen, vector_node, IR_OP(atomic_sub_fetch));
      break;

    case AST_OP(builtin_atomic_compare_exchange_bool):
      result =
          GenerateBuiltinAtomicCompareExchange(gen, vector_node, false, true);
      break;

    case AST_OP(builtin_atomic_compare_exchange_val):
      result =
          GenerateBuiltinAtomicCompareExchange(gen, vector_node, false, false);
      break;

    case AST_OP(builtin_atomic_compare_exchange_n):
      result =
          GenerateBuiltinAtomicCompareExchange(gen, vector_node, true, true);
      break;

    case AST_OP(builtin_atomic_fence):
      result = GenerateBuiltinAtomic(gen, vector_node, IR_OP(atomic_fence));
      break;

    case AST_OP(builtin_source_file):
      result = GenerateBuiltinSourceFile(gen, vector_node);
      break;

    case AST_OP(builtin_source_line):
      result = GenerateBuiltinSourceLine(gen, vector_node);
      break;

    case AST_OP(builtin_source_column):
      result = GenerateBuiltinSourceColumn(gen, vector_node);
      break;

    case AST_OP(builtin_source_function):
      result = GenerateBuiltinSourceFunction(gen, vector_node);
      break;

    case AST_OP(builtin_source_pretty_function):
      result = GenerateBuiltinSourcePrettyFunction(gen, vector_node);
      break;

    case AST_OP(asm):
      // This can happen if a function containing a return asm(...)
      // is inlinec.
      result = GenerateAsm(gen, (AsmASTNode*)node);
      break;
      
    default:
      fprintf(stderr, "Invalid expression AST op: %s (%d)\n", ASTOpcodeName(node->op), node->op);
      assert(false);
      break;
  }

  // During speculative constant evaluation an expression may lower to a
  // result with no type because the enclosing (constexpr) function body is not
  // yet fully analyzed -- e.g. a recursive call reached while that same body's
  // conditional operator is still mid-analysis, so its common type has not been
  // computed.  Recover gracefully instead of aborting; see
  // Compiler::constexpr_codegen_recover.
  if (result->type == NULL && compiler->constexpr_codegen_recover) {
    longjmp(compiler->constexpr_codegen_abort, 1);
  }
  assert(result->type != NULL);
  return result;
}

