//
//  semantics.c
//  c_compiler
//
//  Created by David Allison on 11/7/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "semantics.h"
#include "errors.h"
#include "expr_semantics.h"
#include "lex.h"
#include "statement_semantics.h"

void SemanticError(ASTNode* node, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  VSemanticError(node, format, ap);
  va_end(ap);
}

void VSemanticError(ASTNode* node, const char* format, va_list ap) {
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

void VSemanticWarning(ASTNode* node, const char* warn, const char* format,
                      va_list ap) {
  const char* filename;
  int lineno;
  int start, end;
  DecodeSourceLocation(node->location, &filename, &lineno, &start, &end);
  VReportWarning(filename, lineno, warn, format, ap);
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
static void CheckForUnusedLocalSymbols(Syntax* syntax) {
  for (size_t i = 0; i < syntax->all_local_symbols.length; i++) {
    Symbol* symbol = syntax->all_local_symbols.value[i];
    if (!symbol->used && !symbol->is_argument) {
      SyntaxWarning(syntax, "unused-var",
                    "Local variable '%s' is not used in this function",
                    symbol->name.value);
    }
  }
}

void SemanticAnalyzeFunction(Syntax* syntax, ASTNode* node) {
  // Perform semantic analysis on all the statements in the function body.
  size_t num_statements = node->type->info.function.body.length;
  for (size_t i = 0; i < num_statements; i++) {
    AnalyzeStatement((ASTNode*)node->type->info.function.body.value[i]);
  }
  CheckForUnusedLocalSymbols(syntax);
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
  String from_string;
  String to_string;
  StringInit(&from_string, NULL);
  StringInit(&to_string, NULL);
  TypeRecordToString(from->type, &from_string);
  TypeRecordToString(to, &to_string);
  SemanticError(from, format, from_string.value, to_string.value);
  StringDestruct(&from_string);
  StringDestruct(&to_string);
}

void SemanticTypeConversionWarning(ASTNode* from, TypeRecord* to,
                                   const char* warn, const char* format) {
  String from_string;
  String to_string;
  StringInit(&from_string, NULL);
  StringInit(&to_string, NULL);
  TypeRecordToString(from->type, &from_string);
  TypeRecordToString(to, &to_string);
  SemanticWarning(from, warn, format, from_string.value, to_string.value);
  StringDestruct(&from_string);
  StringDestruct(&to_string);
}

static ASTNode* SignExtendIntConstant(ConstantASTNode* c, int bits) {
  c->value.ivalue <<= 64 - bits;
  c->value.ivalue >>= 64 - bits;
  return (ASTNode*)c;
}

static ASTNode* ConvertIntToDouble(ConstantASTNode* c, int bits) {
  if (!TypeIsUnsigned(c->base.type)) {
    int64_t mask = (1LL << bits) - 1LL;
    c->value.ivalue &= mask;
  } else {
    c->value.ivalue <<= 64 - bits;
    c->value.ivalue >>= 64 - bits;
  }
  c->value.fvalue = (double)c->value.ivalue;
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
          return SignExtendIntConstant(c, 16);
        case AST_OP(i2c):
          return SignExtendIntConstant(c, 8);
        case AST_OP(i2l):
        case AST_OP(i2ll):
          return SignExtendIntConstant(c, 32);
        case AST_OP(i2f):
        case AST_OP(i2d):
        case AST_OP(i2ld):
          return ConvertIntToDouble(c, 32);
          return from;
        case AST_OP(i2b):
        case AST_OP(c2b):
        case AST_OP(s2b):
        case AST_OP(ll2b):
        case AST_OP(l2b):
          c->value.ivalue = c->value.ivalue != 0;
          return from;
        case AST_OP(c2i):
          return SignExtendIntConstant(c, 8);
        case AST_OP(c2s):
          return SignExtendIntConstant(c, 8);
        case AST_OP(c2l):
        case AST_OP(c2ll):
          return SignExtendIntConstant(c, 8);
        case AST_OP(c2f):
        case AST_OP(c2d):
        case AST_OP(c2ld):
          return ConvertIntToDouble(c, 8);
        case AST_OP(s2i):
          return SignExtendIntConstant(c, 16);
        case AST_OP(s2c):
          return SignExtendIntConstant(c, 16);
        case AST_OP(s2l):
        case AST_OP(s2ll):
          return SignExtendIntConstant(c, 16);
        case AST_OP(s2f):
        case AST_OP(s2d):
        case AST_OP(s2ld):
          return ConvertIntToDouble(c, 16);
        case AST_OP(l2i):
        case AST_OP(ll2i):
          return SignExtendIntConstant(c, 32);
        case AST_OP(l2c):
        case AST_OP(ll2c):
          return SignExtendIntConstant(c, 8);
        case AST_OP(l2s):
        case AST_OP(ll2s):
          return SignExtendIntConstant(c, 16);
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

void SemanticConvertType(ASTNode* from, TypeRecord* to) {
  // If the types are already equal we do nothing.
  if (TypeEqual(from->type, to)) {
    return;
  }

  if (TypeEqualIgnoringSign(from->type, to)) {
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
        SemanticTypeConversionWarning(from, to, "type-conversion",
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

  if (TypeIsPointerOrArray(from->type) && TypeIsPointerOrArray(to)) {
    TypeRecord* from_next = from->type->next;
    TypeRecord* to_next = to->next;
    if (!TypeEqual(from_next, to_next)) {
      SemanticTypeConversionWarning(from, to, "ptr-conversion",
                                    "Illegal pointer conversion; "
                                    "from '%s' to '%s'");
    }
    return;
  }

  // Treat enum and ints as same.
  if ((TypeIsEnum(from->type) && TypeIsInt(to)) ||
      (TypeIsInt(from->type) && TypeIsEnum(to))) {
    return;
  }

  // Allow the number 0 (explicitly) to be converted to a pointer.
  if (from->op == AST_OP(number) && TypeIsPointer(to)) {
    ConstantASTNode* const_node = (ConstantASTNode*)from;
    if (TypeIsInt(const_node->base.type) && const_node->value.ivalue == 0) {
      return;
    }
  }

  if (TypeIsFunction(from->type) && TypeIsFunctionPointer(to)) {
    // Functions can be converted to function pointers to the same type.
    if (TypeEqual(from->type, to->next)) {
      return;
    }
  }

  SemanticTypeConversionError(
      from, to, "Illegal conversion; cannot convert from '%s' to '%s'");
  return;
}

void SemanticAnalyzeVariableDefinition(Syntax* syntax,
                                       VariableDeclarationASTNode* node) {
  if (TypeIsVoid(node->symbol->type)) {
    SemanticError((ASTNode*)node, "Cannot define a variable with void type");
  }
  if (node->initializer == NULL) {
    return;
  }
  AnalyzeExpression(node->initializer);
  // TODO: at this level the expression must be evaluatable at compile time.
  SemanticConvertType(node->initializer, node->symbol->type);
  ASTNodeSetType((ASTNode*)node, node->symbol->type);
}
