//
//  semantics.c
//  c_compiler
//
//  Created by David Allison on 11/7/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "semantics.h"
#include <string.h>
#include "compiler.h"
#include "coro_semantics.h"
#include "errors.h"
#include "expr_evaluator.h"
#include "expr_semantics.h"
#include "lex.h"
#include "statement_semantics.h"
#include "var_analysis.h"

static int semantic_catch_depth = 0;

void SemanticEnterCatchHandler(void) {
  semantic_catch_depth++;
}

void SemanticLeaveCatchHandler(void) {
  if (semantic_catch_depth > 0) {
    semantic_catch_depth--;
  }
}

bool SemanticInCatchHandler(void) {
  return semantic_catch_depth > 0;
}

static TypeRecord* DeduceCXXInitializerListAuto(ASTNode* initializer,
                                                ASTNode* diagnostic_node) {
  if (!CompilerIsCXX() || initializer == NULL ||
      initializer->op != AST_OP(braced_init)) {
    return NULL;
  }
  BracedInitializerASTNode* braced = (BracedInitializerASTNode*)initializer;
  if (braced->initializers->length == 0) {
    SemanticError(diagnostic_node,
                  "Cannot deduce auto type from empty braced initializer");
    return NULL;
  }

  TypeRecord* element_type = NULL;
  for (size_t i = 0; i < braced->initializers->length; i++) {
    ASTNode* element = braced->initializers->value.p[i];
    if (element == NULL || element->op != AST_OP(expr_init)) {
      SemanticError(diagnostic_node,
                    "Cannot deduce auto type from braced initializer");
      return NULL;
    }
    ExpressionInitializerASTNode* expr_init =
        (ExpressionInitializerASTNode*)element;
    expr_init->expr = AnalyzeExpression(expr_init->expr);
    if (element_type == NULL) {
      element_type = expr_init->expr->type;
    } else if (!TypeEqual(element_type, expr_init->expr->type)) {
      SemanticError(diagnostic_node,
                    "Cannot deduce auto type from mixed braced initializer types");
      return NULL;
    }
  }

  TypeRecord* deduced =
      TypeInstantiateCXXInitializerList(&compiler->syntax, element_type);
  if (deduced == NULL) {
    SemanticError(diagnostic_node,
                  "std::initializer_list must be declared before auto braced deduction");
  }
  return deduced;
}

void SemanticError(ASTNode* node, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  VSemanticError(node, format, ap);
  va_end(ap);
}

void VSemanticError(ASTNode* node, const char* format, va_list ap) {
  if (abort_on_error) {
    longjmp(error_abort_state, 1);
  }
  const char* filename;
  int lineno;
  int start, end;
  DecodeSourceLocation(node->location, &filename, &lineno, &start, &end);
  VReportError(filename, lineno, format, ap);
}

void SemanticWarning(ASTNode* node, const char* warn, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  VSemanticWarning(node, warn, format, ap);
  va_end(ap);
}

static bool SymbolIsCompilerGenerated(Symbol* symbol) {
  if (symbol == NULL) {
    return false;
  }
  if (symbol->flags.is_temp || symbol->flags.invented) {
    return true;
  }
  if (strncmp(symbol->name.value, "__builtin_", 10) == 0 ||
      strncmp(symbol->name.value, "__invented__", 12) == 0) {
    return true;
  }
  const char* filename;
  int lineno;
  int start, end;
  DecodeSourceLocation(symbol->location, &filename, &lineno, &start, &end);
  return filename != NULL && strcmp(filename, "builtin") == 0;
}

void SemanticSymbolWarning(Symbol* symbol, const char* warn, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  if (SymbolIsCompilerGenerated(symbol)) {
    va_end(ap);
    return;
  }
  const char* filename;
  int lineno;
  int start, end;
  DecodeSourceLocation(symbol->location, &filename, &lineno, &start, &end);
  VReportWarning(filename, lineno, warn, format, ap);
  va_end(ap);
}

static bool NodeIsCompilerGenerated(ASTNode* node) {
  if (node == NULL) {
    return false;
  }
  switch (node->op) {
    case AST_OP(identifier): {
      Symbol* sym = ((IdentifierASTNode*)node)->symbol;
      return SymbolIsCompilerGenerated(sym);
    }
    case AST_OP(builtin_va_start):
    case AST_OP(builtin_va_arg):
    case AST_OP(builtin_va_end):
    case AST_OP(builtin_va_copy):
      return true;
    case AST_OP(label): {
      LabelASTNode* label = (LabelASTNode*)node;
      return label->named;
    }
    default:
      return false;
  }
}

static void FindCompilerGeneratedNode(ASTNode* node, void* data, int child_id,
                                      VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren) {
    return;
  }
  bool* found = data;
  if (*found) {
    return;
  }
  *found = NodeIsCompilerGenerated(node);
}

bool SemanticNodeIsCompilerGenerated(ASTNode* node) {
  bool found = false;
  ASTNodeVisit(node, FindCompilerGeneratedNode, 0, &found);
  return found;
}

void VSemanticWarning(ASTNode* node, const char* warn, const char* format,
                      va_list ap) {
  if (SemanticNodeIsCompilerGenerated(node)) {
    return;
  }
  const char* filename;
  int lineno;
  int start, end;
  DecodeSourceLocation(node->location, &filename, &lineno, &start, &end);
  VReportWarning(filename, lineno, warn, format, ap);
}

bool SemanticDeduceAutoType(Symbol* sym, ASTNode* initializer,
                            ASTNode* diagnostic_node) {
  if (sym == NULL || !TypeContainsAuto(sym->type)) {
    return true;
  }
  if (initializer == NULL) {
    SemanticError(diagnostic_node, "auto variable requires an initializer");
    return false;
  }

  TypeRecord* initializer_type = initializer->type;
  TypeRecord* deduced =
      DeduceCXXInitializerListAuto(initializer, diagnostic_node);
  if (deduced != NULL) {
    SymbolSetType(sym, deduced);
    return true;
  }
  if (initializer->op == AST_OP(expr_init)) {
    ExpressionInitializerASTNode* expr_init =
        (ExpressionInitializerASTNode*)initializer;
    initializer_type = expr_init->expr->type;
  }
  deduced = TypeDeduceAuto(sym->type, initializer_type);
  if (deduced == NULL) {
    SemanticError(diagnostic_node, "Cannot deduce auto type for %s",
                  sym->name.value);
    return false;
  }
  SymbolSetType(sym, deduced);
  return true;
}

void SemanticCheckScalarType(ASTNode* node) {
  if (node == NULL) {
    return;
  }
  if (TypeIsVoid(node->type)) {
    SemanticError(node, "Illegal use of void type");
    return;
  }
  if (TypeIsStructOrUnion(node->type)) {
    SemanticError(node, "Illegal use of composite type (struct or union)");
  }
}

// Check that we've used all local variables and emit a warning
// for each unused one.  These generally point to a programming
// error where an inner scope variable hides an outer scope of the
// same name.
// We don't do this for arguments because it's common for arguments
// to be unused deliberately.
static void CheckForUnusedLocalSymbols(Syntax* syntax, ASTNode* node) {
  for (size_t i = 0; i < syntax->all_local_symbols.length; i++) {
    Symbol* symbol = syntax->all_local_symbols.value.p[i];
    if (symbol->flags.used || symbol->flags.is_temp || symbol->flags.invented ||
        SymbolHasAttribute(symbol, "unused")) {
      continue;
    }
    if (symbol->flags.is_argument) {
      SemanticSymbolWarning(symbol, "unused-parameter",
                    "Parameter '%s' is not used in function '%s'",
                    symbol->name.value,
                      node->type->info.function.symbol->name.value);
    } else {
      SemanticSymbolWarning(symbol, "unused-variable",
                    "Local variable '%s' is not used in function '%s'",
                    symbol->name.value,
                      node->type->info.function.symbol->name.value);
    }
  }
}

static void CheckVLAArgs(Syntax* syntax, ASTNode* node) {
  Vector* prototype = &node->type->info.function.prototype;
  for (size_t i = 0; i < prototype->length; i++) {
    Symbol* arg = prototype->value.p[i];
    if (TypeIsVLA(arg->type)) {
      // Variable length array, check that all the dimensions are
      // bound to variables.
      if (arg->type->info.array.is_placeholder_vla) {
        SemanticError(node,
                      "Variable length array '%s' dimensions must be bound "
                      "to a variable in function definition", arg->name.value);
      }
    }
  }
}

static bool NodeIsZero(ASTNode* node) {
  switch (node->op) {
    case AST_OP(number): {
      ConstantASTNode* c = (ConstantASTNode*)node;
      return c->value.ivalue == 0;
      }
    case AST_OP(question): {      // Conditional expression:
      node = ((BinaryASTNode*)node)->right;     // Colon.
      ASTNode* left = ((BinaryASTNode*)node)->left;
      ASTNode* right = ((BinaryASTNode*)node)->right;
      return NodeIsZero(left) && NodeIsZero(right);
    }
    case AST_OP(cast): {      // cast
      CastASTNode* c = (CastASTNode*)node;
      return c->expr != NULL && NodeIsZero(c->expr);
    }
    default:
    return false;
  }
}

void SemanticAnalyzeFunction(Syntax* syntax, ASTNode* node) {
  // Check Variable Langth Array arguments.
  CheckVLAArgs(syntax, node);
  SemanticAnalyzeCoroutineFunction(node);
  
  // Perform semantic analysis on all the statements in the function body.
  AnalyzeStatement(node->type->info.function.body);
  if (TypeFunctionReturnContainsAuto(node->type)) {
    SemanticError(node, "Cannot deduce auto function return type");
  }
  CheckUnusedLabels(node->type->info.function.body);
  // AnalyzeVariables(node->type->info.function.body);
  CheckForUnusedLocalSymbols(syntax, node);
}

// This table contains mappings from one type to another.  The 'from'
// and 'to' are pointers to functions that return true if the passed
// type is of the type specified by the function.  The 'op' is a new
// operation to give to a new AST node that will be inserted between
// the parent and the 'from' node.
struct {
  bool (*from)(TypeRecord*);  // Function to determine type of from.
  bool (*to)(TypeRecord*);    // Function to determine type of to.
  ASTOpcode op;               // New opcode for the conversion node.
  bool warning;
} type_conversions[] = {
    // Any pointer can be converted to/from a void*.
    {TypeIsPointer, TypeIsVoidPointer, AST_OP(bad)},
    {TypeIsVoidPointer, TypeIsPointer, AST_OP(bad)},

    {TypeIsInt, TypeIsShort, AST_OP(i2s)},
    {TypeIsInt, TypeIsLong, AST_OP(i2l)},
    {TypeIsInt, TypeIsLongLong, AST_OP(i2ll)},
    {TypeIsInt, TypeIsChar, AST_OP(i2c)},
    {TypeIsInt, TypeIsFloat, AST_OP(i2f)},
    {TypeIsInt, TypeIsDouble, AST_OP(i2d)},
    {TypeIsInt, TypeIsLongDouble, AST_OP(i2ld)},
    {TypeIsInt, TypeIsBool, AST_OP(i2b)},

    {TypeIsChar, TypeIsShort, AST_OP(c2s)},
    {TypeIsChar, TypeIsLong, AST_OP(c2l)},
    {TypeIsChar, TypeIsLongLong, AST_OP(c2ll)},
    {TypeIsChar, TypeIsInt, AST_OP(c2i)},
    {TypeIsChar, TypeIsFloat, AST_OP(c2f)},
    {TypeIsChar, TypeIsDouble, AST_OP(c2d)},
    {TypeIsChar, TypeIsLongDouble, AST_OP(c2ld)},
    {TypeIsChar, TypeIsBool, AST_OP(c2b)},

    {TypeIsShort, TypeIsChar, AST_OP(s2c)},
    {TypeIsShort, TypeIsLong, AST_OP(s2l)},
    {TypeIsShort, TypeIsLongLong, AST_OP(s2ll)},
    {TypeIsShort, TypeIsInt, AST_OP(s2i)},
    {TypeIsShort, TypeIsFloat, AST_OP(s2f)},
    {TypeIsShort, TypeIsDouble, AST_OP(s2d)},
    {TypeIsShort, TypeIsLongDouble, AST_OP(s2ld)},
    {TypeIsShort, TypeIsBool, AST_OP(s2b)},

    {TypeIsLong, TypeIsChar, AST_OP(l2c)},
    {TypeIsLong, TypeIsShort, AST_OP(l2s)},
    {TypeIsLong, TypeIsLongLong, AST_OP(l2ll)},
    {TypeIsLong, TypeIsInt, AST_OP(l2i)},
    {TypeIsLong, TypeIsFloat, AST_OP(l2f)},
    {TypeIsLong, TypeIsDouble, AST_OP(l2d)},
    {TypeIsLong, TypeIsLongDouble, AST_OP(l2ld)},
    {TypeIsLong, TypeIsBool, AST_OP(l2b)},

    {TypeIsLongLong, TypeIsChar, AST_OP(ll2c)},
    {TypeIsLongLong, TypeIsShort, AST_OP(ll2s)},
    {TypeIsLongLong, TypeIsLong, AST_OP(ll2l)},
    {TypeIsLongLong, TypeIsInt, AST_OP(ll2i)},
    {TypeIsLongLong, TypeIsFloat, AST_OP(ll2f)},
    {TypeIsLongLong, TypeIsDouble, AST_OP(ll2d)},
    {TypeIsLongLong, TypeIsLongDouble, AST_OP(ll2ld)},
    {TypeIsLongLong, TypeIsBool, AST_OP(ll2b)},

    {TypeIsFloat, TypeIsChar, AST_OP(f2c)},
    {TypeIsFloat, TypeIsShort, AST_OP(f2s)},
    {TypeIsFloat, TypeIsLongLong, AST_OP(f2ll)},
    {TypeIsFloat, TypeIsInt, AST_OP(f2i)},
    {TypeIsFloat, TypeIsLong, AST_OP(f2l)},
    {TypeIsFloat, TypeIsDouble, AST_OP(f2d)},
    {TypeIsFloat, TypeIsLongDouble, AST_OP(f2ld)},
    {TypeIsFloat, TypeIsBool, AST_OP(f2b)},

    {TypeIsDouble, TypeIsChar, AST_OP(d2c)},
    {TypeIsDouble, TypeIsLong, AST_OP(d2l)},
    {TypeIsDouble, TypeIsLongLong, AST_OP(d2ll)},
    {TypeIsDouble, TypeIsInt, AST_OP(d2i)},
    {TypeIsDouble, TypeIsFloat, AST_OP(d2f)},
    {TypeIsDouble, TypeIsShort, AST_OP(d2s)},
    {TypeIsDouble, TypeIsLongDouble, AST_OP(d2ld)},
    {TypeIsDouble, TypeIsBool, AST_OP(d2b)},

    {TypeIsLongDouble, TypeIsChar, AST_OP(ld2c)},
    {TypeIsLongDouble, TypeIsLong, AST_OP(ld2l)},
    {TypeIsLongDouble, TypeIsLongLong, AST_OP(ld2ll)},
    {TypeIsLongDouble, TypeIsInt, AST_OP(ld2i)},
    {TypeIsLongDouble, TypeIsFloat, AST_OP(ld2f)},
    {TypeIsLongDouble, TypeIsShort, AST_OP(ld2s)},
    {TypeIsLongDouble, TypeIsDouble, AST_OP(ld2d)},
    {TypeIsLongDouble, TypeIsBool, AST_OP(ld2b)},

    {TypeIsBool, TypeIsChar, AST_OP(b2c)},
    {TypeIsBool, TypeIsShort, AST_OP(b2s)},
    {TypeIsBool, TypeIsLongLong, AST_OP(b2ll)},
    {TypeIsBool, TypeIsInt, AST_OP(b2i)},
    {TypeIsBool, TypeIsLong, AST_OP(b2l)},
    {TypeIsBool, TypeIsDouble, AST_OP(b2d)},
    {TypeIsBool, TypeIsLongDouble, AST_OP(b2ld)},
    {TypeIsBool, TypeIsFloat, AST_OP(b2f)},
};

#define NUM_TYPE_CONVERSIONS (sizeof(type_conversions) / sizeof(type_conversions[0]))

void SemanticTypeConversionError(ASTNode* from, TypeRecord* to,
                                 const char* format) {
  String from_string = {0};
  String to_string = {0};
  TypeRecordToString(from->type, &from_string);
  TypeRecordToString(to, &to_string);
  SemanticError(from, format, from_string.value, to_string.value);
  StringDestruct(&from_string);
  StringDestruct(&to_string);
}

void SemanticTypeConversionWarning(ASTNode* from, TypeRecord* to,
                                   const char* warn, const char* format) {
  String from_string = {0};
  String to_string = {0};
  TypeRecordToString(from->type, &from_string);
  TypeRecordToString(to, &to_string);
  SemanticWarning(from, warn, format, from_string.value, to_string.value);
  StringDestruct(&from_string);
  StringDestruct(&to_string);
}

// Fold an integer-to-integer constant conversion to type `to`.  The stored
// 64-bit value is reduced to the destination width (a no-op when widening) and
// then re-extended: sign-extended for a signed destination, zero-extended for
// an unsigned one.  Consulting the destination signedness (rather than always
// sign-extending) is required so that, e.g., converting an unsigned int such as
// 0xabcd0000 to (unsigned) long long zero-extends to 0xabcd0000 instead of
// becoming 0xffffffffabcd0000.
static ASTNode* ConvertIntConstantToType(ConstantASTNode* c, TypeRecord* to) {
  // Normalize the source value to a full 64-bit representation according to the
  // source type's width and signedness (char/short constants are stored in
  // their raw low bits, not pre-extended).
  int src_bits = (int)c->base.type->size * 8;
  uint64_t v = (uint64_t)c->value.ivalue;
  if (src_bits > 0 && src_bits < 64) {
    uint64_t smask = (1ULL << src_bits) - 1ULL;
    v &= smask;
    if (!TypeIsUnsigned(c->base.type) && (v & (1ULL << (src_bits - 1))) != 0) {
      v |= ~smask;
    }
  }
  // Reduce to the destination width and re-extend per destination signedness.
  int dst_bits = (int)to->size * 8;
  if (dst_bits > 0 && dst_bits < 64) {
    uint64_t dmask = (1ULL << dst_bits) - 1ULL;
    v &= dmask;
    if (!TypeIsUnsigned(to) && (v & (1ULL << (dst_bits - 1))) != 0) {
      v |= ~dmask;
    }
  }
  c->value.ivalue = (int64_t)v;
  return (ASTNode*)c;
}

static ASTNode* ConvertIntToDouble(ConstantASTNode* c, int bits) {
  if (TypeIsUnsigned(c->base.type)) {
    int64_t mask = (1LL << bits) - 1LL;
    c->value.ivalue &= mask;
  } else {
    c->value.ivalue <<= 64 - bits;
    c->value.ivalue >>= 64 - bits;
  }
  double v = (double)c->value.ivalue;
  c->value.fvalue = v;
  c->base.op = AST_OP(fnumber);
  return (ASTNode*)c;
}

static ASTNode* ConvertDoubleToInt(ConstantASTNode* c, int bits) {
  c->value.ivalue = (int64_t)c->value.fvalue;
  c->value.ivalue <<= 64 - bits;
  c->value.ivalue >>= 64 - bits;
  c->base.op = AST_OP(number);
  return (ASTNode*)c;
}

static ASTNode* ConvertPotentialConstant(ASTNode* from, TypeRecord* to,
                                         ASTOpcode opcode) {
  ConstantASTNode* c = (ConstantASTNode*)from;
  switch (from->op) {
    case AST_OP(number):
      switch (opcode) {
        default:
          break;
        case AST_OP(i2s):
        case AST_OP(i2c):
        case AST_OP(i2l):
        case AST_OP(i2ll):
          return ConvertIntConstantToType(c, to);
        case AST_OP(i2f):
        case AST_OP(i2d):
        case AST_OP(i2ld):
          return ConvertIntToDouble(c, 32);
        case AST_OP(i2b):
        case AST_OP(c2b):
        case AST_OP(s2b):
        case AST_OP(ll2b):
        case AST_OP(l2b):
          c->value.ivalue = c->value.ivalue != 0;
          return from;
        case AST_OP(c2i):
        case AST_OP(c2s):
        case AST_OP(c2l):
        case AST_OP(c2ll):
          return ConvertIntConstantToType(c, to);
        case AST_OP(c2f):
        case AST_OP(c2d):
        case AST_OP(c2ld):
          return ConvertIntToDouble(c, 8);
        case AST_OP(s2i):
        case AST_OP(s2c):
        case AST_OP(s2l):
        case AST_OP(s2ll):
          return ConvertIntConstantToType(c, to);
        case AST_OP(s2f):
        case AST_OP(s2d):
        case AST_OP(s2ld):
          return ConvertIntToDouble(c, 16);
        case AST_OP(l2i):
        case AST_OP(ll2i):
        case AST_OP(l2c):
        case AST_OP(ll2c):
        case AST_OP(l2s):
        case AST_OP(ll2s):
          return ConvertIntConstantToType(c, to);
        case AST_OP(ll2l):
        case AST_OP(l2ll):
          return from;
        case AST_OP(l2f):
        case AST_OP(ll2f):
        case AST_OP(ll2d):
        case AST_OP(l2d):
        case AST_OP(ll2ld):
        case AST_OP(l2ld):
          return ConvertIntToDouble(c, 64);
        case AST_OP(b2i):
        case AST_OP(b2c):
        case AST_OP(b2s):
        case AST_OP(b2l):
          return from;
        case AST_OP(b2ll):
        case AST_OP(b2f):
        case AST_OP(b2d):
        case AST_OP(b2ld):
          c->value.fvalue = (double)c->value.ivalue;
          return from;
      }
      break;
    case AST_OP(fnumber):
      switch (opcode) {
        default:
          break;
        case AST_OP(f2i):
        case AST_OP(d2i):
        case AST_OP(ld2i):
          return ConvertDoubleToInt(c, 32);
        case AST_OP(f2c):
        case AST_OP(d2c):
        case AST_OP(ld2c):
          return ConvertDoubleToInt(c, 8);
        case AST_OP(f2s):
        case AST_OP(d2s):
        case AST_OP(ld2s):
          return ConvertDoubleToInt(c, 16);
        case AST_OP(f2l):
        case AST_OP(f2ll):
        case AST_OP(d2l):
        case AST_OP(d2ll):
        case AST_OP(ld2l):
        case AST_OP(ld2ll):
          return ConvertDoubleToInt(c, 64);
        case AST_OP(f2d):
        case AST_OP(f2ld):
        case AST_OP(d2f):
        case AST_OP(d2ld):
        case AST_OP(ld2f):
        case AST_OP(ld2d):
          return from;
        case AST_OP(f2b):
        case AST_OP(d2b):
        case AST_OP(ld2b):
          c->value.ivalue = c->value.fvalue != 0;
          return from;
      }
      break;
    default:
      return NULL;
  }
  return NULL;
}

void NormalConversion(ASTNode* from, TypeRecord* to) {
  SemanticConvertType(from, to, kConvertNormal);
}

static bool TypeDiscardsQualifiers(TypeRecord* from, TypeRecord* to) {
  if (from == NULL || to == NULL) {
    return false;
  }
  return (from->qualifiers & ~to->qualifiers) != 0;
}

static bool PointerSignednessDiffers(TypeRecord* from, TypeRecord* to) {
  if (from == NULL || to == NULL || from->next == NULL || to->next == NULL) {
    return false;
  }
  TypeRecord* from_pointee = from->next;
  TypeRecord* to_pointee = to->next;
  return TypeIsIntegral(from_pointee) && TypeIsIntegral(to_pointee) &&
         TypeEqualIgnoringSign(from_pointee, to_pointee) &&
         TypeIsUnsigned(from_pointee) != TypeIsUnsigned(to_pointee);
}

static ASTNode* IdentityCloneNode(ASTNode* node, void* data) {
  (void)data;
  return node;
}

static ASTNode* NewVirtualBaseOffsetLoad(ASTNode* receiver, int vbtable_index) {
  SourceLocation location = receiver->location;
  ASTNode* receiver_clone = ASTNodeClone(receiver, IdentityCloneNode, NULL,
                                        NULL);
  ASTNode* vbptr_name =
      NewStringConstantASTNode(NewString("__vbptr"), NULL, location);
  ASTNode* vbptr =
      NewBinaryASTNode(AST_OP(arrow), NULL, location, receiver_clone,
                       vbptr_name);
  ASTNode* index =
      NewIntConstantASTNode(vbtable_index,
                            NewTypeRecordWithSize(kTypeInt, kQualPlain),
                            location);
  ASTNode* load = NewBinaryASTNode(AST_OP(subscript), NULL, location, vbptr,
                                   index);
  return AnalyzeExpression(load);
}

static ASTNode* AddStaticOffsetToRuntimeOffset(ASTNode* offset, int byte_offset,
                                               SourceLocation location) {
  if (byte_offset == 0) {
    return offset;
  }
  TypeRecord* int_type = NewTypeRecordWithSize(kTypeInt, kQualPlain);
  ASTNode* tail = NewIntConstantASTNode(byte_offset, int_type, location);
  ASTNode* combined =
      NewBinaryASTNode(AST_OP(plus), int_type, location, offset, tail);
  combined->flags |= kASTAnalyzed;
  return combined;
}

static ASTNode* NewRawPointerAdjustment(ASTNode* from, TypeRecord* to,
                                        ASTNode* offset) {
  TypeRecordIncRef(to);
  ASTNode* converted_node =
      NewBinaryASTNode(AST_OP(plus), to, from->location, from, offset);
  converted_node->flags |= kASTAnalyzed;
  return converted_node;
}

static bool TryConvertDerivedPointer(ASTNode* from, TypeRecord* to) {
  if (from == NULL || to == NULL || !TypeIsPointer(from->type) || !TypeIsPointer(to) ||
      from->type->next == NULL || to->next == NULL) {
    return false;
  }

  CXXBaseAdjustment adjustment;
  if (!TypeBaseAdjustment(from->type->next, to->next, /*public_only=*/true,
                          &adjustment)) {
    return false;
  }
  if (from->type->next->info.struct_info == to->next->info.struct_info) {
    return false;
  }
  if (adjustment.kind == kCXXBaseAdjustmentNone ||
      (adjustment.kind == kCXXBaseAdjustmentStatic &&
       adjustment.byte_offset == 0)) {
    ASTNodeSetType(from, to);
    return true;
  }

  ASTNode* parent = from->parent;
  int child_id = from->child_id;
  ASTNode* offset_node = NULL;
  if (adjustment.kind == kCXXBaseAdjustmentVirtual) {
    offset_node = NewVirtualBaseOffsetLoad(from, adjustment.vbtable_index);
    offset_node = AddStaticOffsetToRuntimeOffset(
        offset_node, adjustment.byte_offset, from->location);
  } else {
    offset_node = NewIntConstantASTNode(
        adjustment.byte_offset,
        NewTypeRecordWithSize(kTypeInt, kQualPlain), from->location);
  }
  ASTNode* converted_node = NewRawPointerAdjustment(from, to, offset_node);
  ASTNodeReplaceChild(parent, child_id, converted_node, false);
  return true;
}

static void ConversionOperatorName(TypeRecord* type, String* name) {
  String type_name;
  StringInit(&type_name, "");
  TypeRecordToString(type, &type_name);
  StringInit(name, "operator ");
  for (size_t i = 0; i < type_name.length; i++) {
    char ch = type_name.value[i];
    if (ch == '*') {
      StringAppend(name, " pointer");
    } else if (ch == '&') {
      if (i + 1 < type_name.length && type_name.value[i + 1] == '&') {
        StringAppend(name, " rvalue_reference");
        i++;
      } else {
        StringAppend(name, " reference");
      }
    } else {
      StringAppendChar(name, ch);
    }
  }
  while (name->length > 0 && name->value[name->length - 1] == ' ') {
    name->value[name->length - 1] = '\0';
    name->length--;
  }
  StringDestruct(&type_name);
}

static bool ConversionOperatorAllowedInContext(TypeRecord* func,
                                               TypeRecord* to,
                                               ConversionContext ctx) {
  if (!func->info.function.is_explicit_conversion) {
    return true;
  }
  return ctx == kConvertCast ||
         (ctx == kConvertContextualBool && TypeIsBool(to));
}

static StructMember* FindConversionOperator(Struct* str, TypeRecord* to,
                                            ConversionContext ctx) {
  if (str == NULL) {
    return NULL;
  }
  String name;
  ConversionOperatorName(to, &name);
  StructMember* member = FindStructMember(str, &name);
  StringDestruct(&name);
  while (member != NULL) {
    if (member->is_member_function && member->symbol != NULL &&
        TypeIsFunction(member->symbol->type) &&
        TypeEqual(member->symbol->type->next, to) &&
        ConversionOperatorAllowedInContext(member->symbol->type, to, ctx)) {
      return member;
    }
    member = member->overload_next;
  }
  return NULL;
}

static bool TryConvertWithConversionOperator(ASTNode* from, TypeRecord* to,
                                             ConversionContext ctx) {
  if (!CompilerIsCXX() || from == NULL || from->type == NULL ||
      !TypeIsStructOrUnion(from->type)) {
    return false;
  }
  StructMember* member =
      FindConversionOperator(from->type->info.struct_info, to, ctx);
  if (member == NULL) {
    return false;
  }
  ASTNode* parent = from->parent;
  int child_id = from->child_id;
  ASTNode* receiver = ASTNodeMove(from);
  ASTNode* member_name =
      NewStringConstantASTNode(NewString(member->symbol->name.value), NULL,
                               from->location);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, from->location, receiver,
                       member_name);
  ASTNode* call = NewVectorASTNode(AST_OP(call), NULL, from->location,
                                   member_access, NewVector());
  if (parent != NULL) {
    ASTNodeReplaceChild(parent, child_id, call, true);
  }
  call = AnalyzeExpression(call);
  if (parent != NULL) {
    ASTNodeReplaceChild(parent, child_id, call, false);
  }
  return true;
}

void SemanticConvertType(ASTNode* from, TypeRecord* to, ConversionContext ctx) {
  if (TryConvertDerivedPointer(from, to)) {
    return;
  }

  // If the types are already equal we do nothing.
  if (TypeEqual(from->type, to)) {
    return;
  }

  if (TypeEqualIgnoringSign(from->type, to)) {
    // Use the 'to' type as the node type.
    ASTNodeSetType(from, to);
    return;
  }

  if (TryConvertWithConversionOperator(from, to, ctx)) {
    return;
  }

  // Search the type conversion table for matching types.
  for (int i = 0; i < NUM_TYPE_CONVERSIONS; i++) {
    if (type_conversions[i].from(from->type) && type_conversions[i].to(to)) {
      if (type_conversions[i].op == AST_OP(bad)) {
        // Nothing to do.
        return;
      }
      if (type_conversions[i].warning) {
        SemanticTypeConversionWarning(from, to, "conversion",
                                      "Dangerous type conversion "
                                      "from '%s' to '%s'");
      }
      // Convert constants at compile time if possible.
      ASTNode* converted_node =
          ConvertPotentialConstant(from, to, type_conversions[i].op);
      if (converted_node != NULL) {
        ASTNodeSetType(converted_node, to);
        return;
      }
      ASTNode* parent = from->parent;
      int child_id = from->child_id;
      TypeRecordIncRef(to);
      converted_node =
          NewUnaryASTNode(type_conversions[i].op, to, from->location, from);
      ASTNodeReplaceChild(parent, child_id, converted_node, false);
      return;
    }
  }

  switch (ctx) {
    case kConvertCast:
      if (TypeIsVoid(to)) {
         // Casting to void is always allowed.
       } else {
         // Convert the expression to the given type.
         // This is different from a normal conversion in that there are
         // very few illegal casts.
         // Illegal casts:
         // 1. struct/union to/from anything
         // 2. void to anything but void
         bool bad_cast = false;
         if (TypeIsStructOrUnion(from->type) ||
             TypeIsStructOrUnion(to)) {
           bad_cast = true;
         } else if (TypeIsVoid(from->type)) {
           bad_cast = true;
         }
         if (bad_cast) {
           SemanticTypeConversionError(from, to, "Illegal cast");
         }
       }
      break;
      
    case kConvertContextualBool:
    case kConvertNormal:
      if (TypeIsVoidPointer(to)) {
        // Can convert any pointer, array or function to void*.
        if (TypeIsFunction(from->type) || TypeIsPointerOrArray(from->type)) {
          return;
        }
        // The only integer we can convert to void* is NULL.
        if (NodeIsZero(from)) {
          return;
        }
      }
      if (TypeIsPointerOrArray(from->type) && TypeIsPointerOrArray(to)) {
        if (TypeDiscardsQualifiers(from->type->next, to->next)) {
          SemanticTypeConversionWarning(from, to, "discarded-qualifiers",
                                        "Pointer conversion discards qualifiers; "
                                        "from '%s' to '%s'");
        }
        if (PointerSignednessDiffers(from->type, to)) {
          SemanticTypeConversionWarning(from, to, "pointer-sign",
                                        "Pointer targets differ in signedness; "
                                        "from '%s' to '%s'");
        }
        if (!TypeAssignmentCompatible(from->type, to)) {
          SemanticTypeConversionWarning(from, to, "incompatible-pointer-types",
                                        "Illegal pointer conversion; "
                                        "from '%s' to '%s'");
        }
        return;
      }
      
      // Pointers to int or bool is fine.
      if (TypeIsPointerOrArray(from->type) && (TypeIsInt(to) || TypeIsBool(to))) {
        SemanticTypeConversionWarning(from, to, "int-conversion",
                                      "Pointer to integer conversion "
                                      "from '%s' to '%s'");
        return;
      }
      
      // Unscoped enums behave like integers. C++ scoped enums require an
      // explicit cast.
      if ((TypeIsEnum(from->type) && !TypeIsScopedEnum(from->type) &&
           TypeIsInt(to)) ||
          (TypeIsInt(from->type) && TypeIsEnum(to) &&
           !TypeIsScopedEnum(to))) {
        return;
      }

      // Allow the number 0 (explicitly) to be converted to a pointer.
      if (NodeIsZero(from) && TypeIsPointer(to)) {
          return;
      }

      if (TypeIsFunction(from->type) && TypeIsFunctionPointer(to)) {
        // Functions can be converted to function pointers to the same type.
        if (TypeEqual(from->type, to->next)) {
          return;
        }
      }

      if (TypeIsUnknown(to) || TypeIsUnknown(from->type)) {
        // Unknown types don't cause errors.
        return;
      }

      if (TypeIsScopedEnum(to) || TypeIsScopedEnum(from->type)) {
        if (TypeIsScopedEnum(to) && TypeIsScopedEnum(from->type) &&
            to->info.enum_info == from->type->info.enum_info) {
          return;
        }
        SemanticTypeConversionError(
            from, to, "Illegal conversion; cannot convert from '%s' to '%s'");
        return;
      }
      
      if (TypeIsIntegral(to) && TypeIsIntegral(from->type)) {
        return;
      }
      
      SemanticTypeConversionError(
          from, to, "Illegal conversion; cannot convert from '%s' to '%s'");
      break;
  }
}

void SemanticAnalyzeVariableDefinition(Syntax* syntax,
                                       VariableDeclarationASTNode* node) {
  if (TypeIsVoid(node->symbol->type)) {
    SemanticError((ASTNode*)node, "Cannot define a variable with void type");
  }
  if (node->initializer == NULL) {
    return;
  }
  bool object_initializer =
      (TypeIsFixedArray(node->symbol->type) ||
       TypeIsStructOrUnion(node->symbol->type)) &&
      (node->initializer->op == AST_OP(braced_init) ||
       node->initializer->op == AST_OP(call));
  if (!object_initializer) {
    node->initializer = AnalyzeExpression(node->initializer);
    if (!SemanticDeduceAutoType(node->symbol, node->initializer,
                                (ASTNode*)node)) {
      return;
    }
    NormalConversion(node->initializer, node->symbol->type);
  }
  if (TypeIsConst(node->symbol->type) || node->symbol->flags.is_constexpr ||
      node->symbol->flags.is_constinit) {
    EvaluateScalarConstantForSymbol(node->symbol, node->initializer) ||
        ConstexprEvaluateObjectConstantForSymbol(node->symbol,
                                                node->initializer);
  }
  if ((node->symbol->flags.is_constexpr || node->symbol->flags.is_constinit) &&
      !node->symbol->flags.value_set) {
    SemanticError(node->initializer,
                  node->symbol->flags.is_constinit
                      ? "constinit variable initializer is not a constant expression"
                      : "constexpr variable initializer is not a constant expression");
  }
  ASTNodeSetType((ASTNode*)node, node->symbol->type);
}
