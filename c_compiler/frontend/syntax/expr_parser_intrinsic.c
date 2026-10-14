//
//  expr_parser_intrinsic.c
//  c_compiler
//
//  Compiler intrinsics, type-trait builtins, offsetof, and varargs builtins.
//

#include <stdint.h>

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "concepts.h"
#include "errors.h"
#include "expr_evaluator.h"
#include "expr_parser.h"
#include "expr_parser_internal.h"
#include "expr_semantics.h"
#include "preprocessor.h"
#include "reflection.h"
#include "statement_parser.h"
#include "symbol_table.h"
#include "std_header_suggestion.h"
#include "typo_correction.h"
#include "type.h"
#include "type_class_internal.h"
#include "type_internal.h"
#include "type_member.h"
#include "compiler.h"
#include "type_traits_semantics.h"

static struct Intrinsic {
  const char* name;
  ASTOpcode opcode;
  int num_args;
} intrinsics[] = {
    {"__atomic_add_fetch", AST_OP(builtin_atomic_add_fetch), 3},
    {"__atomic_compare_exchange_n", AST_OP(builtin_atomic_compare_exchange_n), 6},
    {"__atomic_fetch_add", AST_OP(builtin_atomic_fetch_add), 3},
    {"__atomic_fetch_sub", AST_OP(builtin_atomic_fetch_sub), 3},
    {"__atomic_load_n", AST_OP(builtin_atomic_load), 2},
    {"__atomic_signal_fence", AST_OP(builtin_atomic_fence), 1},
    {"__atomic_store_n", AST_OP(builtin_atomic_store), 3},
    {"__atomic_sub_fetch", AST_OP(builtin_atomic_sub_fetch), 3},
    {"__atomic_thread_fence", AST_OP(builtin_atomic_fence), 1},
    {"__builtin_COLUMN", AST_OP(builtin_source_column), 0},
    {"__builtin_FILE", AST_OP(builtin_source_file), 0},
    {"__builtin_FUNCTION", AST_OP(builtin_source_function), 0},
    {"__builtin_LINE", AST_OP(builtin_source_line), 0},
    {"__builtin_PRETTY_FUNCTION", AST_OP(builtin_source_pretty_function), 0},
    {"__builtin_alloca", AST_OP(builtin_alloca), 1},
    {"__builtin_bswap16", AST_OP(builtin_bswap), 1},
    {"__builtin_bswap32", AST_OP(builtin_bswap), 1},
    {"__builtin_bswap64", AST_OP(builtin_bswap), 1},
    {"__builtin_clz", AST_OP(builtin_clz), 1},
    {"__builtin_clzll", AST_OP(builtin_clz), 1},
    {"__builtin_constant_p", AST_OP(builtin_constant_p), 1},
    {"__builtin_ctz", AST_OP(builtin_ctz), 1},
    {"__builtin_ctzll", AST_OP(builtin_ctz), 1},
    {"__builtin_expect", AST_OP(builtin_expect), 2},
    {"__builtin_is_constant_evaluated",
     AST_OP(builtin_is_constant_evaluated), 0},
    {"__builtin_observable_checkpoint",
     AST_OP(builtin_observable_checkpoint), 0},
    {"__builtin_popcount", AST_OP(builtin_popcount), 1},
    {"__builtin_popcountll", AST_OP(builtin_popcount), 1},
    {"__builtin_prefetch", AST_OP(builtin_prefetch), 3},
    {"__builtin_start_lifetime", AST_OP(builtin_start_lifetime), 1},
    {"__builtin_trap", AST_OP(builtin_trap), 0},
    {"__builtin_unreachable", AST_OP(builtin_unreachable), 0},
    {"__builtin_va_arg", AST_OP(builtin_va_arg), 2},
    {"__builtin_va_copy", AST_OP(builtin_va_copy), 2},
    {"__builtin_va_end", AST_OP(builtin_va_end), 1},
    {"__builtin_va_start", AST_OP(builtin_va_start), 2},
    {"__davecc_clz", AST_OP(builtin_clz), 2},
    {"__davecc_ctz", AST_OP(builtin_ctz), 2},
    {"__davecc_popcount", AST_OP(builtin_popcount), 2},
    {"__davecc_rotl", AST_OP(builtin_rotl), 2},
    {"__davecc_rotr", AST_OP(builtin_rotr), 2},
    {"__sync_add_and_fetch", AST_OP(builtin_atomic_add_fetch), 2},
    {"__sync_bool_compare_and_swap", AST_OP(builtin_atomic_compare_exchange_bool), 3},
    {"__sync_fetch_and_add", AST_OP(builtin_atomic_fetch_add), 2},
    {"__sync_fetch_and_sub", AST_OP(builtin_atomic_fetch_sub), 2},
    {"__sync_sub_and_fetch", AST_OP(builtin_atomic_sub_fetch), 2},
    {"__sync_synchronize", AST_OP(builtin_atomic_fence), 0},
    {"__sync_val_compare_and_swap", AST_OP(builtin_atomic_compare_exchange_val), 3},
};


typedef struct {
  const char* name;
  CXXTypeTraitKind kind;
} TypeTraitName;


static const TypeTraitName kTypeTraitNames[] = {
    {"__davecc_is_assignable", kCXXTypeTraitIsAssignable},
    {"__davecc_is_base_of", kCXXTypeTraitIsBaseOf},
    {"__davecc_is_class", kCXXTypeTraitIsClass},
    {"__davecc_is_constructible", kCXXTypeTraitIsConstructible},
    {"__davecc_is_convertible", kCXXTypeTraitIsConvertible},
    {"__davecc_is_destructible", kCXXTypeTraitIsDestructible},
    {"__davecc_is_empty", kCXXTypeTraitIsEmpty},
    {"__davecc_is_enum", kCXXTypeTraitIsEnum},
    {"__davecc_is_final", kCXXTypeTraitIsFinal},
    {"__davecc_is_invocable", kCXXTypeTraitIsInvocable},
    {"__davecc_is_invocable_r", kCXXTypeTraitIsInvocableR},
    {"__davecc_is_member_function_pointer",
     kCXXTypeTraitIsMemberFunctionPointer},
    {"__davecc_is_member_object_pointer", kCXXTypeTraitIsMemberObjectPointer},
    {"__davecc_is_member_pointer", kCXXTypeTraitIsMemberPointer},
    {"__davecc_is_member_pointer_direct_object",
     kCXXTypeTraitMemberPointerDirectObject},
    {"__davecc_is_nothrow_assignable", kCXXTypeTraitIsNothrowAssignable},
    {"__davecc_is_nothrow_constructible",
     kCXXTypeTraitIsNothrowConstructible},
    {"__davecc_is_nothrow_destructible", kCXXTypeTraitIsNothrowDestructible},
    {"__davecc_is_nothrow_invocable", kCXXTypeTraitIsNothrowInvocable},
    {"__davecc_is_polymorphic", kCXXTypeTraitIsPolymorphic},
    {"__davecc_is_standard_layout", kCXXTypeTraitIsStandardLayout},
    {"__davecc_is_swappable", kCXXTypeTraitIsSwappable},
    {"__davecc_is_swappable_with", kCXXTypeTraitIsSwappableWith},
    {"__davecc_is_trivially_assignable", kCXXTypeTraitIsTriviallyAssignable},
    {"__davecc_is_trivially_constructible",
     kCXXTypeTraitIsTriviallyConstructible},
    {"__davecc_is_trivially_copyable", kCXXTypeTraitIsTriviallyCopyable},
    {"__davecc_is_trivially_destructible",
     kCXXTypeTraitIsTriviallyDestructible},
    {"__davecc_is_union", kCXXTypeTraitIsUnion},
};


static int CompareIntrinsicName(const void* key, const void* element);
static CXXTypeTraitKind TypeTraitKindFromName(const char* name);
static Vector* ParseTypeTraitTypeArguments(Syntax* syntax, TokenClass followers);
static StructMember* FindOffsetofMemberOnStruct(Struct* str, const char* name, int64_t base_offset, int64_t* byte_offset);
static Struct* StructFromTypeRecord(TypeRecord* type);
static TypeRecord* BareOffsetofType(TypeRecord* type);
static bool TypeIsPairLikeForOffsetof(TypeRecord* bare);
static TypeRecord* TypedefMemberType(TypeRecord* owner, const char* member);
static Struct* StructForOffsetofType(Syntax* syntax, TypeRecord* type);
static bool ResolveOffsetofDesignator(Syntax* syntax, TypeRecord* type, int64_t* offset);
static bool ResolveOffsetofMember(Syntax* syntax, TypeRecord* type, const char* member_name, int64_t* offset);


// Compares an intrinsic-table entry with a lookup key for bsearch.
static int CompareIntrinsicName(const void* key, const void* element) {
  const char* name = key;
  const struct Intrinsic* intrinsic = element;
  return strcmp(name, intrinsic->name);
}


// Looks up a builtin intrinsic by its source-level name.
const struct Intrinsic* GetIntrinsic(const char* name) {
  return bsearch(name, intrinsics,
                 sizeof(intrinsics) / sizeof(intrinsics[0]),
                 sizeof(intrinsics[0]), CompareIntrinsicName);
}


// Reports whether the target supports an intrinsic, rejecting atomic builtins
// on targets without atomics.
bool IntrinsicAvailableOnCurrentTarget(
    const struct Intrinsic* intrinsic) {
  if (intrinsic == NULL) {
    return false;
  }
  bool atomic = false;
  switch (intrinsic->opcode) {
    case AST_OP(builtin_atomic_load):
    case AST_OP(builtin_atomic_store):
    case AST_OP(builtin_atomic_fetch_add):
    case AST_OP(builtin_atomic_fetch_sub):
    case AST_OP(builtin_atomic_add_fetch):
    case AST_OP(builtin_atomic_sub_fetch):
    case AST_OP(builtin_atomic_compare_exchange_bool):
    case AST_OP(builtin_atomic_compare_exchange_val):
    case AST_OP(builtin_atomic_compare_exchange_n):
    case AST_OP(builtin_atomic_fence):
      atomic = true;
      break;
    default:
      break;
  }
  return !atomic || CompilerTargetSupportsAtomics();
}


// Recognizes table-driven builtins and the compiler type-trait builtin family.
bool IsBuiltinCallName(const char* name) {
  if (GetIntrinsic(name) != NULL) {
    return true;
  }
  return strncmp(name, "__davecc_is_", 12) == 0;
}


// Maps a type-trait builtin name to its semantic trait kind.
static CXXTypeTraitKind TypeTraitKindFromName(const char* name) {
  if (name == NULL || name[0] != '_' || name[1] != '_') {
    return (CXXTypeTraitKind)-1;
  }
  size_t low = 0;
  size_t high = sizeof(kTypeTraitNames) / sizeof(kTypeTraitNames[0]);
  while (low < high) {
    size_t middle = low + (high - low) / 2;
    int comparison = strcmp(name, kTypeTraitNames[middle].name);
    if (comparison < 0) {
      high = middle;
    } else if (comparison > 0) {
      low = middle + 1;
    } else {
      return kTypeTraitNames[middle].kind;
    }
  }
  return (CXXTypeTraitKind)-1;
}


// Parses the comma-separated type operands of a type-trait builtin, preserving
// pack expansions.
static Vector* ParseTypeTraitTypeArguments(Syntax* syntax, TokenClass followers) {
  Vector* type_args = NewVector();
  while (!LexLookingAt(syntax->lex, TOK(rparen))) {
    TypeParser parser;
    TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), kParsingBlockScope);
    TypeRecord* type = TypeParserParseType(&parser, true);
    Symbol* sym = TypeParserParseDeclarator(&parser, type);
    type = sym->type;
    ASTNode* type_node =
        NewIntConstantASTNode(0, TypeRecordCopy(type), syntax->lex->current_token_location);
    if (sym->flags.is_parameter_pack ||
        (CompilerIsCXX() && LexLookingAt(syntax->lex, TOK(ellipsis)))) {
      if (LexMatch(syntax->lex, TOK(ellipsis))) {
        type_node->flags |= kASTPackExpansion;
      } else if (sym->flags.is_parameter_pack) {
        type_node->flags |= kASTPackExpansion;
      }
    }
    VectorAppend(type_args, type_node);
    SymbolDelete(sym);
    TypeParserDestruct(&parser);
    if (!LexMatch(syntax->lex, TOK(comma))) {
      break;
    }
  }
  SyntaxNeedBracket(syntax, TOK(rparen), followers);
  return type_args;
}


// Rewrites a recognized type-trait call into a builtin type-trait AST node.
ASTNode* TypeTraitIntrinsic(Syntax* syntax, ASTNode* left,
                                   TokenClass followers) {
  if (left->op != AST_OP(identifier)) {
    return NULL;
  }
  IdentifierASTNode* id_node = (IdentifierASTNode*)left;
  const char* name = id_node->symbol->name.value;
  CXXTypeTraitKind kind = TypeTraitKindFromName(name);
  if ((int)kind < 0) {
    return NULL;
  }
  Vector* type_args = ParseTypeTraitTypeArguments(syntax, followers);
  ASTNode* kind_node = NewIntConstantASTNode(
      (int)kind, NewTypeRecordWithSize(kTypeInt, kQualPlain),
      syntax->lex->current_token_location);
  Vector* actuals = NewVector();
  VectorAppend(actuals, kind_node);
  for (size_t i = 0; i < type_args->length; i++) {
    VectorAppend(actuals, type_args->value.p[i]);
  }
  VectorDelete(type_args);
  return NewVectorASTNode(AST_OP(builtin_type_trait), NULL,
                          syntax->lex->current_token_location, left, actuals);
}


// Finds a member in a class or its bases and computes its byte offset from the
// complete object.
static StructMember* FindOffsetofMemberOnStruct(Struct* str, const char* name,
                                                int64_t base_offset,
                                                int64_t* byte_offset) {
  if (str == NULL || name == NULL) {
    return NULL;
  }
  StructMember* member = FindStructMemberByName(str, name);
  if (member != NULL) {
    if (byte_offset != NULL) {
      *byte_offset = base_offset + member->byte_offset;
    }
    return member;
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base->type == NULL || !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    member = FindOffsetofMemberOnStruct(base->type->info.struct_info, name,
                                        base_offset + base->byte_offset,
                                        byte_offset);
    if (member != NULL) {
      return member;
    }
  }
  return NULL;
}


// Removes pointer and reference layers and returns the represented class or
// union, if any.
static Struct* StructFromTypeRecord(TypeRecord* type) {
  TypeRecord* bare = type;
  while (bare != NULL && (TypeIsPointer(bare) || TypeIsReference(bare))) {
    bare = bare->next;
  }
  if (bare != NULL && TypeIsStructOrUnion(bare) &&
      bare->info.struct_info != NULL) {
    return bare->info.struct_info;
  }
  return NULL;
}


// Removes pointer and reference layers from a type used by offsetof
// resolution.
static TypeRecord* BareOffsetofType(TypeRecord* type) {
  TypeRecord* bare = type;
  while (bare != NULL && (TypeIsPointer(bare) || TypeIsReference(bare))) {
    bare = bare->next;
  }
  return bare;
}


// Recognizes pair-like concrete or dependent types whose first member is known
// to start at offset zero.
static bool TypeIsPairLikeForOffsetof(TypeRecord* bare) {
  if (bare == NULL) {
    return false;
  }
  if (bare->dependent_member_name != NULL &&
      strcmp(bare->dependent_member_name->value, "value_type") == 0) {
    return true;
  }
  if (TypeIsStructOrUnion(bare) && bare->info.struct_info != NULL) {
    return FindStructMemberByName(bare->info.struct_info, "first") != NULL &&
           FindStructMemberByName(bare->info.struct_info, "second") != NULL;
  }
  return false;
}


// Returns the type named by a typedef member of a class, or NULL when no such
// typedef exists.
static TypeRecord* TypedefMemberType(TypeRecord* owner, const char* member) {
  if (owner == NULL || member == NULL || !TypeIsStructOrUnion(owner) ||
      owner->info.struct_info == NULL) {
    return NULL;
  }
  String name;
  StringInit(&name, member);
  StructMember* typedef_member = FindStructMember(owner->info.struct_info, &name);
  StringDestruct(&name);
  if (typedef_member == NULL || typedef_member->symbol == NULL ||
      !StorageIs(typedef_member->symbol->storage, STO(typedef)) ||
      typedef_member->symbol->type == NULL) {
    return NULL;
  }
  return typedef_member->symbol->type;
}


// Resolves a concrete class for an offsetof operand, including dependent
// member typedefs when possible.
static Struct* StructForOffsetofType(Syntax* syntax, TypeRecord* type) {
  Struct* str = StructFromTypeRecord(type);
  if (str != NULL) {
    return str;
  }
  TypeRecord* bare = BareOffsetofType(type);
  if (bare == NULL || bare->dependent_member_name == NULL ||
      bare->template_parameter_index < 0) {
    return NULL;
  }
  Vector* parameters = syntax->current_template_parameters;
  if (parameters == NULL ||
      (size_t)bare->template_parameter_index >= parameters->length) {
    return NULL;
  }
  TemplateParameter* parameter =
      parameters->value.p[bare->template_parameter_index];
  if (parameter == NULL || parameter->name.length == 0) {
    return NULL;
  }
  String param_name;
  StringInit(&param_name, parameter->name.value);
  Symbol* param_symbol = SyntaxFindSymbol(syntax, &param_name);
  StringDestruct(&param_name);
  if (param_symbol == NULL || param_symbol->type == NULL) {
    return NULL;
  }
  TypeRecord* owner = BareOffsetofType(param_symbol->type);
  TypeRecord* resolved =
      TypedefMemberType(owner, bare->dependent_member_name->value);
  return StructFromTypeRecord(resolved);
}


// Parses a dotted offsetof member designator and accumulates its byte offset.
static bool ResolveOffsetofDesignator(Syntax* syntax, TypeRecord* type,
                                      int64_t* offset) {
  int64_t byte_offset = 0;
  Struct* current = StructForOffsetofType(syntax, type);
  while (true) {
    if (!LexLookingAt(syntax->lex, TOK(identifier))) {
      SyntaxError(syntax,
                  "Expected member name in __builtin_offsetof(type, member)");
      return false;
    }
    String member_name;
    StringInit(&member_name, syntax->lex->spelling.value);
    LexNextToken(syntax->lex);
    bool found = false;
    TypeRecord* member_type = NULL;
    if (current != NULL &&
        FindOffsetofMemberOnStruct(current, member_name.value, byte_offset,
                                   &byte_offset) != NULL) {
      found = true;
      StructMember* member = FindStructMemberByName(current, member_name.value);
      member_type = member != NULL && member->symbol != NULL
                        ? member->symbol->type
                        : NULL;
    } else if (ResolveOffsetofMember(syntax, type, member_name.value,
                                     &byte_offset)) {
      found = true;
    } else if (strcmp(member_name.value, "first") == 0 &&
               TypeIsPairLikeForOffsetof(BareOffsetofType(type))) {
      found = true;
    }
    StringDestruct(&member_name);
    if (!found) {
      SyntaxError(syntax, "Member is not a member of the given type");
      return false;
    }
    while (LexMatch(syntax->lex, TOK(lsquare))) {
      if (member_type == NULL ||
          (!TypeIsArray(member_type) && !TypeIsVector(member_type))) {
        SyntaxError(syntax,
                    "offsetof subscript requires an array or vector member");
        return false;
      }
      ASTNode* index_expr =
          SyntaxParseExpression(syntax, TC(closebra) | TC(exprsep));
      index_expr = AnalyzeExpression(index_expr);
      int64_t index = 0;
      bool constant_index =
          index_expr != NULL && EvaluateIntegerExpression(index_expr, &index);
      ASTNodeDelete(index_expr);
      SyntaxNeedBracket(syntax, TOK(rsquare), TC(closebra) | TC(exprsep));
      if (!constant_index) {
        SyntaxError(syntax,
                    "offsetof array subscript must be an integer constant");
        return false;
      }
      TypeRecord* element_type = member_type->next;
      TypeRecordCalculateSize(element_type);
      byte_offset += index * element_type->size;
      member_type = element_type;
    }
    current = StructFromTypeRecord(member_type);
    if (!LexMatch(syntax->lex, TOK(dot))) {
      break;
    }
    if (current == NULL) {
      SyntaxError(syntax,
                  "Cannot apply nested member designator to dependent type");
      return false;
    }
  }
  *offset = byte_offset;
  return true;
}


// Resolves one offsetof member against concrete or dependent class
// information.
static bool ResolveOffsetofMember(Syntax* syntax, TypeRecord* type,
                                  const char* member_name, int64_t* offset) {
  TypeRecord* bare = type;
  while (bare != NULL && (TypeIsPointer(bare) || TypeIsReference(bare))) {
    bare = bare->next;
  }
  if (bare != NULL && TypeIsStructOrUnion(bare) &&
      bare->info.struct_info != NULL) {
    int64_t byte_offset = 0;
    if (FindOffsetofMemberOnStruct(bare->info.struct_info, member_name, 0,
                                   &byte_offset) != NULL) {
      *offset = byte_offset;
      return true;
    }
  }
  if (bare == NULL || !TypeContainsTemplateParameter(bare)) {
    return false;
  }
  Vector* parameters = syntax->current_template_parameters;
  if (parameters == NULL) {
    return false;
  }
  for (size_t i = 0; i < parameters->length; i++) {
    TemplateParameter* parameter = parameters->value.p[i];
    if (parameter == NULL || parameter->kind != kTemplateParameterType ||
        parameter->name.length == 0) {
      continue;
    }
    Symbol* tag = SyntaxFindTag(syntax, &parameter->name);
    if (tag == NULL || tag->type == NULL || !TypeIsStructOrUnion(tag->type) ||
        tag->type->info.struct_info == NULL) {
      continue;
    }
    int64_t byte_offset = 0;
    if (FindOffsetofMemberOnStruct(tag->type->info.struct_info, member_name, 0,
                                   &byte_offset) != NULL) {
      *offset = byte_offset;
      return true;
    }
  }
  if (strcmp(member_name, "first") == 0 &&
      TypeIsPairLikeForOffsetof(bare)) {
    *offset = 0;
    return true;
  }
  Struct* dependent = StructForOffsetofType(syntax, type);
  if (dependent != NULL &&
      FindOffsetofMemberOnStruct(dependent, member_name, 0, offset) != NULL) {
    return true;
  }
  return false;
}


// Parses table-driven builtin calls, including va_arg and offsetof special
// operands, and validates their arity.
ASTNode* VarargsIntrinsic(Syntax* syntax, ASTNode* left,
                               TokenClass followers) {
  if (left->op == AST_OP(identifier)) {
    IdentifierASTNode* id_node = (IdentifierASTNode*)left;
    const char* name = id_node->symbol->name.value;
    SourceLocation location = left->location;

    if (strcmp(name, "__builtin_offsetof") == 0) {
      TypeParser parser;
      TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                     kParsingBlockScope);
      TypeRecord* type = TypeParserParseType(&parser, true);
      Symbol* type_sym = TypeParserParseDeclarator(&parser, type);
      type = type_sym->type;
      TypeParserDestruct(&parser);
      SyntaxNeedBracket(syntax, TOK(comma), followers);
      int64_t offset = 0;
      if (!ResolveOffsetofDesignator(syntax, type, &offset)) {
        SyntaxError(syntax, "Invalid member designator in __builtin_offsetof");
      }
      SyntaxNeedBracket(syntax, TOK(rparen), followers);
      SymbolDelete(type_sym);
      TypeRecord* size_type =
          NewTypeRecordWithSize(kTypeLong | kTypeUnsigned, kQualPlain);
      ASTNode* result =
          NewIntConstantASTNode(offset, size_type, location);
      ASTNodeDelete(left);
      return result;
    }

    const struct Intrinsic* intrinsic = GetIntrinsic(name);
    if (intrinsic == NULL) {
      return NULL;
    }

    Vector* actuals = NewVector();
    while (!LexLookingAt(syntax->lex, TOK(rparen))) {
      ASTNode* actual;
      // Special case: __builtin_va_arg has a type as its second arg.
      if (intrinsic->opcode == AST_OP(builtin_va_arg) &&
          actuals->length == 1) {
        TypeParser parser;
        TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), kParsingBlockScope);
        TypeRecord* type = TypeParserParseType(&parser, true);
        Symbol* sym = TypeParserParseDeclarator(&parser, type);
        type = sym->type;
        actual =
            NewIntConstantASTNode(0, type, syntax->lex->current_token_location);
        SymbolDelete(sym);  // Don't need this.
        TypeParserDestruct(&parser);
      } else {
        actual = SyntaxParseSingleExpression(syntax, followers | TC(exprsep));
      }
      VectorAppend(actuals, actual);
      if (!LexMatch(syntax->lex, TOK(comma))) {
        break;
      }
    }
    SyntaxNeedBracket(syntax, TOK(rparen), followers);
    bool valid_arity =
        intrinsic->opcode == AST_OP(builtin_prefetch)
            ? actuals->length >= 1 && actuals->length <= 3
            : actuals->length == (size_t)intrinsic->num_args;
    if (!valid_arity) {
      if (intrinsic->opcode == AST_OP(builtin_prefetch)) {
        SyntaxError(
            syntax,
            "Wrong number of args for builtin; expected between 1 and 3, got %zd",
            actuals->length);
      } else {
      SyntaxError(
          syntax,
          "Wrong number of args for builtin; expected %d, got %zd",
          intrinsic->num_args, actuals->length);
      }
    }
    return NewVectorASTNode(intrinsic->opcode, NULL,
                            syntax->lex->current_token_location, left, actuals);
  }
  return NULL;
}


// Parses __builtin_bit_cast(type, expression) and builds a bit-cast AST node.
ASTNode* BitCastIntrinsic(Syntax* syntax, ASTNode* left,
                                 TokenClass followers) {
  if (left->op != AST_OP(identifier) ||
      ((IdentifierASTNode*)left)->symbol == NULL ||
      !StringEqual(&((IdentifierASTNode*)left)->symbol->name,
                   "__builtin_bit_cast")) {
    return NULL;
  }
  SourceLocation location = left->location;
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                 kParsingBlockScope);
  TypeRecord* type = TypeParserParseType(&parser, true);
  Symbol* sym = NULL;
  if (type == NULL) {
    SyntaxError(syntax, "Invalid type name");
    type = NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
  } else {
    sym = TypeParserParseDeclarator(&parser, type);
    type = sym->type;
  }
  TypeParserDestruct(&parser);
  SyntaxNeedBracket(syntax, TOK(comma), followers);
  ASTNode* expr = SyntaxParseSingleExpression(syntax, TC(closebra));
  SyntaxNeedBracket(syntax, TOK(rparen), followers);
  ASTNode* result = NewCastASTNode(type, location, expr);
  ((CastASTNode*)result)->kind = kCastBit;
  if (sym != NULL) {
    SymbolDelete(sym);
  }
  ASTNodeDelete(left);
  return result;
}
